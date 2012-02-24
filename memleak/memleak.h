/*
 * Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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

#ifndef MEM_LEAK_H
#define MEM_LEAK_H

#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>

#include <vector>
#include <map>
#include <string>
#include <algorithm>

#ifndef ANDROID
#include <execinfo.h>
#include <cxxabi.h>
#endif

#ifdef ANDROID
#define USE_DL_PREFIX
#include "dlmalloc.h"
#include "malloc_debug_common.h"
#include "unwind.h"
#endif

#ifdef ANDROID
#include <android/log.h>
#define MEM_LEAK_LOG(...) do { __android_log_print(ANDROID_LOG_INFO, "MEM_LEAK", __VA_ARGS__);} while (0);
#define ASSERT(x) { \
  if (!(x)) {__android_log_print(ANDROID_LOG_ERROR, "MEM_LEAK",  "%s, %s, %d, *** ASSERT *** \"%s\"", \
      __FILE__, __FUNCTION__, __LINE__, #x); *(int *)0xBAADBAAD = 0;}}

#define ASSERT_REACHABLE() \
  __android_log_print(ANDROID_LOG_ERROR, "MEM_LEAK", "%s, %s, %d, *** ASSERT ***", \
      __FILE__, __FUNCTION__, __LINE__); *(int *)0xBAADBAAD = 0;

#define MEM_LEAK_LOG_RAW MEM_LEAK_LOG
#else
#define MEM_LEAK_LOG(...) do { printf("MEM_LEAK"); printf(__VA_ARGS__); printf("\n"); fflush(stdout);} while (0);
#define MEM_LEAK_LOG_RAW(...) do { printf(__VA_ARGS__); printf("\n"); fflush(stdout);} while (0);
#define ASSERT(x) { \
  if (!(x)) {printf("MEM_LEAK %s, %s, %d, *** ASSERT *** \"%s\"", \
      __FILE__, __FUNCTION__, __LINE__, #x); *(int *)0xBAADBAAD = 0;}}

#define ASSERT_REACHABLE() \
  printf("MEM_LEAK %s, %s, %d, *** ASSERT ***", \
      __FILE__, __FUNCTION__, __LINE__); *(int *)0xBAADBAAD = 0;
#endif

#define MEM_LEAK_LOGD(...)

#define LOG_FILE "memlog"
#define MAX_TRACK_ALLOCS 7
#define MAX_BT_LEVEL  20

struct CallStackInfo {
  CallStackInfo(long int size);
  CallStackInfo();

  void add(long int size);
  long int m_total;
  long int m_sizes[MAX_TRACK_ALLOCS];
  int m_count[MAX_TRACK_ALLOCS];
  int m_allocs;
};

template<class T>
class less_second : std::binary_function<T,T,bool> {
  public:
    inline bool operator()(const T& lhs, const T& rhs) {
      return lhs.second.m_total > rhs.second.m_total;
    }
};

struct CallStack {
  CallStack(void **addr);
  void *m_addr[MAX_BT_LEVEL];
};

class MallocInfo {
  public:
    MallocInfo(void **addr_, int size) {
      memset(m_addr, 0, sizeof(m_addr));
      for (int i = 0; i < MAX_BT_LEVEL && addr_[i]; i++){
        m_addr[i] = addr_[i];
      }
      m_size = size;
    }

    MallocInfo() : m_size(0){}
    int size() const {return m_size;}
    void **addr(){ return m_addr;}

  private:
    void *m_addr[MAX_BT_LEVEL];
    int m_size;
};

class MemLeakTracker {
  public:
    MemLeakTracker();
    ~MemLeakTracker();

    void init();
    void addMallocInfo(void *p, void **addr, int msize);
    void removeMallocInfo(void *p);

    // sort the map by callstack and write it
    void writeLeaks();
    void writeMaps();
    void createCallStackMap();
    void writeSortedCallStack();

    // hooks
    static void* x86_realloc_hook(void* p, size_t size, const void* caller);
    static void* x86_malloc_hook(size_t size, const void *caller);
    static void  x86_free_hook(void *p, const void *caller);

    void logWrap(const char* fmt, ...);

#if ANDROID
    // arm specific
    static void* arm_malloc_hook(size_t size);
    static void  arm_free_hook(void* mem);
    static void* arm_calloc_hook(size_t n_elements, size_t elem_size);
    static void* arm_realloc_hook(void* mem, size_t bytes);
    static void* arm_memalign_hook(size_t alignment, size_t bytes);
    int arm_backtrace(void** addrs, size_t max_entries);
    static MallocDebug m_arm_malloc_dispatch __attribute__((aligned(32)));
#endif

    void addMalloc(void *mem);
    void addRealloc(void* mem, void* p);
    void removeMalloc(void *p);

    static void signalHandler(int sig_num);

    bool inMainThread();
    void storeHooks();
    void setHooks();
    void resetHooks();

    bool m_disabled;
    int m_depth;
    int m_dumpSize;
    int m_trackSize;
    int m_breakSize;
    int m_initialized;
    bool m_track;
    pthread_t m_thread;
    FILE *m_file;

    std::map<void*, MallocInfo> m_mallocMap; // each address mapped to a size
    typedef std::map<void *, MallocInfo>::iterator m_mallocMapIterator;

    std::map<struct CallStack, struct CallStackInfo> m_callStackMap;
    typedef std::map<struct CallStack, struct CallStackInfo>::iterator m_callStackMapIterator;

    void *(*libc_malloc_hook)(size_t size, const void *caller);
    void *(*libc_realloc_hook)( void *,size_t size,  const void *caller);
    void (*libc_free_hook)( void *, const void *caller);

    void lock(const char* context);
    void unlock(const char* context);
    pthread_mutex_t m_mutex;

    int RegisterSignalHandler(int signal, void (*handler)(int));
    static struct sigaction old_sa[NSIG];

    static MemLeakTracker *global_instance;
    static void start();
    static void stop();
    static bool active();
};

#endif
