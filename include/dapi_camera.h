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

#ifndef DAPI_CAMERA_H
#define DAPI_CAMERA_H

namespace dapi {

class INodeClientCameraView;

/**
 * @defgroup camera
 * Interfaces used by the camera module in the node and the HTML VNode element
 * in webkit.
 * @{
 */

/**
 * Represents a camera source, a native object that interacts with camera stack.
 * This object is owned by the camera module/camera js object that created it.
 */
class INodeCameraSource {
  public:
    INodeCameraSource(){}
    virtual ~INodeCameraSource(){}

    /**
     * Attached a view (vnode) to the source. We also get a surface texture. This is usually
     * triggered by the preview element (vnode) when it gets attached in the DOM. The source starts
     * preview after its attached has a surface texture.
     */
    virtual void attach(INodeClientCameraView*, void* texture) = 0;

    /**
     * Detaches view from the source, any camera activity is stopped, but the object is not destroyed.
     * we could be reattached to the same or a different view.
     */
    virtual void detach() = 0;

    /**
     * Camera hardware needs a surface texture (a android display object) to start the preview
     * The camera source holds off starting preview till it gets a texture from the client.
     * The texture is available to the client (vnode) usually when it gets attached to the DOM and
     * after the initial paint.
     */
    virtual void setPreviewTexture(void* texture) = 0;
};

/**
 * Interface for the camera source to interact with the view (vnode) element.
 * currently unused since the interaction is oneway (vnode->source)
 */
class INodeClientCameraView {
  public:
    INodeClientCameraView(){}
    virtual ~INodeClientCameraView(){}
};

/**
 * Exposes api for camera module like creating a preview html element,
 * an array buffer out of a raw picture data etc
 */
class INodeClientCamera {
	public:
    INodeClientCamera(){}
    virtual ~INodeClientCamera(){}

    /**
     * creates a new preview node (a HTML element) that could be appended
     * to DOM for display. The api provides a camera source (a node js object
     * that sources camera frames) that is bound to the created HTML element
     * which can then interact with a standard interface defined by INodeCameraSource
     * and INodeClientCameraView classes resp.
     */
    virtual v8::Handle<v8::Value> createPreviewNode(INodeCameraSource*) = 0;

    /**
     * Creates a ArrayBuffer from the raw image data. We use webkit api
     * currently since node does not have a ArrayBuffer implementation
     */
    virtual v8::Handle<v8::Value> createArrayBuffer(void *buf, int size) = 0;
};

/*@}*/

}

#endif
