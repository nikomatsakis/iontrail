/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef jslock_h__
#define jslock_h__

#include "jsapi.h"

#ifdef JS_THREADSAFE

# include "pratom.h"
# include "prlock.h"
# include "prcvar.h"
# include "prthread.h"
# include "prinit.h"

// Equivalent to ++*p. Yields result of increment. Full barrier
// semantics guarantee that memory accesses cannot move across this
// increment.
# define JS_ATOMIC_INCREMENT(p)      PR_ATOMIC_INCREMENT((int32_t *)(p))

// Equivalent to --*p. Yields result of decrement. Full barrier
// semantics guarantee that memory accesses cannot move across this
// decrement.
# define JS_ATOMIC_DECREMENT(p)      PR_ATOMIC_DECREMENT((int32_t *)(p))

// Equivalent to *p = *p + v. Yields result of addition. Full barrier
// semantics guarantee that memory accesses cannot move across this
// increment.
# define JS_ATOMIC_ADD(p,v)          PR_ATOMIC_ADD((int32_t *)(p), (int32_t)(v))

// Assigns v to *p and yields old value of *p. Full barrier semantics
// guarantee that memory accesses cannot move across this assignment.
# define JS_ATOMIC_SET(p,v)          PR_ATOMIC_SET((int32_t *)(p), (int32_t)(v))

// Equivalent to *p = v. Release semantics guarantee that memory
// accesses cannot move from before the store to afterwards.  Used to
// write a flag that indicates when a condition has occurred (e.g., a
// signal is sent).
# define JS_ATOMIC_STORE_RELEASE(p,v) \
    __atomic_store_n((p), (v), __ATOMIC_RELEASE)

// Equivalent to *p. Acquire semantics guarantee that memory accesses
// cannot be hoisted from after the store to before.  Used to read a
// flag that will indicate when some condition has occurred (e.g., a
// signal was sent).
# define JS_ATOMIC_LOAD_ACQUIRE(p) \
    __atomic_load_n((p), __ATOMIC_ACQUIRE)

namespace js {
    // Defined in jsgc.cpp.
    unsigned GetCPUCount();
}

#else  /* JS_THREADSAFE */

typedef struct PRThread PRThread;
typedef struct PRCondVar PRCondVar;
typedef struct PRLock PRLock;

# define JS_ATOMIC_INCREMENT(p)       (++*(p))
# define JS_ATOMIC_DECREMENT(p)       (--*(p))
# define JS_ATOMIC_ADD(p,v)           (*(p) += (v))
# define JS_ATOMIC_STORE_RELEASE(p,v) (*(p) = (v))
# define JS_ATOMIC_LOAD_ACQUIRE(p)    (*(p))

static inline int32_t
JS_ATOMIC_SET(int32_t *val, int32_t newval) {
    int32_t r = *val;
    *val = newval;
    return r;
}

#endif /* JS_THREADSAFE */

#endif /* jslock_h___ */
