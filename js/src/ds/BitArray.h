/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef ds_BitArray_h
#define ds_BitArray_h

#include "mozilla/TemplateLib.h"

#include <limits.h>

#include "jstypes.h"

namespace js {

template <size_t nbits>
class BitArray {
  private:
    static const size_t bitsPerElement = sizeof(uintptr_t) * CHAR_BIT;
    static const size_t numSlots =
        nbits / bitsPerElement + (nbits % bitsPerElement == 0 ? 0 : 1);
    uintptr_t map[numSlots];

  public:
    void clear(bool value) {
        memset(map, value ? 0xFF : 0, sizeof(map));
    }

    inline bool get(size_t offset) const {
        uintptr_t index, mask;
        getMarkWordAndMask(offset, &index, &mask);
        return map[index] & mask;
    }

    inline void set(size_t offset) {
        uintptr_t index, mask;
        getMarkWordAndMask(offset, &index, &mask);
        map[index] |= mask;
    }

    inline void unset(size_t offset) {
        uintptr_t index, mask;
        getMarkWordAndMask(offset, &index, &mask);
        map[index] &= ~mask;
    }

    bool isAllClear() const {
        for (size_t i = 0; i < numSlots; i++) {
            if (map[i])
                do { printf("Fail %s:%d\n", __FILE__, __LINE__); return false; } while(false);
        }
        return true;
    }

  private:
    inline void getMarkWordAndMask(size_t offset,
                                   uintptr_t *indexp, uintptr_t *maskp) const {
        static_assert(bitsPerElement == 32 || bitsPerElement == 64,
                      "unexpected bitsPerElement value");
        *indexp = offset / bitsPerElement;
        *maskp = uintptr_t(1) << (offset % bitsPerElement);
    }
};

} /* namespace js */

#endif /* ds_BitArray_h */
