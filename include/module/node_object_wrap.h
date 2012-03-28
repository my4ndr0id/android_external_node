// Copyright Joyent, Inc. and other Node contributors.
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

#ifndef object_wrap_h
#define object_wrap_h

#include <v8.h>
#include <assert.h>

#include "dapi_log.h"
#include "dapi_inode.h"
#include "dapi_core.h"

namespace node {
using namespace dapi;

class ObjectWrap {
 public:
  ObjectWrap ( ) {
    refs_ = 0;

    // add ourself to the current node to enable cleanup of watchers at node
    // deletion. The assumption is that all watchers are controlled by a
    // native object implementing the objectwrap interface
    m_inode = INode::getCurrentINode();
    NODE_LOGV("ObjectWrap(%p), inode(%p)", this, m_inode);
    INodeCore *core;
    INode::getCurrentINode()->queryInterface(INTERFACE_CORE, (void**)&core);
    core->addWatcherWrap(this);
  }


  virtual ~ObjectWrap ( ) {
    if (!handle_.IsEmpty()) {
      // This is no more valid since we can destroy the native instance when the node
      // instance is deleted and no more rely on the JS GC cycle to delete native objects
      // assert(handle_.IsNearDeath());
      handle_.ClearWeak();
      handle_->SetInternalField(0, v8::Undefined());
      handle_.Dispose();
      handle_.Clear();
    }

    // This could be called directly by the node cleanup, module itself
    // or during GC
    NODE_LOGV("~ObjectWrap(%p), inode(%p)", this, m_inode);
    INodeCore *core;
    m_inode->queryInterface(INTERFACE_CORE, (void**)&core);
    core->removeWatcherWrap(this);
  }


  template <class T>
  static inline T* Unwrap (v8::Handle<v8::Object> handle) {
    assert(!handle.IsEmpty());
    assert(handle->InternalFieldCount() > 0);
    return static_cast<T*>(handle->GetPointerFromInternalField(0));
  }


  v8::Persistent<v8::Object> handle_; // ro

 protected:
  inline void Wrap (v8::Handle<v8::Object> handle) {
    assert(handle_.IsEmpty());
    assert(handle->InternalFieldCount() > 0);
    handle_ = v8::Persistent<v8::Object>::New(handle);
    handle_->SetPointerInInternalField(0, this);
    MakeWeak();
  }


  inline void MakeWeak (void) {
    handle_.MakeWeak(this, WeakCallback);
  }

  /* Ref() marks the object as being attached to an event loop.
   * Refed objects will not be garbage collected, even if
   * all references are lost.
   */
  virtual void Ref() {
    assert(!handle_.IsEmpty());
    refs_++;
    handle_.ClearWeak();
  }

  /* Unref() marks an object as detached from the event loop.  This is its
   * default state.  When an object with a "weak" reference changes from
   * attached to detached state it will be freed. Be careful not to access
   * the object after making this call as it might be gone!
   * (A "weak reference" means an object that only has a
   * persistant handle.)
   *
   * DO NOT CALL THIS FROM DESTRUCTOR
   */
  virtual void Unref() {
    assert(!handle_.IsEmpty());
    assert(!handle_.IsWeak());
    assert(refs_ > 0);
    if (--refs_ == 0) { MakeWeak(); }
  }


  int refs_; // ro
  INode* m_inode;


 private:
  static void WeakCallback (v8::Persistent<v8::Value> value, void *data) {
    ObjectWrap *obj = static_cast<ObjectWrap*>(data);
    assert(value == obj->handle_);
    assert(!obj->refs_);
    assert(value.IsNearDeath());
    delete obj;
  }
};

} // namespace node
#endif // object_wrap_h
