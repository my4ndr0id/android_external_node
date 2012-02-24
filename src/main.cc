/*
 * Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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

#include "string.h"
#include "stdlib.h"
#include "node.h"
#include "unistd.h"

using namespace v8;
using namespace node;
using namespace std;

class NodeProxy : public NodeClient {
  public:
    NodeProxy() : m_node(0) {}
    ~NodeProxy();

    void runInNodeContext(const char *module);
    void handleNodeEvents();

    // NodeClient interface
    void HandleNodeEvent(NodeEvent *) {}
    static void HandleInvokePending();
    void OnDelete() {}
    const char* url() { return ""; }
    virtual NodeView* Unwrap(v8::Handle<v8::Object> object) { return 0; }
    virtual Handle<Value> CreatePreviewNode(NodeSource* ) { return Handle<Value>(); }
    virtual Handle<Value> CreateArrayBuffer(void*, int size) {return Handle<Value>();}
    virtual const char* GetEnvironmentProperty(const char *prop, bool type) {return "";};

  private:
    Node *m_node;
    Persistent<Context> m_context;
    bool m_testDone;
};

class Host {
  public:
    static Host* instance() {
      if (!s_instance) {
        s_instance = new Host();
      }
      return s_instance;
    }

    int init(int argc, char *argv[]);
    void parseOpt(int argc, char **argv);
    void handleNodeEvents();

    static void handleInvokePendingEvThread();
    static void handleInvokePendingMainThreadAsyncCb(EV_P_ ev_async *w, int revents);
    static void InvokePendingCB(EV_P);

    // accessors
    struct ev_loop* loop() { return s_loop; }
    vector<Node*>* nodes() { return &s_nodes; }

  private:
    struct ev_loop* s_loop;
    ev_async s_asyncInvokePending;
    vector<Node*> s_nodes;

    // singleton host instance
    static Host* s_instance;
};

Host* Host::s_instance = 0;
#define si() Host::instance()

static inline v8::Local<v8::String> v8_str(const char* x) {
  return v8::String::New(x);
}

void Host::handleInvokePendingEvThread() {
  NODE_LOGV("HOST: %s, ev_async_send to host loop", __FUNCTION__);
  ev_async_send(si()->s_loop, &si()->s_asyncInvokePending);
}

void Host::handleInvokePendingMainThreadAsyncCb(EV_P_ ev_async *w, int revents) {
  NODE_LOGV("handleInvokePendingMainThreadAsyncCb");
  Node::InvokePending();
}

void Host::InvokePendingCB(EV_P) {
  ev_invoke_pending(si()->s_loop);
  if (ev_activecnt(si()->s_loop) <= 1 && ev_activecnt(ev_default_loop()) <= 1) {
    NODE_LOGV("All loops are Empty: Exiting -> main_loop: %d ev_loop %d",
        ev_activecnt(si()->s_loop), ev_activecnt(ev_default_loop()));

    // send idle events to all active nodes..
    vector<Node* >::iterator it = si()->s_nodes.begin();
    for (;it != si()->s_nodes.end(); it++) {
      (*it)->EmitEvent("idle");
    }
    ev_break(si()->s_loop);
  }
}

void Host::handleNodeEvents() {
  NODE_LOGV("HOST: loop start");
  ev_set_invoke_pending_cb(s_loop, InvokePendingCB);
  ev_run(s_loop, 0);
  NODE_LOGV("HOST: loop end");
}

void NodeProxy::runInNodeContext(const char *module) {
  NODE_LOGV("runInNodeContext, %s", module);

  HandleScope scope;
  if (m_context.IsEmpty()) {
    NODE_LOGV("runInNodeContext, creating Host context");
    m_context = Context::New();
    Context::Scope cscope(m_context);

    // create node instance and populate navigator.loadModule
    m_node = new Node(this);
    si()->nodes()->push_back(m_node);
    Handle<Object> navigator = Object::New();
    navigator->Set(v8::String::New("loadModule"), m_node->GetLoadModule());
    m_context->Global()->Set(v8::String::New("navigator"), navigator);
  }

  Context::Scope csope(m_node->context());

  TryCatch try_catch;
  Local<Function> require = Local<Function>::New(m_node->GetRequire());
  Local<Value> args[1] = { String::New(module) };
  require->Call(m_node->context()->Global(), 1, args);
  if (try_catch.HasCaught()) {
    Node::FatalException(try_catch, true);
  }
}

NodeProxy::~NodeProxy() {
  NODE_LOGV("~NodeProxy: %p", this);
  if (m_node) {
    delete m_node;
  }
}

int Host::init(int argc, char *argv[]) {
  V8::SetFlagsFromCommandLine(&argc, argv, false);

  // create the host loop
  s_loop = ev_loop_new();

  // setup node
  char cwd[256];
  getcwd(cwd, 256);
  NODE_LOGI("cwd = %s", cwd);
  Node::Initialize(
      Host::handleInvokePendingEvThread, // InvokePending callback
      false, // not a browser
      cwd, // download root dir
      s_loop);

  // start the host watchers..
  ev_async_init(&s_asyncInvokePending, handleInvokePendingMainThreadAsyncCb);
  ev_async_start(s_loop, &s_asyncInvokePending);

  NodeProxy proxy;
  for (int i = 1; i <= argc-1; i++ ) {
    const char *arg = argv[i];
    if (!arg || arg[0] == '-') {
      continue;
    }
    NODE_LOGI("main, running test %s", arg);
    proxy.runInNodeContext(arg);
    si()->handleNodeEvents();
  }
  return 0;
}

int main(int argc, char *argv[]) {
  Host::instance()->init(argc, argv);
  return Node::ExitCode();
}

