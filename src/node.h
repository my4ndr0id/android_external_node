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

#include <pthread.h>
#include <eio.h>
#include <v8.h>
#include <vector>
#include <node_object_wrap.h>
#include <nodelog.h>
#include <node_bridge.h>

namespace node {

class NodeView;
class Node;
class NodeSource : public ObjectWrap {
  public:
    NodeSource() : m_view(0) {}
    virtual ~NodeSource(){}

    virtual void SetView(NodeView *view) = 0;
    virtual void SetPreviewTexture(void* texture) = 0;
    virtual void Attach(NodeView*, void*) = 0;
    virtual void Detach(NodeView*) = 0;

    virtual NodeView *View() { return m_view; }

  protected:
    NodeView *m_view;
};

class Lock {
  public:
    Lock(Node* n, v8::Persistent<v8::Function> func)
    {
      lockFunction = func;
      s_node = n;
    }
    static void LockWeakCallback(v8::Persistent<v8::Value> value,void* data){
      NODE_LOGV("%s, LockWeakCallback for dispose : %s", __FUNCTION__, (char*) data);
      value.Dispose();
    }
    ~Lock(){
      NODE_LOGV("%s, set LockWeakCallback for lock : %p", __FUNCTION__, this);
      lockFunction.MakeWeak((void*)"lock",LockWeakCallback);
    }

    v8::Persistent<v8::Function> lockFunction;
    Node* s_node;
};

class NodeView {
  public:
    NodeView() : m_source(0) {}
    virtual ~NodeView(){}
    virtual void SetSource(NodeSource *source) = 0;
    virtual void UpdatePreviewFrame(const char* buf, int bufsize,
        int width, int height, int orientation) = 0;
    virtual NodeSource* Source() { return m_source; }
    virtual bool IsCamera() { return false; }

  protected:
    NodeSource *m_source;
};

/**
 * Interface to be implemented by modules interested in getting events
 * from node
 */
class NodeModule {
  public:
    virtual ~NodeModule() {}
    virtual ModuleId Module() = 0;
};

/*
 * Interface to be implemented by node clients e.g. browser
 */
class NodeClient {
  public:
    virtual ~NodeClient() {}
    virtual void HandleNodeEvent(NodeEvent*) = 0;
    virtual void OnDelete() = 0;
    virtual const char* url() = 0;
    virtual NodeView* Unwrap(v8::Handle<v8::Object> object) = 0;
    virtual v8::Handle<v8::Value> CreatePreviewNode(NodeSource *) = 0;
    virtual v8::Handle<v8::Value> CreateArrayBuffer(void *buf, int size) = 0;
    virtual const char* GetEnvironmentProperty(const char *prop, bool ) = 0;
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
class Node {
  public:
    /**
     * Creates a node instance, initializes it and loads it.
     * initialization includes creation of libev watchers, and creating the process object
     * load bootstraps node by reading/compiling/executing the builtin modules
     * (node.js, buffer.js, module.js, fs.js etc)
     * @param client Handle to browser, used for sending events
     */
    Node(NodeClient *client);

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
    static void Initialize(void (*clientcb)(), bool isBrowser, const char* moduleRootPath, struct ev_loop *hostLoop = 0);

    /**
     * Node client
     * @return returns the client handle, used to send events to the client
     */
    NodeClient* client() { return m_client; }

    /**
     * Invoked by the client on the main thread to process pending libev events
     * (internally calls ev_invoke_pending)
     */
    static void InvokePending();

    /**
     * Called by the browser when the current page goes out of focus, either
     * user has switched to a new tab or browser has gone to background
     */
    void Pause();

    /**
     * Called by the browser when the page associated with this node comes in focus, either
     * user has switched to this tab or browser has come to foreground with this tab in focus
     */
    void Resume();

    /**
     * Get handle to loadModule function in the current node context
     * this will be set as window.navigator.loadModule by the client
     * @return handle to the loadModule function in node context
     */
    v8::Handle<v8::Function> GetLoadModule();
    v8::Handle<v8::Function> GetRequire();

    /**
     * This invokes loadModule in node's context. This function also
     * places try catch blocks around the call to catch any errors for test reporting
     * @param args module to load
     * @return Reference to the module after loading it
     */
    v8::Handle<v8::Value> LoadModule(const v8::Arguments& args);

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
     * Retreive the node instance from the process object, stored in slot 0
     * @param process process object
     * @return node instance for the given process
     */
    static Node* GetNode(v8::Handle<v8::Object> process);

    /**
     * Executes the given string in node context
     * @param source source code to be compiled/run
     * @param filename filename associated with the source code for reporting errors
     * @return return value after evaluating the source
     */
    static v8::Local<v8::Value> ExecuteString(v8::Handle<v8::String> source,
        v8::Handle<v8::Value> filename);

    /**
     * Test API
     * Called by the client after the node indicates an update in test api
     * reports status for one or more tests
     */
    static void FatalException(v8::TryCatch &try_catch, bool isHost = false);
    static void DisplayExceptionLine(v8::TryCatch &try_catch);
    void ReportException(v8::TryCatch &try_catch, bool show_line);

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

    /**
     * structure used to register a native module to node,
     * the module could be a builtin (e.g. node_fs) or dynamic .so)
     */
    struct node_module_struct {
      int version;
      void *dso_handle;
      const char *filename;
      void (*register_func) (v8::Handle<v8::Object> target);
      const char *modname;
    };

    static node_module_struct*
      get_builtin_module(const char *name);

    // js/native traces
    static void PrintJSStackTrace(android_LogPriority pri = ANDROID_LOG_WARN);

#ifndef ANDROID // only supported on desktop for now..
    static void PrintNativeStackTrace(android_LogPriority pri = ANDROID_LOG_WARN);
    static std::string AddressToString(void *addr);
#endif

    // returns host loop, e.g. in non-browser environment
    static struct ev_loop* HostLoop();

    // return exit code for test.py
    static int ExitCode();

  private:
    void Init();
    void SetupProcessObject();
    void Load(); // load all builtin modules in current context
    void Tick();

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

    // NodeClient (e.g. webkit node proxy)
    NodeClient *m_client;

    friend class NodeStatic;
};

#define NODE_PSYMBOL(s) Persistent<String>::New(String::NewSymbol(s))

/* Converts a unixtime to V8 Date */
#define NODE_UNIXTIME_V8(t) Date::New(1000*static_cast<double>(t))
#define NODE_V8_UNIXTIME(v) (static_cast<double>((v)->NumberValue())/1000.0);

#define NODE_DEFINE_CONSTANT(target, constant)                            \
  (target)->Set(String::NewSymbol(#constant),                             \
                Integer::New(constant),                                   \
                static_cast<PropertyAttribute>(ReadOnly|DontDelete))

#define NODE_SET_METHOD(obj, name, callback)                              \
  obj->Set(String::NewSymbol(name),                                       \
           FunctionTemplate::New(callback)->GetFunction())

#define NODE_SET_PROTOTYPE_METHOD(templ, name, callback)                  \
do {                                                                      \
  v8::Local<Signature> __callback##_SIG = Signature::New(templ);          \
  v8::Local<FunctionTemplate> __callback##_TEM =                          \
    FunctionTemplate::New(callback, v8::Handle<v8::Value>(),              \
                          __callback##_SIG);                              \
  templ->PrototypeTemplate()->Set(String::NewSymbol(name),                \
                                  __callback##_TEM);                      \
} while (0)

/**
 * When this version number is changed, node.js will refuse
 * to load older modules.  This should be done whenever
 * an API is broken in the C++ side, including in v8 or
 * other dependencies
 */
#define NODE_MODULE_VERSION (1)
#define NODE_STANDARD_MODULE_STUFF \
   NODE_MODULE_VERSION, NULL, __FILE__

#ifndef NODE_STRINGIFY
#define NODE_STRINGIFY(n) NODE_STRINGIFY_HELPER(n)
#define NODE_STRINGIFY_HELPER(n) #n
#endif

#define NODE_MODULE(modname, regfunc)                    \
  node::Node::node_module_struct modname ## _module =    \
  {                                                      \
      NODE_STANDARD_MODULE_STUFF,                        \
      regfunc,                                           \
      NODE_STRINGIFY(modname)                            \
  };

#define NODE_MODULE_DECL(modname) \
  extern node::Node::node_module_struct modname ## _module;

v8::Local<v8::Value> ErrnoException(int errorno, const char *syscall = NULL,
    const char *msg = "", const char *path = NULL);

void SetErrno(uv_err_code code);

}  // namespace node

#endif // __cplusplus
#endif
