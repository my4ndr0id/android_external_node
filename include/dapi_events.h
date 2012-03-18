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

#ifndef NODE_EVENTS_H
#define NODE_EVENTS_H

namespace dapi {

/**
 * @defgroup events
 * Handle system events from the client
 * @{
 */

/**
 * Handle events from the client
 */
class INodeEvents {
  public:
    INodeEvents(){}
    virtual ~INodeEvents(){}

    /**
     * Handle pause event from the client, invoked when the tab goes out of focus
     */
    virtual void onPause() = 0;

    /**
     * Handle pause event from the client, invoked when the tab gets focus
     */
    virtual void onResume() = 0;

    /**
     * Handle idle event, invoked when there are is pending activity in the event loop.
     * Currently used in the test mode to detect when we are done.
     */
    virtual void onIdle() = 0;
};

class INodeClientEvents {
  public:
    INodeClientEvents() {}
    virtual ~INodeClientEvents() {}

    /**
     * Invoked by inode when its deleted, this is currently used in tests
     * to simulate closing a page
     */
    virtual void onDelete() = 0;
};

/*@}*/
}

#endif
