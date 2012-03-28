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

#ifndef DAPI_H_
#define DAPI_H_

/**
 * @mainpage HTML Device API interface guide
 * HTML Device API (DAPI) is a framework for exposing device features as javascript api in the web browser.
 * The framework is built on top of nodejs opensource framework and allows for such features
 * as evented non-blocking IO, expose services as Javascript API using commonJS specifications,
 * dynamic loading of modules, secure environment to run modules, user permission to access device
 * hardware etc.
 *
 * INode is a wrapper around standard nodejs api and embeds in the host environment. Each webpage gets
 * a inode and is fully isolated environment from other inodes in different webpage.
 *
 * INode uses a proxy on the webkit to call in to browser api. The lifecyle of INode and INodeProxy are
 * tied to the webpage.
 *
 * There are two sets of API exposed by dapi, api to interact with the browser and api to interact with the
 * dynamic modules. The APIs are designed to be source and binary compatible, so any exhancements need to
 * follow the binary compatiblity guildelines
 * http://techbase.kde.org/Policies/Binary_Compatibility_Issues_With_C++
 *
 * dapi.h is to be included by the webkit, dapi_module.h to be included by dynamic modules. dapi_module.h
 * includes dap.h and in addtion exposes the standard nodejs api to the modules (e.g. node_object_wrap, libev,
 * in addition to dapi)
 *
 * The ABI compatibility requirement is to allow a enhanced version of a module to work with deployed dapi
 * infrastructure. If a module enhancement/or a new module is not compatible, its NODE_VERSION should be bumped
 * up and will only work on a newer dapi infrastructure and this should be as infrequent as possible.
 *
 * To enable ABI compatibility, the dapi inode interface exposes a single api (getImplementation) to get a handle
 * to a particular service (on the node side and client side) e.g. camera needs a interface on the webkit side to
 * provide api such as creating a HTML view element, array buffer for the captured image, connect the camera
 * source (a node js object) and the view etc. An example of node side service is a INodeWebView which handles
 * events like pause/resume of the webview. The behavior of these interfaces can be enhanced/modified with out by
 * breaking ABI compatibility by using standard c++ inheritance/polymorphism techniques.
 */

#include "dapi_log.h"
#include "dapi_inode.h"
#include "dapi_core.h"
#include "dapi_events.h"
#include "dapi_audio.h"
#include "dapi_camera.h"
#include "dapi_permission.h"
#include "dapi_webview.h"

#endif
