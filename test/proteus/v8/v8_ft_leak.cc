#include <v8.h>
using namespace v8;

static void WeakCallback(Persistent<Value> value, void *data) {
  printf("WeakCallback for %s\n", (char *) data);
  value.Dispose();
}

static Handle<Value> callback(const Arguments& args) {
  return Handle<Value>();
}

Persistent<FunctionTemplate> templ;

void LeakContext() {
  {
    HandleScope scope;
    Persistent<Context> context = Context::New();
    Context::Scope cscope(context);

    templ = Persistent<FunctionTemplate>::New(FunctionTemplate::New());
    templ->Set(String::New("callback"), FunctionTemplate::New(callback)->GetFunction());

    Persistent<Object> global = Persistent<Object>::New(context->Global());
    global.MakeWeak((void*) "global", WeakCallback);
    context.MakeWeak((void*) "context", WeakCallback);

    // templ.Dispose();
  }
  v8::V8::LowMemoryNotification();
}

int main() {
  LeakContext();
}

