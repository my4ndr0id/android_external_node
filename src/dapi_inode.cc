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

#include "node.h"
#include <map>
#include <set>

namespace dapi {
using namespace node;
using namespace v8;
using namespace std;

class INodePrivate {
  public:
    INodePrivate(INode *inode) : m_inode(inode) {}
    void registerDelegate(Interface,
        bool (*queryInterface)(Interface, void** object, void* data), void* data);
    bool queryInterface(Interface interface, void** object);
    void registerInterface(Interface interface, void* object);

  private:
    struct delegateInfo {
      bool (*queryInterface)(Interface, void**object, void* data);
      void* data;
    };
    map<Interface, struct delegateInfo> m_delegateMap;
    map<Interface, void*> m_interfaceMap;
    INode *m_inode;
};

void INodePrivate::registerDelegate(Interface interface,
    bool (*queryInterface)(Interface, void** object, void* data), void* data) {
  struct delegateInfo info = {queryInterface, data};
  m_delegateMap[interface] = info;
}

void INodePrivate::registerInterface(Interface interface, void* object) {
  m_interfaceMap[interface] = object;
}

bool INodePrivate::queryInterface(Interface interface, void** object) {
  *object = 0;

  // check the interface map
  map<Interface, void*>::iterator it = m_interfaceMap.find(interface);
  if (it != m_interfaceMap.end()) {
    *object = it->second;
    return true;
  }

  // check delegate map
  map<Interface, struct delegateInfo>::iterator it_ = m_delegateMap.find(interface);
  if (it_ != m_delegateMap.end()) {
    struct delegateInfo info = it_->second;
    return (info.queryInterface)(interface, object, info.data);
  }

  NODE_LOGE("Node interface %d not implemented", interface);
  NODE_ASSERT_REACHABLE();
  return false;
}

INode::INode(INodeClient *client)
  : m_client(client)
  , m_private(new INodePrivate(this))
{
  m_node = new Node(this);
  NODE_LOGI("Inode::Inode(%p), node(%p)", this, m_node);
  m_node->Init();
}

INode::~INode() {
  NODE_LOGI("~Inode::Inode(%p), node(%p)", this, m_node);
  delete m_node;
  delete m_private;
}

void INode::initialize(void (*cb)(), bool isBrowser, const char* moduleRootPath) {
  Node::Initialize(cb, isBrowser, moduleRootPath);
}

void INode::invokePending() {
  Node::InvokePending();
}

bool INode::queryInterface(Interface interface, void** object) {
  return m_private->queryInterface(interface, object);
}

void INode::registerInterface(Interface interface, void* object) {
  m_private->registerInterface(interface, object);
}

void INode::registerDelegate(Interface interface,
    bool (*queryInterface)(Interface, void** object, void* data), void* data) {
  m_private->registerDelegate(interface, queryInterface, data);
}

INode* INode::getCurrentINode() {
  HandleScope scope;

  NODE_ASSERT(Context::InContext());
  // global object is a shadow object, the prototype holds the real global
  return getINodeFromObject(Context::GetCurrent()->Global()->GetPrototype()->ToObject());
}

INode* INode::getINodeFromObject(v8::Handle<v8::Object> o) {
  HandleScope scope;

  INode *inode = static_cast<INode*>(o->GetPointerFromInternalField(0));
  NODE_ASSERT(inode);

  return inode;
}

int INode::exitCode() {
	return  node::Node::ExitCode();
}

void INode::reportException(v8::TryCatch &try_catch) {
  Node::ReportException(try_catch, true);
}

}
