/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef jit_StackSlotAllocator_h
#define jit_StackSlotAllocator_h

#include "jit/Registers.h"

namespace js {
namespace jit {

class StackSlotAllocator
{
    js::Vector<uint32_t, 4, SystemAllocPolicy> normalSlots;
    js::Vector<uint32_t, 4, SystemAllocPolicy> doubleSlots;
    js::Vector<uint32_t, 4, SystemAllocPolicy> simd128Slots;
    uint32_t height_;

  public:
    StackSlotAllocator() : height_(0)
    { }

    void freeSlot(uint32_t index) {
        normalSlots.append(index);
    }
    void freeDoubleSlot(uint32_t index) {
        doubleSlots.append(index);
    }
    void freeSIMD128Slot(uint32_t index) {
        simd128Slots.append(index);
    }
    void freeValueSlot(uint32_t index) {
        freeDoubleSlot(index);
    }

    uint32_t allocateDoubleSlot() {
        if (!doubleSlots.empty())
            return doubleSlots.popCopy();
        if (ComputeByteAlignment(height_, DOUBLE_STACK_ALIGNMENT))
            normalSlots.append(++height_);
        height_ += (sizeof(double) / STACK_SLOT_SIZE);
        return height_;
    }
    uint32_t allocateSIMD128Slot() {
        if (!simd128Slots.empty())
            return simd128Slots.popCopy();
        if (ComputeByteAlignment(height_, SIMD128_STACK_ALIGNMENT))
            normalSlots.append(++height_);
        height_ += (16 / STACK_SLOT_SIZE);
        return height_;
    }
    uint32_t allocateSlot() {
        if (!normalSlots.empty())
            return normalSlots.popCopy();
        if (!doubleSlots.empty()) {
            uint32_t index = doubleSlots.popCopy();
            normalSlots.append(index - 1);
            return index;
        }
        if (!simd128Slots.empty()) {
            uint32_t index = simd128Slots.popCopy();
            normalSlots.append(index - 1);
            return index;
        }
        return ++height_;
    }
    uint32_t allocateValueSlot() {
        return allocateDoubleSlot();
    }
    uint32_t stackHeight() const {
        return height_;
    }
};

} // namespace jit
} // namespace js

#endif /* jit_StackSlotAllocator_h */
