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

#ifndef NODE_LOG_H
#define NODE_LOG_H

extern "C" __attribute__ ((visibility("default")))
int DAPILog(int prio, const char *tag, const char *fmt, ...);

typedef enum {
  DAPI_LOG_UNKNOWN = 0,
  DAPI_LOG_DEFAULT,    /* only for SetMinPriority() */
  DAPI_LOG_VERBOSE,
  DAPI_LOG_DEBUG,
  DAPI_LOG_INFO,
  DAPI_LOG_WARN,
  DAPI_LOG_ERROR,
  DAPI_LOG_FATAL,
  DAPI_LOG_SILENT,     /* only for SetMinPriority(); must be last */
} DAPILogPriority;

#ifndef LOG_TAG_NODE
#define LOG_TAG_NODE "node"
#endif

// Standard android macros cloned to __<> for use in nodejs code and to avoid conflicts with existing webkit code
// These are always defined irrespective of DEBUG flags
// FIXME: need to disable the VERBOSE versions in release build
#define NODE_LOGV(...) DAPILog(DAPI_LOG_VERBOSE, LOG_TAG_NODE, __VA_ARGS__);
#define NODE_LOGD(...) DAPILog(DAPI_LOG_DEBUG,   LOG_TAG_NODE, __VA_ARGS__);
#define NODE_LOGI(...) DAPILog(DAPI_LOG_INFO,    LOG_TAG_NODE, __VA_ARGS__);
#define NODE_LOGW(...) DAPILog(DAPI_LOG_WARN,    LOG_TAG_NODE, __VA_ARGS__);
#define NODE_LOGE(...) DAPILog(DAPI_LOG_ERROR,   LOG_TAG_NODE, __VA_ARGS__);

// Function entry
#define NODE_LOGF() DAPILog(DAPI_LOG_VERBOSE, LOG_TAG_NODE, "%s:%d", __FUNCTION__, __LINE__); //Function entry
#define NODE_LOGFT() DAPILog(DAPI_LOG_VERBOSE, LOG_TAG_NODE, "%s:%d [%p]", __FUNCTION__, __LINE__, this); //Function entry with this
#define NODE_LOGFR() DAPILog(DAPI_LOG_VERBOSE, LOG_TAG_NODE, "%s:%d: return", __FUNCTION__, __LINE__); //Function entry

// some functions get invoked too many times (e.g. ev_invoke_pending), so logs for these are disabled by default
// if you need to log those enable the following
// #define LOG_MULTIPLE
#ifdef LOG_MULTIPLE
#define NODE_LOGM(...) DAPILog(DAPI_LOG_VERBOSE, LOG_TAG_NODE, __VA_ARGS__);
#else
#define NODE_LOGM(...)
#endif

// use it for development and when done replace with appropriate macros..
#define NODE_LOGT(...) DAPILog(DAPI_LOG_ERROR,   LOG_TAG_NODE, __VA_ARGS__);

#if 0
#define NODE_CRASH()
#else
#define NODE_CRASH() { *(int *)0xDEADBEAD = 0; }
#endif

#define NODE_ASSERT(cond) { \
  if (!(cond)) { \
    DAPILog(DAPI_LOG_ERROR, LOG_TAG_NODE, "%s, %s, %d, === ASSERT === \"%s\"", \
      __FILE__, __FUNCTION__, __LINE__, #cond); \
    NODE_CRASH(); \
  } \
}

#define NODE_ASSERT_COND(cond, ...) { \
  if (!(cond)) { \
    DAPILog(DAPI_LOG_ERROR, LOG_TAG_NODE, "=== ASSERT ===" __VA_ARGS__ ); \
    NODE_CRASH(); \
  } \
}

#define NODE_ASSERT_REACHABLE() { \
  DAPILog(DAPI_LOG_ERROR, LOG_TAG_NODE, "%s, %s, %d, === ASSERT === REACHABLE", \
      __FILE__, __FUNCTION__, __LINE__); \
  NODE_CRASH(); \
}

#define NODE_NI() NODE_LOGW("%s, %d NOT_IMPLEMENTED", __FUNCTION__, __LINE__);


#endif


