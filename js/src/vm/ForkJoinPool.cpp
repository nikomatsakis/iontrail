/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef JS_THREADSAFE
#include "prtime.h"
#include "prthread.h"
#endif

#include "jsapi.h"
#include "jscntxt.h"

#include "vm/Monitor.h"
#include "vm/ForkJoinPool.h"

#define PRINTF_ENABLED false
#define PRINTF(...) \
    do { if (PRINTF_ENABLED) fprintf(stderr, __VA_ARGS__); } while (0)

#ifndef JS_THREADSAFE

class ForkJoinPool {}
ForkJoinPool dummy;

ForkJoinPool *
js::CreateForkJoinPool(size_t numWorkers)
{
    return &dummy;
}

ForkJoinPool *
js::AddForkJoinPoolRef(ForkJoinPool *pool)
{
    JS_ASSERT(pool == &dummy);
    return NULL;
}

void
js::DropForkJoinPool(ForkJoinPool *pool)
{
    JS_ASSERT(pool == &dummy);
}

uint32_t
js::ForkJoinPoolWorkers(ForkJoinPool *pool)
{
    JS_ASSERT(pool == &dummy);
    return 0;
}

#else

namespace js {

class ForkJoinPool : public Monitor {
  private:
    friend class ForkJoinPoolWorker;

    enum PoolState {
        PoolIdle,
        PoolActive,
        PoolTerminating,
    };
    PoolState state_;
    TaskExecutor *executor_;
    uint32_t workCounter_;
    uint32_t refCount_;

    uint32_t desiredWorkers_;
    uint32_t startedWorkers_;
    uint32_t activeWorkers_;

    bool ensureThreadsHaveStarted(AutoLockMonitor &lock);
    void terminateThreads(AutoLockMonitor &lock);
    bool isTerminating() { return state_ == PoolTerminating; }

  public:
    ForkJoinPool(size_t numWorkers);

    ForkJoinPool *addRef();
    void dropRef();

    bool acquire();
    void release();
    void submit(TaskExecutor *executor);

    // Number of workers we will have once fully started.
    uint32_t numWorkers() {
        return desiredWorkers_;
    }
};

class ForkJoinPoolWorker {
  private:
    static const size_t STACK_SIZE = 1*1024*1024;

    ForkJoinPool &pool_;
    const size_t id_;

    static void runStatic(void *arg);
    void run();
    bool blockUntilPoolIsActiveOrTerminating();

  public:
    ForkJoinPoolWorker(ForkJoinPool &pool, size_t id)
      : pool_(pool),
        id_(id)
    {}

    bool start();
};

}

///////////////////////////////////////////////////////////////////////////
// ForkJoinPool

js::ForkJoinPool::ForkJoinPool(size_t numWorkers)
  : state_(PoolIdle),
    executor_(NULL),
    workCounter_(0),
    refCount_(1),
    desiredWorkers_((numWorkers == 0 ? GetCPUCount() - 1 : numWorkers)),
    startedWorkers_(0),
    activeWorkers_(0)
{
    if (char *jsthreads = getenv("JS_THREADPOOL_SIZE"))
        desiredWorkers_ = strtol(jsthreads, NULL, 10);
}

js::ForkJoinPool *
js::ForkJoinPool::addRef()
{
    JS_ATOMIC_INCREMENT(&refCount_);
    return this;
}

void
js::ForkJoinPool::dropRef()
{
    uint32_t rc = JS_ATOMIC_DECREMENT(&refCount_);
    if (rc == 0) {
        // At this point, there are no users left, and the guy who
        // just dropped us should not have dropped us during an active
        // session!
        JS_ASSERT(state_ == PoolIdle);
        {
            AutoLockMonitor lock(*this);
            terminateThreads(lock);
        }
        delete this;
    }
}

bool
js::ForkJoinPool::acquire()
{
    PRINTF("ForkJoinPool::acquire()\n");

    AutoLockMonitor lock(*this);
    JS_ASSERT(activeWorkers_ == 0);

    // Another thread is making use of the pool.
    // Wait for `release()` to notify us that pool
    // is idle.
    while (state_ != PoolIdle)
        lock.wait();

    if (!ensureThreadsHaveStarted(lock))
        return false;

    state_ = PoolActive;

    // Wake up any workers that are waiting in
    // `blockUntilPoolIsActiveOrTerminating()` for state to change
    // from idle:
    lock.notifyAll();

    return true;
}

void
js::ForkJoinPool::release()
{
    PRINTF("ForkJoinPool::release()\n");

    AutoLockMonitor lock(*this);
    JS_ASSERT(state_ != PoolIdle);
    JS_ASSERT(activeWorkers_ == 0);

    state_ = PoolIdle;
    executor_ = NULL;

    // If there are workers blocked, they will not care that the
    // state has changed to idle, but there may be another master
    // thread blocked in `acquire()`.
    lock.notifyAll();
}

void
js::ForkJoinPool::submit(TaskExecutor *executor)
{
    JS_ASSERT(state_ == PoolActive);
    JS_ASSERT(activeWorkers_ == 0);
    JS_ASSERT(startedWorkers_ == desiredWorkers_);

    executor_ = executor;
#ifdef DEBUG
    activeWorkers_ = startedWorkers_;
#endif

    PRINTF("Submit(): producing work item %d with %d workers\n",
        workCounter_ + 1,
        startedWorkers_);

    // Signal to threads that new work is available. Note the release
    // barrier, which guarantees that all writes which have occurred
    // until this point will be visible to worker threads once they
    // observe the new value of `counter_`:
    JS_ATOMIC_STORE_RELEASE(&workCounter_, workCounter_ + 1);
}

bool
js::ForkJoinPool::ensureThreadsHaveStarted(AutoLockMonitor &lock)
{
    JS_ASSERT(state_ == PoolIdle);

    while (startedWorkers_ < desiredWorkers_) {
        ForkJoinPoolWorker *worker =
            new ForkJoinPoolWorker(*this, startedWorkers_);

        if (!worker) {
            terminateThreads(lock);
            return false;
        }

        if (!worker->start()) {
            delete worker;
            terminateThreads(lock);
            return false;
        }

        startedWorkers_++;
    }

    return true;
}

void
js::ForkJoinPool::terminateThreads(AutoLockMonitor &lock)
{
    JS_ASSERT(state_ == PoolIdle);
    JS_ASSERT(activeWorkers_ == 0);

    activeWorkers_ = startedWorkers_;
    state_ = PoolTerminating;

    // Notify workers blocked in `blockUntilPoolIsActiveOrTerminating()`
    // that state has changed from idle.
    lock.notifyAll();

    // Wait for last worker to notify us from `ForkJoinPoolWorker::run()`
    // that it has terminated.
    while (activeWorkers_ != 0)
        lock.wait();
}

///////////////////////////////////////////////////////////////////////////
// ForkJoinPoolWorker

bool
js::ForkJoinPoolWorker::start()
{
    PRThread *thread = PR_CreateThread(PR_USER_THREAD,
                                       runStatic,
                                       this,
                                       PR_PRIORITY_NORMAL,
                                       PR_LOCAL_THREAD,
                                       PR_UNJOINABLE_THREAD,
                                       STACK_SIZE);
    return !!thread;
}

void
js::ForkJoinPoolWorker::runStatic(void *arg)
{
    ForkJoinPoolWorker *worker = static_cast<ForkJoinPoolWorker*>(arg);
    worker->run();
}

void
js::ForkJoinPoolWorker::run()
{
    // This is hokey in the extreme.  To compute the stack limit,
    // subtract the size of the stack from the address of a local
    // variable and give a 10k buffer.  Is there a better way?
    // (Note: 2k proved to be fine on Mac, but too little on Linux)
    uintptr_t stackLimitOffset = STACK_SIZE - 10*1024;
    uintptr_t stackLimit = (((uintptr_t)&stackLimitOffset) +
                             stackLimitOffset * JS_STACK_GROWTH_DIRECTION);

    const uint32_t spins[] = {1<<9, 1<<10, 1<<11, 1<<12, 1<<13, 1<<14, 1<<15, 1<<16, 0};

    uint32_t mark = 0;
    while (blockUntilPoolIsActiveOrTerminating()) {
        uint32_t spinCount = 0;
        while (spins[spinCount]) {
            for (uint32_t i = 0; i < spins[spinCount]; i++)
                if (pool_.workCounter_ != mark)
                    break;
            spinCount++;

            // Check whether new work is available. Note the acquire
            // barrier, which guarantees that we wait until observing
            // new value of `workCounter_` before reading `executor_`
            // field etc:
            uint32_t workCounter = JS_ATOMIC_LOAD_ACQUIRE(&pool_.workCounter_);
            if (workCounter != mark) {
                TaskExecutor *executor = pool_.executor_;
                JS_ASSERT(executor);

#ifdef DEBUG
                // We decrement activeWorkers_ at the point where we
                // have observed and read in all the details about the
                // current work item, but before it has necessarily
                // been completed.  This allows for maximum overlap
                // between successive users, while still retaining the
                // invariant that there is at most one outstanding
                // work item at a time, see detailed discussion above.
                uint32_t aw = JS_ATOMIC_DECREMENT(&pool_.activeWorkers_);
#endif

                executor->executeFromWorker(id_, stackLimit);

                PRINTF("run(%d): executed work item %d\n",
                       id_, workCounter);

                spinCount = 0;
                mark = workCounter;
            }
        }
    }

    PRINTF("run(%d): terminating\n", id_);

    // Decrement count and notify main thread if we are the last to
    // terminate. In this case, `notify()` (vs `notifyAll()`) is
    // sufficient because the only one left to be waiting is the
    // thread which dropped the last ref on the pool.
    if (JS_ATOMIC_DECREMENT(&pool_.activeWorkers_) == 0) {
        AutoLockMonitor lock(pool_);
        lock.notify();
    }

    delete this;
}

bool
js::ForkJoinPoolWorker::blockUntilPoolIsActiveOrTerminating()
{
    AutoLockMonitor lock(pool_);
    while (true) {
        switch (pool_.state_) {
          case js::ForkJoinPool::PoolActive:
            return true;
          case js::ForkJoinPool::PoolTerminating:
            return false;
          case js::ForkJoinPool::PoolIdle:
            // Wait for `terminateThreads()` or `acquire()` to notify
            // us when pool changes from idle to active/terminating
            // state. Note that we may also be (spuriously) awoken by
            // a call to `release()`.
            lock.wait();
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////
// AutoForkJoinPool

js::AutoForkJoinPool::AutoForkJoinPool(ForkJoinPool &pool)
  : pool_(pool)
{
}

js::AutoForkJoinPool::~AutoForkJoinPool()
{
    pool_.release();
}

bool
js::AutoForkJoinPool::init(JSContext *cx)
{
    if (!pool_.acquire()) {
        js_ReportOutOfMemory(cx);
        return false;
    }
    return true;
}

void
js::AutoForkJoinPool::submit(TaskExecutor *executor)
{
    pool_.submit(executor);
}

///////////////////////////////////////////////////////////////////////////
// Public interface

js::ForkJoinPool *
js::CreateForkJoinPool(size_t numWorkers)
{
    ForkJoinPool *pool = new ForkJoinPool(numWorkers);
    if (!pool->init()){
        delete pool;
        return NULL;
    }
    return pool;
}

js::ForkJoinPool *
js::AddForkJoinPoolRef(ForkJoinPool *pool)
{
    return pool->addRef();
}

void
js::DropForkJoinPool(ForkJoinPool *pool)
{
    pool->dropRef();
}

uint32_t
js::ForkJoinPoolWorkers(ForkJoinPool *pool)
{
    return pool->numWorkers();
}

#endif /* JS_THREADSAFE */
