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

#include "memleak.h"

using namespace std;

MemLeakTracker *MemLeakTracker::global_instance = 0;
#define si() MemLeakTracker::global_instance

#if ANDROID
extern const MallocDebug __libc_malloc_default_dispatch;
extern const MallocDebug* __libc_malloc_dispatch;
#endif

MemLeakTracker::MemLeakTracker()
  : m_disabled(false)
  , m_depth(10)
  , m_dumpSize(10000)
  , m_trackSize(0)
  , m_breakSize(-1)
  , m_thread(pthread_self())
  , m_file(0)
{
  init();
}

void MemLeakTracker::start() {
  if (!global_instance) {
    MEM_LEAK_LOG("** MemLeakTracker started");
    global_instance = new MemLeakTracker();
  }
}

void MemLeakTracker::stop() {
  if (global_instance) {
    delete global_instance;
    MEM_LEAK_LOG("** MemLeakTracker stopped");
    global_instance = 0;
  }
}

bool MemLeakTracker::active() {
  return !!global_instance;
}

#define ENV(var, value, desc)     \
  if (char *v = getenv(#value)) { \
     var = atoi(v);               \
  }                               \
  MEM_LEAK_LOG(#value ": " desc " = %d", var)

#ifdef ANDROID
MallocDebug MemLeakTracker::m_arm_malloc_dispatch = {
  arm_malloc_hook, arm_free_hook, arm_calloc_hook, arm_realloc_hook, arm_memalign_hook
};
#endif

struct sigaction MemLeakTracker::old_sa[NSIG];
int MemLeakTracker::RegisterSignalHandler(int signal, void (*handler)(int)) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handler;
  sigfillset(&sa.sa_mask);
  return sigaction(signal, &sa, &old_sa[signal]);
}

void MemLeakTracker::init() {
  ENV(m_disabled, MEM_LEAK_DISABLE, "disable memleak tracking");
  if (m_disabled) {
    return;
  }

  ENV(m_depth, MEM_LEAK_DEPTH, "depth of the call stack");
  ENV(m_trackSize, MEM_LEAK_TRACK_SIZE, "track allocations greater this size only");
  ENV(m_breakSize, MEM_LEAK_BREAK_SIZE, "break when the allocation is of this size");
  ENV(m_dumpSize, MEM_LEAK_DUMP_SIZE, "dump memory traces leaking more than this size");
  if (m_depth > MAX_BT_LEVEL) {
    m_depth = MAX_BT_LEVEL;
  }

  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&m_mutex, &attr);
#ifdef ANDROID
  __libc_malloc_dispatch = &m_arm_malloc_dispatch;
#else
  storeHooks();
  setHooks();
#endif
  RegisterSignalHandler(SIGUSR1, MemLeakTracker::signalHandler);
  RegisterSignalHandler(SIGSEGV, MemLeakTracker::signalHandler);
  RegisterSignalHandler(SIGINT, MemLeakTracker::signalHandler);
  RegisterSignalHandler(SIGTRAP, SIG_IGN);

  MEM_LEAK_LOG("running --:)");
}

bool MemLeakTracker::inMainThread() {
  return m_thread == pthread_self();
}

void MemLeakTracker::lock(const char* context) {
  MEM_LEAK_LOGD("%s thread, context %s, waiting lock", inMainThread() ? "Main" : "Worker", context);
  pthread_mutex_lock(&m_mutex);
  MEM_LEAK_LOGD("%s thread, context %s, got lock", inMainThread() ? "Main" : "Worker", context);
}

void MemLeakTracker::unlock(const char* context) {
  MEM_LEAK_LOGD("%s thread, context %s, unlock", inMainThread() ? "Main" : "Worker", context);
  pthread_mutex_unlock(&m_mutex);
}

#define WRITE_MAPS 0
#define WRITE_TO_FILE 0
#define MAX_FILE_NAME_SIZE 20
void MemLeakTracker::writeMaps() {
#if WRITE_TO_FILE
  static int count = 0;
  char log_file[MAX_FILE_NAME_SIZE];
  sprintf(log_file, "%s-%d", LOG_FILE, count++);
  MEM_LEAK_LOG("Logging to File : %s", log_file);
  m_file = fopen(log_file,"w");
  ASSERT(m_file);
#else
  m_file = stdout;
#endif

#if WRITE_MAPS
  int fd = open("/proc/self/maps", O_RDONLY);
  char buf[1024];
  fprintf(m_file,"MAPS_START\n");
  int n = 0;
  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    fwrite(buf,1, n, m_file);
  }
  fprintf(m_file,"MAPS_END\n");
#endif

}

void MemLeakTracker::signalHandler(int signum) {
  MEM_LEAK_LOG("received %d signal, dumping memory usage", signum);
  si()->writeLeaks();

  if (signum == SIGSEGV) {
    MEM_LEAK_LOG("** exit");
    exit(0);
  }

  if (signum == SIGINT) {
    signal(SIGINT, SIG_DFL);
    kill(getpid(), SIGINT);
  }
}

void MemLeakTracker::storeHooks() {
  // No need to store hooks on arm
#ifndef ANDROID
  libc_malloc_hook = __malloc_hook;
  libc_realloc_hook = __realloc_hook;
  libc_free_hook = __free_hook;
#endif
}

void MemLeakTracker::resetHooks() {
#ifdef ANDROID
  __libc_malloc_dispatch = &__libc_malloc_default_dispatch;
#else
  __malloc_hook = libc_malloc_hook;
  __realloc_hook = libc_realloc_hook;
  __free_hook = libc_free_hook;
#endif
}

void MemLeakTracker::setHooks() {
#ifdef ANDROID
  __libc_malloc_dispatch = &m_arm_malloc_dispatch;
#else
  __malloc_hook = x86_malloc_hook;
  __realloc_hook = x86_realloc_hook;
  __free_hook = x86_free_hook;
#endif
}

void MemLeakMallocBreak(int size) {
  MEM_LEAK_LOG("**MEM_LEAK_CHECK: in MemLeakMallocBreak %d\n",size);
  raise(SIGTRAP);
}

void MemLeakTracker::addMalloc(void *mem) {
#ifdef ANDROID
  long int size = dlmalloc_usable_size(mem);
#else
  long int size = malloc_usable_size(mem);
#endif
  if (size == m_breakSize){
    MemLeakMallocBreak(size);
  }

  if (size >= m_trackSize) {
    // request three extra entries and we skip the malloc/addMalloc/backtrace functions
    static void *addr[MAX_BT_LEVEL + 3] = {0};
    memset(addr, 0, sizeof(addr));
#ifdef ANDROID
    arm_backtrace(addr, m_depth + 3);
#else
    backtrace(addr, m_depth + 3);
#endif
    si()->addMallocInfo(mem, addr + 2, size);
  }
}

void* MemLeakTracker::x86_malloc_hook(size_t size, const void *caller) {
  si()->lock("malloc");

  si()->resetHooks();
  void *mem = malloc(size);
  MEM_LEAK_LOGD("%s, malloc, %d", si()->inMainThread() ? "Main" : "Worker", size);
  si()->addMalloc(mem);
  si()->setHooks();

  si()->unlock("malloc");
  return mem;
}

void MemLeakTracker::addRealloc(void* mem, void* p) {
  if (p) {
    si()->removeMalloc(p);
  }
  if (mem) {
    si()->addMalloc(mem);
  }
}

void* MemLeakTracker::x86_realloc_hook(void *p,size_t size, const void* caller) {
  si()->lock("realloc");

  si()->resetHooks();
  void *mem = realloc(p, size);
  MEM_LEAK_LOGD("%s, realloc, %d", si()->inMainThread() ? "Main" : "Worker", size);
  si()->addRealloc(mem, p);
  si()->setHooks();

  si()->unlock("realloc");
  return mem;
}

void MemLeakTracker::removeMalloc(void *p) {
  si()->removeMallocInfo(p);
}

void MemLeakTracker::x86_free_hook( void *p, const void *caller) {
  if (p == 0) {
    return;
  }

  si()->lock("free");

  // set old hook, track, do the op, and revert hooks
  si()->resetHooks();
  si()->removeMalloc(p);
  free(p);
  si()->setHooks();

  si()->unlock("free");
}

#if ANDROID
void* MemLeakTracker::arm_malloc_hook(size_t size) {
  si()->lock("malloc");

  si()->resetHooks();
  void *mem = malloc(size);
  MEM_LEAK_LOGD("%s, malloc, %d", si()->inMainThread() ? "Main" : "Worker", size);
  si()->addMalloc(mem);
  si()->setHooks();

  si()->unlock("malloc");
  return mem;
}

void  MemLeakTracker::arm_free_hook(void* p) {
  if (p == 0) {
    return;
  }

  si()->lock("free");

  // set old hook, track, do the op, and revert hooks
  si()->resetHooks();
  si()->removeMalloc(p);
  free(p);
  si()->setHooks();

  si()->unlock("free");
}

void* MemLeakTracker::arm_calloc_hook(size_t n_elements, size_t elem_size) {
  void *ptr = arm_malloc_hook(n_elements * elem_size);
  if (ptr) {
    memset(ptr, 0, n_elements * elem_size);
  }
  return ptr;
}

void* MemLeakTracker::arm_realloc_hook(void* p, size_t size) {
  si()->lock("realloc");

  si()->resetHooks();
  void *mem = realloc(p, size);
  MEM_LEAK_LOGD("%s, realloc, %d", si()->inMainThread() ? "Main" : "Worker", size);
  si()->addRealloc(mem, p);
  si()->setHooks();

  si()->unlock("realloc");
  return mem;
}

void* MemLeakTracker::arm_memalign_hook(size_t alignment, size_t bytes) {
  return arm_malloc_hook(bytes);
}
#endif

MemLeakTracker::~MemLeakTracker() {
  writeLeaks();
  resetHooks();
}

void MemLeakTracker::addMallocInfo(void *p, void **addr,  int size){
  m_mallocMap[p] = MallocInfo(addr, size);
}

void MemLeakTracker::removeMallocInfo(void *p) {
  m_mallocMapIterator it = m_mallocMap.find(p);
  if (it != m_mallocMap.end()) {
    m_mallocMap.erase(it);
  } else {
    MEM_LEAK_LOGD("free, %p not tracked", p);
  }
}

CallStack::CallStack(void **addr) {
  memset(m_addr, 0, sizeof(m_addr));
  for (int i = 0; i < MAX_BT_LEVEL && addr[i]; i++)
    m_addr[i] = addr[i];
}

bool operator<(const CallStack &a, const CallStack &b) {
  return memcmp(a.m_addr, b.m_addr, sizeof(a.m_addr[0]) * si()->m_depth) > 0;
}

void CallStackInfo::add(long int size) {
  m_allocs++;
  int index = size % MAX_TRACK_ALLOCS;
  if (m_sizes[index]) {
    if (m_sizes[index] == size) {
      m_count[index]++;
      MEM_LEAK_LOGD("size %ld already exists, new count %d",
          size, m_count[index]);
    } else {
      MEM_LEAK_LOGD("hash clash for size=%ld, with size=%ld",
          size, m_sizes[index]);
    }
  } else {
    m_sizes[index] = size;
    m_count[index]++;
  }
}

CallStackInfo::CallStackInfo(long int size) : m_total(size), m_allocs(0) {
  memset(m_sizes, 0, sizeof(m_sizes));
  memset(m_count, 0, sizeof(m_count));
  add(size);
}

CallStackInfo::CallStackInfo() : m_total(0), m_allocs(0) {
  memset(m_sizes, 0, sizeof(m_sizes));
  memset(m_count, 0, sizeof(m_count));
}

// Traverse the leak map and create a call stack map with callstack addresses
// as key and size as the value
void MemLeakTracker::createCallStackMap() {
  for (m_mallocMapIterator it = m_mallocMap.begin(); it != m_mallocMap.end(); ++it) {
    struct CallStack addr(it->second.addr());
    long int size = it->second.size();
    m_callStackMapIterator it_ = m_callStackMap.find(addr);
    if (it_ == m_callStackMap.end()) {
      m_callStackMap[addr] = CallStackInfo(size);
    } else {
      it_->second.m_total += size;
      it_->second.add(size);
    }
  }
}

#ifndef ANDROID
std::string addressToString(void *addr) {
  void *buffer[1] = { addr };
  char **strings = backtrace_symbols(buffer, 1);
  if (!strings) {
    return "";
  }

  std::string symbol = strings[0];
  free(strings);

  // demangle..
  char *mangled = strdup(symbol.c_str());
  MEM_LEAK_LOGD("symbol %s", mangled);
  char *begin = 0, *offset = 0, *end = 0;
  char *p = mangled ;
  while (*p) {
    if (*p == '(') {
      begin = p;
    } else if (*p == '+') {
      offset = p;
    } else if (*p == ')' && offset) {
      end = p;
    }
    p++;
  }

  if (begin && offset && end && begin + 1 < offset) {
    // +1 is for case where content before + is empty as in
    // /usr/lib32/libcrypto.so.0.9.8(+0x3e28e)
    // libc..(__libc_malloc+0x1d8fc) ..
    //       ^begin        ^ ofset ^ end
    *begin++ = 0;
    *offset++ = 0;
    *end = 0;

    int status;
    char *demangled = abi::__cxa_demangle(begin, 0, 0, &status);
    if (status) {
      MEM_LEAK_LOGD("demangled failed for %s", begin);
    }

    symbol = demangled ? demangled : begin + string("+") + offset;
    free(mangled);
    free(demangled);
  }

  return symbol;
}
#endif

void MemLeakTracker::logWrap(const char* fmt, ...) {
  va_list ap;
  static char buf[1028];
  va_start(ap, fmt);
  vsnprintf(buf, 1028, fmt, ap);
  va_end(ap);
}

typedef struct mapinfo {
  struct mapinfo *next;
  unsigned start;
  unsigned end;
  unsigned exidx_start;
  unsigned exidx_end;
  struct symbol_table *symbols;
  bool isExecutable;
  char *name_start;
  char name[];
} mapinfo;

mapinfo *milist = 0;

// 6f000000-6f01e000 rwxp 00000000 00:0c 16389419   /system/lib/libcomposer.so
// 012345678901234567890123456789012345678901234567890123456789
// 0         1         2         3         4         5

mapinfo *parse_maps_line(char *line) {
  mapinfo *mi;
  int len = strlen(line);

  if (len < 1) return 0;      /* not expected */
  line[--len] = 0;

  if (len < 50) {
    mi = (mapinfo *)malloc(sizeof(mapinfo) + 1);
  } else {
    mi = (mapinfo *)malloc(sizeof(mapinfo) + (len - 47));
  }
  if (mi == 0) return 0;

  mi->isExecutable = (line[20] == 'x');

  mi->start = strtoul(line, 0, 16);
  mi->end = strtoul(line + 9, 0, 16);
  /* To be filled in parse_elf_info if the mapped section starts with
   * elf_header
   */
  mi->exidx_start = mi->exidx_end = 0;
  mi->symbols = 0;
  mi->next = 0;
  mi->name_start = 0;
  if (len < 50) {
    mi->name[0] = '\0';
  } else {
    strcpy(mi->name, line + 49);
    // skip white spaces..
    char *p = mi->name;
    while (*p && *p++ == ' ');
    mi->name_start = p - 1;
  }


  return mi;
}

void readMaps() {
  FILE *fp = fopen("/proc/self/maps", "r");
  if (fp) {
    char data[1024];
    while(fgets(data, 1024, fp)) {
      mapinfo *mi = parse_maps_line(data);
      if(mi) {
        mi->next = milist;
        milist = mi;
      }
    }
    fclose(fp);
  }

  MEM_LEAK_LOGD("readMaps, dumping maps info");
  mapinfo *p = milist;
  while (p) {
    MEM_LEAK_LOGD("%s, %x-%x", p->name[0] ? p->name_start : "<unknown>", p->start, p->end);
    p = p->next;
  }
}

mapinfo* addressToMap(unsigned pc, unsigned *rel_pc) {
  ASSERT(milist);
  mapinfo *p = milist;
  *rel_pc = pc;
  while (p) {
    if (pc >= p->start && pc <= p->end) {
      MEM_LEAK_LOGD("addressToMap, %x -> [%s, %x]", pc,
          p->name[0] ? p->name: "<unknown>", pc - p->start);
      if (strstr(p->name, ".so")) {
        *rel_pc = pc - p->start;
      }
      return p;
    }
    p = p->next;
  }
  MEM_LEAK_LOG("addressToMap, error for addr %x not in maps", pc);
  return 0;
}

// sort the call stack map and dump to file
void MemLeakTracker::writeSortedCallStack() {
  typedef pair<struct CallStack, struct CallStackInfo> data_t;
  vector<data_t> vec(m_callStackMap.begin(), m_callStackMap.end());
  sort(vec.begin(), vec.end(), less_second<data_t>());
  typedef vector<data_t>::iterator veciterator;
  for (veciterator vit=vec.begin(); vit!=vec.end(); ++vit) {
    if (vit->second.m_total >= m_dumpSize) {
      ASSERT(vit->first.m_addr[0]);
#define ACC_SIZE 4096
      static char acc[ACC_SIZE];
      struct CallStackInfo *info = &vit->second;
      snprintf(acc, ACC_SIZE, "%ld [", info->m_total);
      int tracked = 0;
      for (int i = 0; i < MAX_TRACK_ALLOCS; i++) {
        if (info->m_sizes[i]) {
          snprintf(acc + strlen(acc), ACC_SIZE, "%ld/%d, ", info->m_sizes[i], info->m_count[i]);
          tracked += info->m_count[i];
        }
      }
      snprintf(acc + strlen(acc), ACC_SIZE, "=%d/%d=]\n", tracked, info->m_allocs);
      for (int i = 0; i < m_depth && vit->first.m_addr[i]; i++) {
        const char* fmt = "|------------------------------";
#ifdef ANDROID
        unsigned addr = (unsigned)vit->first.m_addr[i];
        unsigned rel_pc = 0;
        mapinfo *map = addressToMap(addr, &rel_pc);
        snprintf(acc + strlen(acc), ACC_SIZE, "[%p] #%02d  pc %08x  %s\n", addr, i,
            rel_pc, map ? map->name_start : "<unknown>");
#else
        void* addr = vit->first.m_addr[i];
        snprintf(acc + strlen(acc), ACC_SIZE, "%.*s%s\n", i * 2 + 1, fmt, addressToString(addr).c_str());
#endif
      }
      MEM_LEAK_LOG_RAW(acc);
    }
  }

  if (m_file != stdout) {
    fclose(m_file);
    m_file = 0;
  }
}

void MemLeakTracker::writeLeaks() {
  if (m_mallocMap.size() == 0) {
    MEM_LEAK_LOG("No leaks :)");
    return;
  }

  resetHooks();
  readMaps();
  writeMaps();
  createCallStackMap();
  writeSortedCallStack();
  setHooks();
}

////////////////////////////////// ARM backtrace //////////////////////////
#if ANDROID
typedef struct {
  size_t count;
  void** addrs;
} stack_crawl_state_t;

static _Unwind_Reason_Code trace_function(struct _Unwind_Context *context, void *arg) {
  stack_crawl_state_t* state = (stack_crawl_state_t*)arg;
  if (state->count) {
    void* ip = (void*)_Unwind_GetIP(context);
    if (ip) {
      state->addrs[0] = ip;
      state->addrs++;
      state->count--;
      return _URC_NO_REASON;
    }
  }
  return _URC_END_OF_STACK;
}

int MemLeakTracker::arm_backtrace(void** addrs, size_t max_entries) {
  stack_crawl_state_t state;
  state.count = max_entries;
  state.addrs = addrs;
  _Unwind_Backtrace(trace_function, (void*)&state);
  return max_entries - state.count;
}
#endif

