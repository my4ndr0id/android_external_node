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

#ifndef DAPI_CORE_H
#define DAPI_CORE_H

namespace dapi {

/**
 * Interface to the node core api
 */
class INodeCore {
  public:
    INodeCore() {}
    virtual ~INodeCore() {}

    /**
     * Invokes "loadModule" javascript api in node's context. This api loads
     * the module asynchonously including downloading the module if required.
     *
     * @param args module to load, success callback and optional error callback
     * @return none, the module reference is returned in the successcb
     */
    virtual v8::Handle<v8::Value> loadModule(v8::Handle<v8::Value>* args) = 0;

    /**
     * Invokes "require" javascript api in node's context. This api loads the
     * module synchronously. To be used in offtarget test environment only.
     *
     * @param args module to load
     * @return reference to module
     */
    virtual v8::Handle<v8::Value> require(v8::Handle<v8::Value>* args) = 0;

    /**
     * Get the inode v8 context
     */
    virtual v8::Handle<v8::Context> inodeContext() = 0;

    /**
     * Get the inode client (webkit) v8 context
     */
    virtual v8::Handle<v8::Context> inodeClientContext() = 0;
};

}
#endif
