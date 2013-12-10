/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef jit_MoveResolver_h
#define jit_MoveResolver_h

#include "jit/InlineList.h"
#include "jit/IonAllocPolicy.h"
#include "jit/Registers.h"

namespace js {
namespace jit {

class MoveResolver
{
  public:
    // This is similar to Operand, but carries more information. We're also not
    // guaranteed that Operand looks like this on all ISAs.
    class MoveOperand
    {
      public:
        enum Kind {
            REG,
            FLOAT_REG,
            SIMD128_REG,
            ADDRESS,
            FLOAT_ADDRESS,
            SIMD128_ADDRESS,
            EFFECTIVE_ADDRESS
        };

        Kind kind_;
        uint32_t code_;
        int32_t disp_;

      public:
        MoveOperand()
        { }
        explicit MoveOperand(const Register &reg) : kind_(REG), code_(reg.code())
        { }
        explicit MoveOperand(const FloatRegister &reg, Kind kind) : kind_(kind), code_(reg.code())
        {
            JS_ASSERT(kind == FLOAT_REG || kind == SIMD128_REG);
        }
        MoveOperand(const Register &reg, int32_t disp, Kind kind)
            : kind_(kind),
            code_(reg.code()),
            disp_(disp)
        {
            JS_ASSERT(kind == ADDRESS ||
                      kind == FLOAT_ADDRESS ||
                      kind == SIMD128_ADDRESS ||
                      kind == EFFECTIVE_ADDRESS);

            // With a zero offset, this is a plain reg-to-reg move.
            if (disp == 0 && kind == EFFECTIVE_ADDRESS)
                kind_ = REG;
        }
        MoveOperand(const MoveOperand &other)
          : kind_(other.kind_),
            code_(other.code_),
            disp_(other.disp_)
        { }
        bool isFloatReg() const {
            return kind_ == FLOAT_REG;
        }
        bool isGeneralReg() const {
            return kind_ == REG;
        }
        bool isSIMD128Reg() const {
            return kind_ == SIMD128_REG;
        }
        bool isMemory() const {
            return kind_ == ADDRESS;
        }
        bool isFloatAddress() const {
            return kind_ == FLOAT_ADDRESS;
        }
        bool isSIMD128Address() const {
            return kind_ == FLOAT_ADDRESS;
        }
        bool isEffectiveAddress() const {
            return kind_ == EFFECTIVE_ADDRESS;
        }
        Register reg() const {
            JS_ASSERT(isGeneralReg());
            return Register::FromCode(code_);
        }
        FloatRegister floatReg() const {
            JS_ASSERT(isFloatReg());
            return FloatRegister::FromCode(code_);
        }
        Register base() const {
            JS_ASSERT(isMemory() || isEffectiveAddress() || isFloatAddress() || isSIMD128Address());
            return Register::FromCode(code_);
        }
        int32_t disp() const {
            JS_ASSERT(isMemory() || isEffectiveAddress() || isFloatAddress() || isSIMD128Address());
            return disp_;
        }
        Kind kind() const {
            return kind_;
        }

        bool operator ==(const MoveOperand &other) const {
            if (kind_ != other.kind_)
                return false;
            if (code_ != other.code_)
                return false;
            if (isMemory() || isEffectiveAddress() || isFloatAddress() || isSIMD128Address())
                return disp_ == other.disp_;
            return true;
        }
        bool operator !=(const MoveOperand &other) const {
            return !operator==(other);
        }
    };

    class Move
    {
      protected:
        MoveOperand from_;
        MoveOperand to_;
        bool cycle_;

      public:
        enum Kind {
            GENERAL,
            DOUBLE,
            SIMD128
        };

      protected:
        Kind kind_;

      public:
        Move()
        { }
        Move(const Move &other)
          : from_(other.from_),
            to_(other.to_),
            cycle_(other.cycle_),
            kind_(other.kind_)
        { }
        Move(const MoveOperand &from, const MoveOperand &to, Kind kind, bool cycle = false)
          : from_(from),
            to_(to),
            cycle_(cycle),
            kind_(kind)
        { }

        bool inCycle() const {
            return cycle_;
        }
        const MoveOperand &from() const {
            return from_;
        }
        const MoveOperand &to() const {
            return to_;
        }
        Kind kind() const {
            return kind_;
        }
    };

  private:
    struct PendingMove
      : public Move,
        public TempObject,
        public InlineListNode<PendingMove>
    {
        PendingMove()
        { }
        PendingMove(const MoveOperand &from, const MoveOperand &to, Kind kind)
          : Move(from, to, kind, false)
        { }
        
        void setInCycle() {
            JS_ASSERT(!inCycle());
            cycle_ = true;
        }

    };

    typedef InlineList<MoveResolver::PendingMove>::iterator PendingMoveIterator;

  private:
    // Moves that are definitely unblocked (constants to registers). These are
    // emitted last.
    js::Vector<Move, 16, SystemAllocPolicy> orderedMoves_;
    bool hasCycles_;

    TempObjectPool<PendingMove> movePool_;

    InlineList<PendingMove> pending_;

    PendingMove *findBlockingMove(const PendingMove *last);

    // Internal reset function. Does not clear lists.
    void resetState();

  public:
    MoveResolver();

    // Resolves a move group into two lists of ordered moves. These moves must
    // be executed in the order provided. Some moves may indicate that they
    // participate in a cycle. For every cycle there are two such moves, and it
    // is guaranteed that cycles do not nest inside each other in the list.
    //
    // After calling addMove() for each parallel move, resolve() performs the
    // cycle resolution algorithm. Calling addMove() again resets the resolver.
    bool addMove(const MoveOperand &from, const MoveOperand &to, Move::Kind kind);
    bool resolve();

    size_t numMoves() const {
        return orderedMoves_.length();
    }
    const Move &getMove(size_t i) const {
        return orderedMoves_[i];
    }
    bool hasCycles() const {
        return hasCycles_;
    }
    void clearTempObjectPool() {
        movePool_.clear();
    }
    void setAllocator(TempAllocator &alloc) {
        movePool_.setAllocator(alloc);
    }
};

} // namespace jit
} // namespace js

#endif /* jit_MoveResolver_h */
