/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef ForkJoinPool_h__
#define ForkJoinPool_h__

#include "ThreadPool.h"

namespace js {

class ForkJoinPool;

ForkJoinPool *CreateForkJoinPool(size_t numWorkers);
ForkJoinPool *AddForkJoinPoolRef(ForkJoinPool *);
void DropForkJoinPool(ForkJoinPool *);
uint32_t ForkJoinPoolWorkers(ForkJoinPool *);

#ifdef JS_THREADSAFE
// To make use of the fork-join pool, you should follow
// the following pattern:
//
// {
//     AutoForkJoinPool auto(*pool);
//     if (!auto.init(cx))
//         return false;
//     ...
//     /* ideally, do some amount of useful work here */
//     ...
//     if (!auto.submit(executor))
//         return false;
//     ...
//     /* you must ensure all threads have terminated somehow */
// }
//
// The `AutoForkJoinPool` constructor blocks until the `ForkJoinPool`
// can be acquired (perhaps some other webworker or thread is using
// it) and also signals the worker threads to wake up if they have
// fallen into an idle state due to inactivity. Ideally you will
// do some amount of work after the constructor so as to give the
// threads time to respond and get warmed up.
//
// The `submit()` method then triggers the actual execution. The
// `executeFromWorker()` method on the `executor` object will be
// invoked once from each worker thread.  *YOU ARE RESPONSIBLE FOR NOT
// RELEASING THE FORKJOINPOOL UNTIL `executeFromWorker()` HAS AT LEAST
// *BEGUN* EXECUTION FROM EACH WORKER THREAD.* The `ForkJoinPool` does
// not itself provide a means to block until the workers have
// finished. Typically this would be done by having the executor job
// set flags or counters that you are blocking on.
class AutoForkJoinPool {
  private:
    ForkJoinPool &pool_;

  public:
    AutoForkJoinPool(ForkJoinPool &pool);
    ~AutoForkJoinPool();

    bool init(JSContext *cx);
    void submit(TaskExecutor *executor);
};
#endif

} /* namespace js */

#endif /* ForkJoinPool_h__ */
