/*
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <v8.h>
#include <vector>

#define LOG_TAG_NODE "NodeProxy"
#include "node.h"

//#define LOADMODULE_ASYNC 1 // Tests downloader, much slower

using namespace v8;
using namespace node;
using namespace std;

enum {
  PENDING = 0,
  DONE = 1
};

class NodeProxy : public NodeClient {
  public:
    NodeProxy() : m_node(0) {}
    NodeProxy(const char* test);
    ~NodeProxy();

    void runTest(const char *test);
    static int* pipefd(){ return s_pipefd; }
    void processNodeEvents();

    // NodeClient interface
    void HandleNodeEvent(NodeEvent *) {}
    static void HandleInvokePending();
    void OnDelete();
    void OnTestDone();
    string url() { return ""; }
    virtual NodeView* Unwrap(v8::Handle<v8::Object> object) { return 0; }
    virtual Handle<Value> CreatePreviewNode(NodeSource* ) { return Handle<Value>(); }
    virtual Handle<Value> CreateArrayBuffer(void*, int size) {return Handle<Value>();}
    virtual std::string GetEnvironmentProperty(const char *prop, bool type) {return "";};

    static Handle<Value> loadModuleSyncCallback(const Arguments& args);
    static Handle<Value> loadModuleCallback(const Arguments& args);
    void ClearNode();

  private:
    Node *m_node;
    const char *m_test;
    Persistent<Context> m_context;
    static int s_pipefd[2];
    bool m_testDone;
};

bool s_reload = false;
bool s_multiple = false;
bool s_parallel = false;
int s_useMultipleContexts = 0;
int s_runTestsInNewNodeParallely = 0;
int s_runTestsInNewNodeSerially = 0;
int s_runTestsInSingleNodeSerially = 0;
vector<Node*> s_nodes;

int s_mainThreadId = 0;
int NodeProxy::s_pipefd[2];
bool isAndroid = false;

void NodeProxy::HandleInvokePending() {
  NODE_ASSERT(s_mainThreadId != gettid());
  int ret = write(NodeProxy::s_pipefd[1], "0", 2);
  NODE_LOGM("%s, ** write to the pipe - PENDING", __FUNCTION__);
}

void NodeProxy::ClearNode() {
  NODE_LOGV("%s, s_nodes(%d)", __FUNCTION__, s_nodes.size());
  bool found = false;
  for (vector<Node* >::iterator it = s_nodes.begin();
      it != s_nodes.end(); it++) {
    if (*it == m_node) {
      s_nodes.erase(it);
      found = true;
      break;
    }
  }
  NODE_ASSERT(found);
}

void NodeProxy::OnDelete() {
  NODE_LOGI("%s, node instance %p cleared from proxy", __FUNCTION__, m_node);
  ClearNode();
  m_node = 0;
  m_context.Dispose();
  m_context.Clear();
}

void NodeProxy::OnTestDone() {
  NODE_LOGV("%s, test done for node (%p)", __FUNCTION__, m_node);
  // This is called on the main
  int ret = write(NodeProxy::s_pipefd[1], "2", 2);
}

const char* ModuleCode(const char *test) {
#ifdef LOADMODULE_ASYNC
  std::string code = "\
    navigator.loadModule('test', function(testmod) { \
        testmod.runTest('" + std::string(test) + "'); \
        });";
#else
  std::string code = "\
    var testmod = navigator.loadModuleSync('test'); \
        testmod.runTest('" + std::string(test) + "');";
#endif

  return strdup(code.c_str());
}

// Reads a file into a v8 string.
char* ReadFile(const char* name) {
  FILE* file = fopen(name, "rb");

  if (!file) {
    NODE_LOGE("%s, %s", __FUNCTION__, strerror(errno));
    exit(0);
  }

  fseek(file, 0, SEEK_END);
  int size = ftell(file);
  rewind(file);

  char* chars = new char[size + 1];
  chars[size] = '\0';
  for (int i = 0; i < size;) {
    int read = fread(&chars[i], 1, size - i, file);
    i += read;
  }
  fclose(file);
  return chars;
}

char* BrowserCode(const char *test) {
  NODE_LOGI("Test STARTED (simulated browser): %s", test);
  char *jssource = ReadFile(test);
  return jssource;
}

void NodeProxy::processNodeEvents() {
  NODE_LOGF();
  HandleScope scope;

  m_testDone = false;
  while (1) {
    char s[2] = "";
    NODE_LOGV("HANDSHAKE: %s, ** blocked on read", __FUNCTION__);
    int nbytes = read(s_pipefd[0], s, sizeof(s));
    NODE_LOGV("HANDSHAKE: %s, ** out of read received %s", __FUNCTION__, s[0] == '0' ? "INVOKE_PENDING" : "EV_DONE");
    if (nbytes == -1) {
      NODE_LOGV("%s, no data to read from pipe non blocking", __FUNCTION__);
      NODE_LOGFR();
      return;
    }
    else if (s[0] == '0') {
      NODE_LOGV("HANDSHAKE: %s, ** wakeup handling pending callbacks", __FUNCTION__);
      Node::InvokePending();
      if (Node::CheckTestStatus()) {
        NODE_LOGV("HANDSHAKE: %s, INVOKE_PENDING returned done processing, returning", __FUNCTION__);
        NODE_LOGFR();
        return;
      }
    } else if (s[0] == '2') {
      // NODE_EVENT_TEST_DONE
      if (s_nodes.size() <= 1) {
        NODE_LOGFR();
        return;
      }
    } else {
      NODE_LOGE("%s, **error** bad data on the pipe", __FUNCTION__);
    }
  }
  NODE_LOGFR();
}

Handle<Value> NodeProxy::loadModuleSyncCallback(const Arguments& args) {
  NODE_LOGF();

  HandleScope scope;
  if (args[0].IsEmpty() || !args[0]->IsString()) {
    return ThrowException(String::New("Invalid arguments to loadModule"));
  }

  Handle<Object> navigator = args.Holder()->ToObject();
  NodeProxy *np = static_cast<NodeProxy*>(navigator->GetPointerFromInternalField(0));
  np->m_node = new Node(np);
  s_nodes.push_back(np->m_node);
  Handle<Function> loadModuleSync = np->m_node->GetLoadModuleSync();
  NODE_ASSERT(!loadModuleSync.IsEmpty() && loadModuleSync->IsFunction());
  if (!loadModuleSync.IsEmpty() && loadModuleSync->IsFunction()) {
    navigator->Set(v8::String::New("loadModuleSync"), loadModuleSync);
    return np->m_node->LoadModuleSync(args);
  } else {
    return Undefined();
  }
}

Handle<Value> NodeProxy::loadModuleCallback(const Arguments& args) {
  NODE_LOGF();

  HandleScope scope;
  if (args[0].IsEmpty() || !args[0]->IsString()) {
    return ThrowException(String::New("Invalid arguments to loadModule"));
  }

  Handle<Object> navigator = args.Holder()->ToObject();
  NodeProxy *np = static_cast<NodeProxy*>(navigator->GetPointerFromInternalField(0));
  np->m_node = new Node(np);
  s_nodes.push_back(np->m_node);
  Handle<Function> loadModule = np->m_node->GetLoadModule();
  NODE_ASSERT(!loadModule.IsEmpty() && loadModule->IsFunction());
  if (!loadModule.IsEmpty() && loadModule->IsFunction()) {
    navigator->Set(v8::String::New("loadModule"), loadModule);
    return np->m_node->LoadModule(args);
  } else {
    return Undefined();
  }
}

void NodeProxy::runTest(const char *test) {
  NODE_LOGI("%s, %s", __PRETTY_FUNCTION__, test);
  m_test = test;

  // first time create the context and navigator.loadModule
  HandleScope scope;
  if (m_context.IsEmpty()) {
    NODE_LOGV("%s, Creating Proxy Context", __FUNCTION__);
    m_context = Context::New();
    Context::Scope cscope(m_context);
    Local<FunctionTemplate> navigator_template = FunctionTemplate::New();
    navigator_template->InstanceTemplate()->SetInternalFieldCount(1);
    Handle<Object> navigator = navigator_template->GetFunction()->NewInstance();
    m_context->Global()->Set(v8::String::New("navigator"), navigator);
#ifdef LOADMODULE_ASYNC
    navigator->Set(v8::String::New("loadModule"),
        FunctionTemplate::New(loadModuleCallback)->GetFunction());
#else
    navigator->Set(v8::String::New("loadModuleSync"),
        FunctionTemplate::New(loadModuleSyncCallback)->GetFunction());
#endif
    navigator->SetPointerInInternalField(0, this);
  }

  Context::Scope cscope(m_context);
  // if we are simulating browser, we behave just as you would copy the contents of
  // the test file in a html and run in browser
  // otherwise (default behavior), we run as if the contents of the test file are in
  // a module and we are loading that module through loadModule from a HTML
  const char *code = getenv("BROWSER") ? BrowserCode(test) : ModuleCode(test);
  NODE_LOGV("Executing code in NodeProxy context: %s", code);
  Local<Value> result = Node::ExecuteString(v8::String::New(code),
      getenv("BROWSER") ? v8::String::New(m_test) : v8::String::New("<load stub>"));
  // This should happen only in browser code, since the other case is static
  if (result.IsEmpty()) {
    NODE_LOGE("Test **FAILED: %s", m_test);
  }
}

NodeProxy::NodeProxy(const char *test)
  : m_node(0)
  , m_testDone(false)
{
  NODE_LOGV("%s: %p", __FUNCTION__, this);
  runTest(test);
}

NodeProxy::~NodeProxy() {
  NODE_LOGV("%s: %p", __FUNCTION__, this);
  if (m_node) {
    delete m_node;
  }
}

int main(int argc, char *argv[]) {
  for (int i = 1; i < argc; i++){
    if (strstr(argv[i], "--wait")){
      sleep(15);
    }

    if (strcmp(argv[i], "-r") == 0) {
      s_reload = true;
    }

    if (strcmp(argv[i], "-m") == 0) {
      s_multiple = true;
    }
  }

  V8::SetFlagsFromCommandLine(&argc, argv, false);

#ifdef ANDROID
  isAndroid = true;
#endif

  // should we create multiple nodes
  if (getenv("MULTIPLE")) {
    s_multiple = true;
  }

  // should we run tests parallely
  s_parallel = getenv("PARALLEL") ? true : false;

  if (s_multiple && s_parallel) {
    s_runTestsInNewNodeParallely = true;
  } else if (s_multiple && !s_parallel) {
    s_runTestsInNewNodeSerially = true;
  } else {
    s_runTestsInSingleNodeSerially = true;
  }

  if (getenv("RELOAD")) {
    s_reload = true;
  }
  s_mainThreadId = gettid();
  int ret = pipe(NodeProxy::pipefd());
  assert(ret == 0);

  Node::Initialize(NodeProxy::HandleInvokePending, false, isAndroid ? "/data/data/com.android.browser" : getenv("PWD"));
  int nTests = argc - 1;
  if (s_useMultipleContexts) {
    NodeProxy *tests = new NodeProxy[nTests];
    for (int i = 1; i <= nTests; i++ ){
      tests[i-1].runTest(argv[i]);
    }
    //NodeProxy::processNodeEvents();
  }
  // create one node for each test.., all tests run parallely
  else if (s_runTestsInNewNodeParallely) {
    NODE_LOGE("** runTestInNewNodeParallely");
    NodeProxy *proxy = new NodeProxy[nTests];
    for (int i = 1; i <= nTests; i++ ) {
      NODE_LOGI("main, running test %s", argv[i]);
      proxy[i-1].runTest(argv[i]);
    }
    proxy[0].processNodeEvents();
    NODE_LOGI("main, deleting NodeProxy for all tests");
    delete []proxy;
  }
  // create one node and run each test in it
  else if (s_runTestsInSingleNodeSerially) {
    NODE_LOGE("** s_runTestsInSingleNodeSerially");
    NodeProxy proxy;
    do {
      for (int i = 1; i <= nTests; i++ ) {
        if (argv[i][0] == '-') {
          continue;
        }
        NODE_LOGI("main, running test %s", argv[i]);
        proxy.runTest(argv[i]);
        proxy.processNodeEvents();
      }
      if (s_reload) {
        static int iterations = 0;
        NODE_LOGD("RELOAD, ITERATION : %d", ++iterations);
      }
    } while(s_reload);
  } else if (s_runTestsInNewNodeSerially) {
    NODE_LOGE("** s_runTestsInNewNodeSerially");
    do {
      for (int i = 1; i <= nTests; i++ ) {
        if (argv[i][0] == '-') {
          continue;
        }
        NODE_LOGI("main, running test %s", argv[i]);
        NodeProxy proxy;
        proxy.runTest(argv[i]);
        proxy.processNodeEvents();
      }
      if (s_reload) {
        static int iterations = 0;
        NODE_LOGD("RELOAD, ITERATION : %d", ++iterations);
      }
    } while (s_reload);
  }

  return 0;
}
