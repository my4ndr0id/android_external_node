// Copyright Joyent, Inc. and other Node contributors.
// Copyright (c) 2011, Code Aurora Forum. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef NODE_H_
#define NODE_H_

#include <uv.h>

/* C api exposed from Node, essentially for libev C code */
#ifdef __cplusplus
extern "C" {
#endif
  int gettid();
  void on_ev_start(ev_watcher *w);
  void on_ev_stop(ev_watcher *w);
  void lock();
  void unlock();
  void wakeup();
  int is_main_thread();

#define LOCK \
  lock();

#define UNLOCK \
  unlock();

#define UNLOCK_N_WAKEUP \
  wakeup();           \
  unlock();

#ifdef __cplusplus
};
#endif

#ifdef __cplusplus

#include <v8.h>
#include <vector>
#include <set>
#include "eio.h"
#include "node_object_wrap.h"
#include "dapi.h"
#include "dapi_module.h"

namespace node {

class Node;

class Lock {
  public:
    Lock(Node* n, v8::Persistent<v8::Function> func, v8::Persistent<v8::Object> obj)
    {
      m_lockFunction = func;
      m_node = n;
      m_obj = obj;
    }
    static void LockWeakCallback(v8::Persistent<v8::Value> value,void* data){
      NODE_LOGV("%s, LockWeakCallback for dispose : %s", __FUNCTION__, (char*) data);
      value.Dispose();
    }
    ~Lock(){
      NODE_LOGV("%s, set LockWeakCallback for lock : %p", __FUNCTION__, this);
      m_lockFunction.MakeWeak((void*)"lock",LockWeakCallback);
      m_obj.MakeWeak((void*)"lockObj",LockWeakCallback);
    }

    v8::Persistent<v8::Function> m_lockFunction;
    v8::Persistent<v8::Object> m_obj;
    Node* m_node;
};

typedef enum {
  MODULE_UNKNOWN,
  MODULE_FS,
  MODULE_CAMERA
} ModuleId;

/**
 * Interface to be implemented by modules interested in getting events
 * from node
 */
class NodeModule {
  public:
    virtual ~NodeModule() {}
    virtual ModuleId Module() = 0;
};

class StopWatch {
  public:
    StopWatch() : m_started(false) {}
    void start();
    int time();
    bool started() { return m_started; }

  private:
    static double currentTime();
    bool m_started;
    double m_startTime;
};

enum TestStatus {FAILED, PASSED, CRASHED, TIMEOUT};
enum TestState {INIT, STARTED, DONE, REPORTED};
enum encoding {ASCII, UTF8, BASE64, UCS2, BINARY, HEX};

/**
 * Represents a node instance, there is one for each browser context
 * Gets created when the webpage loads a module through navigator.loadModule
 */
class Node :
  public dapi::INodeCore,
  public dapi::INodeEvents {
  public:
    /**
     * Creates a node instance, initializes it and loads it.
     * initialization includes creation of libev watchers, and creating the process object
     * load bootstraps node by reading/compiling/executing the builtin modules
     * (node.js, buffer.js, module.js, fs.js etc)
     * @param client Handle to browser, used for sending events
     */
    Node(dapi::INode *inode);

    /**
     * Destroys the node instance, triggered by embedder when page is navigated out
     * destroys all persistent handles it holds in current instance, stops all libev watchers
     * signals to module for cleanup (through release event)
     */
    ~Node();

    /**
     * Bootup node, create ev thread ..
     * @param isBrowser specifies if client is a browser or a shell
     * @param moduleRootPath Root directory specified by client for installing downloaded modules
     */
    static void Initialize(void (*clientcb)(), bool isBrowser, const char* moduleRootPath);

    /**
     * Invoked by the client on the main thread to process pending libev events
     * (internally calls ev_invoke_pending)
     */
    static void InvokePending();

    /**
     * Get handle to loadModule function in the current node context
     * this will be set as window.navigator.loadModule by the client
     * @return handle to the loadModule function in node context
     */
    v8::Handle<v8::Function> GetLoadModule();
    v8::Handle<v8::Function> GetRequire();

    /**
     * This can be used to emit custom events from native code,
     * to emit event from js use 'process.emit('event name')'
     * The pattern is process.on('event', function() {}); and
     * from native code emit EmitEvent("event");
     * @param event name of the event
     */
    void EmitEvent(const char* event);

    /**
     * Invokes the method on the given object with argc/argv as input
     */
    static void MakeCallback(v8::Handle<v8::Object> object,
        const char* method, int argc, v8::Handle<v8::Value> argv[]);

    /**
     * Returns the context associated with this node instance
     * @return node context
     */
    v8::Handle<v8::Context> context() { return m_context; }

    /**
     * Retreive the node instance from the object
     * @param process process object
     * @return node instance for the given process
     */
    static Node* GetNodeFromObject(v8::Handle<v8::Object> o);
    static Node* GetCurrentNode();

    /**
     * Executes the given string in node context
     * @param source source code to be compiled/run
     * @param filename filename associated with the source code for reporting errors
     * @return return value after evaluating the source
     */
    static v8::Local<v8::Value> ExecuteString(v8::Handle<v8::String> source,
        v8::Handle<v8::Value> filename);

    static void FatalException(v8::TryCatch &try_catch);
    static void DisplayExceptionLine(v8::TryCatch &try_catch); 
    static void ReportException(v8::TryCatch &try_catch, bool show_line);

    /* thread checks */
    static bool IsMainThread();

      /* weak callback */
    static void WeakCallback(v8::Persistent<v8::Value> value, void *data);

    /**
     * Call this when your constructor is invoked as a regular function,
     * e.g. Buffer(10) instead of new Buffer(10).
     * @param constructorTemplate Constructor template to instantiate from.
     * @param args The arguments object passed to your constructor.
     * @see Arguments::IsConstructCall
     */
    static v8::Handle<v8::Value> FromConstructorTemplate(
        v8::Persistent<v8::FunctionTemplate>& constructorTemplate, const v8::Arguments& args);

    // Returns -1 if the handle was not valid for decoding
    static ssize_t DecodeBytes(v8::Handle<v8::Value>,
                    enum encoding encoding = BINARY);
    static enum encoding ParseEncoding(v8::Handle<v8::Value> encoding_v,
                            enum encoding _default = BINARY);
    // returns bytes written.
    static ssize_t DecodeWrite(char *buf, size_t buflen, v8::Handle<v8::Value>,
                    enum encoding encoding = BINARY);

    static v8::Local<v8::Value> Encode(const void *buf, size_t len,
                            enum encoding encoding = BINARY);

    static node_module_struct*
      get_builtin_module(const char *name);

    // js/native traces
    static void PrintJSStackTrace(DAPILogPriority pri = DAPI_LOG_WARN);

#ifndef ANDROID // only supported on desktop for now..
    static void PrintNativeStackTrace(DAPILogPriority pri = DAPI_LOG_WARN);
    static std::string AddressToString(void *addr);
#endif

    // returns host loop, e.g. in non-browser environment
    static struct ev_loop* HostLoop();

    // return exit code for test.py
    static int ExitCode();

    dapi::INodeClient* client() { return m_inode->client(); }
    dapi::INode* inode() { return m_inode; }

    void Init();
    void SetupProcessObject();
    void Load(); // load all builtin modules in current context
    void Tick();

    static void setINodeInObject(v8::Handle<v8::Object> o, dapi::INode *inode);

    // INodeEvents
    void onPause();
    void onResume();
    void onIdle();

    // INodeCore

    /**
     * This invokes loadModule in node's context. This function also
     * places try catch blocks around the call to catch any errors for test reporting
     * @param args module to load
     * @return Reference to the module after loading it
     */
    v8::Handle<v8::Value> loadModule(v8::Handle<v8::Value>* args);
    v8::Handle<v8::Value> require(v8::Handle<v8::Value>* args);
    v8::Handle<v8::Context> inodeContext() { return m_context; }
    v8::Handle<v8::Context> inodeClientContext() { return m_browserContext; }
    void addWatcherWrap(void* watcherWrap);
    void removeWatcherWrap(void* watcherWrap);

  private:

    /**
     * Runs an javascript string in service node context
     */
    static v8::Local<v8::Value> RunScriptInServiceNode(v8::Handle<v8::String> source);

    v8::Persistent<v8::Object>  m_process;
    v8::Persistent<v8::Object>  m_global;
    v8::Persistent<v8::Object>  m_test;
    v8::Persistent<v8::Context> m_context;
    v8::Persistent<v8::Context> m_browserContext;
    v8::Persistent<v8::Object>  m_bindingCache;
    v8::Persistent<v8::Function>  m_loadModule;
    v8::Persistent<v8::Function>  m_require;

    StopWatch m_stopWatch;

    // modules registered to the current node instance
    std::vector<NodeModule*> m_modules;

    // watchers related
    uv_counters_t m_watchers_active;
    uv_prepare_t m_prepare_tick_watcher;
    uv_check_t m_check_tick_watcher;
    uv_idle_t m_tick_spinner;
    bool m_need_tick_cb;

    // watcher for timeouts
    uv_timer_t  m_test_timeout_watcher;

    // maintains a list of watcher wrappers, this enables us to
    // cleanup watchers at delete
    std::set<void*> m_watcherWrapSet;

    // INodeClient (e.g. webkit node proxy)
    dapi::INode *m_inode;

    friend class NodeStatic;
};


v8::Local<v8::Value> ErrnoException(int errorno, const char *syscall = NULL,
    const char *msg = "", const char *path = NULL);

void SetErrno(uv_err_code code);

}  // namespace node

#endif // __cplusplus
#endif
