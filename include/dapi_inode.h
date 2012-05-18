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

#ifndef DAPI_INODE_H_
#define DAPI_INODE_H_

#include <vector>
#include <string>
#include <v8.h>

#define DAPIEXPORT __attribute__ ((visibility("default")))

namespace node {
  class Node;
}

namespace dapi {

/**
 * @defgroup inode
 * INode is a wrapper around standard nodejs api and embeds in the host environment.
 * Each webpage gets a inode and is fully isolated environment from other inodes in different webpage.
 * @{
 */

/*
 * Ids for various services exposed by inode.
 * The enums could be extended in later versions but existing
 * values should never change for binary compatibility constraints.
 */
typedef enum {
  INTERFACE_INVALID,
  INTERFACE_CORE,
  INTERFACE_EVENTS,
  INTERFACE_WEBVIEW,
  INTERFACE_AUDIO,
  INTERFACE_CAMERA,
  INTERFACE_PERMISSION,
  INTERFACE_JAVABRIDGE
} Interface;

/*
 * Interface implemented by the client (webkit)
 */
class INodeClient {
  public:
    INodeClient() {}
    virtual ~INodeClient() {}

    /**
     * Returns the client implementation for the specified interface
     */
    virtual bool queryInterface(Interface interface, void** object) = 0;
};

class INodePrivate;

/**
 * Represents a node instance, there is one for each browser context
 * Gets created when the webpage loads a module through navigator.loadModule
 */
class DAPIEXPORT INode {
  public:
    /**
     * Creates a node instance, associated with a browser frame/context
     * client - handle to the client/browser object for callbacks
     */
    INode(INodeClient *client);

    /**
     * Destroys the node instance, triggered by embedder when page is navigated out
     * Native objects are immediately destroyed, JS objects are reclaimed in next GC
     */
    ~INode();

    /**
     * Bootup node, create ev thread ..
     * isBrowser - client is browser or shell
     * moduleRootPath - base directory for installing/lookup downloaded modules
     */
    static void initialize(void (*clientcb)(), bool isBrowser, const char* moduleRootPath);

    /**
     * Invoked by the client on the main thread to process pending libev events
     * (internally calls ev_invoke_pending)
     */
    static void invokePending();

    /**
     * Returns the inode reference stores from the active v8 context/global object
     */
    static INode* getCurrentINode();

    /**
     * V8 provides for JS objects to carry native references. This allows
     * for JS objects to carry context, e.g from a global object, we could get
     * the inode reference and get to the webpage that embeds it. This api returns
     * the inode reference stored in the object (usually in slot 0)
     */
    static INode* getINodeFromObject(v8::Handle<v8::Object> o);

    /**
     * Get exit code of the process, used for test purposes
     */
    static int exitCode();

    /**
     * Report exception on the console
     * FIXME: Need to be propagated to the browser/debugger console
     */
    static void reportException(v8::TryCatch &try_catch);

    /**
     * This is the core bridge api that exposes services from the node. Intended to be
     * used by the webkit and dynamic modules. Static modules and internal node code can
     * use the services directly without going through this API. There is a similar api
     * on the client (webkit) side that exposes client services.
     * Get handle to object that implements the interface
     * @interface interface that is being queried
     * @object object that implements the interface, null if no object implements
     * @return true if there is object that implements the interface, false otherwise
     */
    bool queryInterface(Interface interface, void** object);

    /**
     * Register implementation delegate for a interface.
     * The delegate will be invoked when the client does queryInterface
     * for a specific interface.
     * The delegate is free to manage the lifecycle of the implementation
     * (e.g. could be created early/cached, created on demand etc)
     * @interface interface that this delegate implements
     * @delegate callback to be invoked
     * @data data that will be passed back in the callback, this is used to pass the context
     */
    void registerDelegate(Interface,
        bool (*queryInterface)(Interface, void** object, void* data), void* data);

    /**
     * Register implementation for a interface. This is useful to
     * register objects already available that implement the interface.
     * @interface interface this object implements
     * @imp object implementing the interface
     */
    void registerInterface(Interface interface, void* object);

    /**
     * Get the client (webkit inode proxy)
     */
    INodeClient* client() { return m_client; }

  private:
    // handle to internal node object
    node::Node *m_node;

    // handle to client (e.g. webkit node proxy)
    INodeClient *m_client;

    // private data
    INodePrivate *m_private;

    // let node access inode internals
    friend class node::Node;
};

/*@} */

}

#endif
