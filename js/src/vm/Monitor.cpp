/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "vm/Monitor.h"

using namespace js;

bool
Monitor::init()
{
#ifdef JS_THREADSAFE
    lock_ = PR_NewLock();
    if (!lock_)
        do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);

    condVar_ = PR_NewCondVar(lock_);
    if (!condVar_)
        do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);
#endif

    return true;
}
