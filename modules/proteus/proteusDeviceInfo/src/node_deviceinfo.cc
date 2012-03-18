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

#include <stdlib.h>
#include <string.h>
#include <dapi_module.h>

#ifdef ANDROID
#include <cutils/properties.h>
#endif

using namespace v8;
using namespace dapi;

class DeviceInfoUtil: node::ObjectWrap {
  private :
    INode *m_inode;
    char *m_downloadPath;
  public:
    INode *inode() { return m_inode; }

    static Persistent<FunctionTemplate> s_ct;

    DeviceInfoUtil(INode *inode, char* downloadPath) : m_inode(inode)
    {
      m_downloadPath = strdup(downloadPath);
    }

    ~DeviceInfoUtil(){
      free(m_downloadPath);
      m_inode = 0;
      m_downloadPath = 0;
    }

    static void InitDeviceInfo(Handle<Object> target){

      HandleScope scope;
      Local<FunctionTemplate> t = FunctionTemplate::New(New);

      s_ct = Persistent<FunctionTemplate>::New(t);
      s_ct->InstanceTemplate()->SetInternalFieldCount(1);
      s_ct->SetClassName(String::NewSymbol("createDeviceInfo"));

      NODE_SET_PROTOTYPE_METHOD(s_ct, "getSystemProp", GetSystemProp);
      NODE_SET_PROTOTYPE_METHOD(s_ct, "getEnvironmentProp", GetEnvironmentProp);

      target->Set(String::NewSymbol("createDeviceInfo"),s_ct->GetFunction());
    }

    static Handle<Value>  GetSystemProp(const Arguments& args)
    {
      HandleScope scope;
      bool accessPermission = false;

      if (args.Length() == 0)
          return v8::ThrowException(v8::String::New("Bad parameters"));

      if (args.Length() > 1 || *args[1] == NULL)
          return v8::ThrowException(v8::String::New("Bad parameters"));

      if (*args[0] == NULL)
          return v8::ThrowException(v8::String::New("Bad parameters"));

      String::AsciiValue property(args[0]->ToString());

#ifdef ANDROID
      char propertyValue[PROPERTY_VALUE_MAX];

      property_get(*property,propertyValue, NULL);
      NODE_LOGV("%s, GetSystemProp : Property %s  --> %s\n", __FUNCTION__, *property, propertyValue);
      return scope.Close(v8::String::New((char *)&propertyValue));
#else
      return scope.Close(v8::String::New("Desktop"));
#endif
    }

    static Handle<Value> GetEnvironmentProp(const Arguments& args)
    {
      HandleScope scope;
      bool accessPermission = false;

      if (args.Length() == 0)
          return v8::ThrowException(v8::String::New("Bad parameters"));

      if (args.Length() > 1 || *args[1] == NULL)
          return v8::ThrowException(v8::String::New("Bad parameters"));

      if (*args[0] == NULL)
          return v8::ThrowException(v8::String::New("Bad parameters"));

      DeviceInfoUtil* deviceInfoUtil = ObjectWrap::Unwrap<DeviceInfoUtil>(args.This());
      String::AsciiValue property(args[0]->ToString());
#ifdef ANDROID
      dapi::INodeClientWebView *webview;
      deviceInfoUtil->inode()->client()->queryInterface(dapi::INTERFACE_WEBVIEW, (void**)&webview);
      NODE_ASSERT(webview);
      std::string propertyValue = webview->getEnvironmentProperty(*property, false);
      NODE_LOGV("%s, GetEnvironmentProp : Property %s  --> %s\n", __FUNCTION__, *property, propertyValue.c_str());
      return v8::String::New((char *)propertyValue.c_str());
#else
      std::string path;
      path.append(deviceInfoUtil->m_downloadPath);
      path.append(*property);
      return scope.Close(v8::String::New(path.c_str()));
#endif
    }

    static Handle<Value> New(const Arguments& args)
    {
      HandleScope scope;
      String::AsciiValue downloadPath(args[1]->ToString());
      INode *inode = INode::getCurrentINode();
      NODE_LOGD("%s, node(%p)", __FUNCTION__, inode);
      DeviceInfoUtil* deviceInfoUtil = new DeviceInfoUtil(inode, *downloadPath);
      deviceInfoUtil->Wrap(args.This());
      return args.This();
    }
};


Persistent<FunctionTemplate> DeviceInfoUtil::s_ct;

// FIXME(proteus) need to fix the naming issue for static/dynamic modules
extern "C" void deviceinfo_init (Handle<Object> target) {
  HandleScope scope;
  DeviceInfoUtil::InitDeviceInfo(target);
}

NODE_MODULE(node_deviceinfo, deviceinfo_init);
