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

#include <unistd.h>
#include <string>
#include "memleak.h"

pthread_t s_thread;
pthread_mutex_t mutex;
pthread_cond_t cond;
int count = 1;

#define NOINLINE __attribute__((noinline))


void test1() {
  //while (true)
  for (int i = 1; i<= count; i++) {
    void *malloc1 = malloc(100);
    void *malloc2 = malloc(120);
    char *new1 = new char[200];
    void *realloc1 = realloc(malloc(300), 400);

    free(malloc(500));
    delete (new char[50]);
  }
}

NOINLINE
void f3(int n) {
  malloc(n);
}

NOINLINE
void f2(int n) {
  f3(n);
}

NOINLINE
void f1(int n){
  f2(n);
}

void* runThread(void*) {
  pthread_cond_signal(&cond);
  for (int i = 1; i<= count; i++) {
    void *malloc1 = malloc(100);
    char *new1 = new char[200];
    void *realloc1 = realloc(malloc(300), 400);

    free(malloc(500));
    delete (new char[50]);
  }
  return 0;
}

int main(int argc, char **argv) {
  for (int i = 1; i < argc; i++){
    if (strstr(argv[i], "--wait")){
      sleep(15);
    }
  }
  MemLeakTracker::start();

  // wait till thread starts..
  pthread_mutex_init(&mutex, 0);
  pthread_cond_init(&cond, 0);
  pthread_create(&s_thread, 0, runThread, 0);
  pthread_cond_wait(&cond, &mutex);

  std::string first = "hello";
  std::string second = "world";
  std::string third = first + "/" + second;
  for (int i = 0; i <= 100; i++) {
    third += "hello world";
  }
  test1();
  for (int i = 1; i <= 100; i++) {
    f1(100 + i);
  }
  int ret = pthread_join(s_thread, 0);
  MemLeakTracker::stop();
  return 0;
}
