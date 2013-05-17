/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdarg.h>

#include "PerfSpewer.h"
#include "IonSpewer.h"
#include "LIR.h"
#include "MIR.h"
#include "MIRGraph.h"
#include "LinearScan.h"
#include "RangeAnalysis.h"
using namespace js;
using namespace js::ion;

PerfSpewer::PerfSpewer()
  : fp_(NULL)
{
    if (!IonPerfEnabled())
        return;

    // perf expects its data to be in a file /tmp/perf-PID.map
    const ssize_t bufferSize = 256;
    char filenameBuffer[bufferSize];
    if (snprintf(filenameBuffer, bufferSize,
                 "/tmp/perf-%d.map",
                 getpid()) >= bufferSize)
        return;

    fp_ = fopen(filenameBuffer, "a");
    if (!fp_)
        return;
}

PerfSpewer::~PerfSpewer()
{
    if (fp_)
        fclose(fp_);
}

bool
PerfSpewer::startBasicBlock(MBasicBlock *blk,
                            MacroAssembler &masm)
{
    if (!IonSpewEnabled(IonSpew_PerfBlock) || !fp_)
        return true;

    const char *filename = blk->info().script()->filename();
    unsigned lineNumber, columnNumber;
    if (blk->pc()) {
        lineNumber = PCToLineNumber(blk->info().script(),
                                    blk->pc(),
                                    &columnNumber);
    } else {
        lineNumber = 0;
        columnNumber = 0;
    }
    Record r(filename, lineNumber, columnNumber, blk->id());
    masm.bind(&r.start);
    return basicBlocks_.append(r);
}

bool
PerfSpewer::endBasicBlock(MacroAssembler &masm)
{
    masm.bind(&basicBlocks_[basicBlocks_.length() - 1].end);
    return true;
}

void
PerfSpewer::writeRecordedBasicBlocks(JSScript *script,
                                     IonCode *code,
                                     MacroAssembler &masm)
{
    if (!fp_)
        return;

    if (IonSpewEnabled(IonSpew_PerfFunc)) {
        fprintf(fp_,
                "%lx %lx %s:%d\n",
                (unsigned long) code->raw(),
                code->instructionsSize(),
                script->filename(),
                script->lineno);
    } else if (IonSpewEnabled(IonSpew_PerfBlock)) {
        unsigned long funcStart = (unsigned long) code->raw();
        unsigned long funcEnd = funcStart + code->instructionsSize();

        unsigned long cur = funcStart;
        for (uint32_t i = 0; i < basicBlocks_.length(); i++) {
            Record &r = basicBlocks_[i];

            unsigned long blockStart = funcStart + masm.actualOffset(r.start.offset());
            unsigned long blockEnd = funcStart + masm.actualOffset(r.end.offset());

            JS_ASSERT(cur <= blockStart);
            if (cur < blockStart) {
                fprintf(fp_,
                        "%lx %lx %s:%d-Mid\n",
                        cur, blockStart - cur,
                        script->filename(),
                        script->lineno);
            }
            cur = blockEnd;

            fprintf(fp_,
                    "%lx %lx Block-%d(%s:%d:%d)\n",
                    blockStart, blockEnd - blockStart,
                    r.id, r.filename, r.lineNumber, r.columnNumber);
        }

        JS_ASSERT(cur <= funcEnd);
        if (cur < funcEnd) {
            fprintf(fp_,
                    "%lx %lx %s:%d-Tail\n",
                    cur, funcEnd - cur,
                    script->filename(),
                    script->lineno);
        }
    }
}
