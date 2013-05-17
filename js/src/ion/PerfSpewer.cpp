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

    fprintf(fp_, "Startaddr Size Name\n");
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
                "%x %x %s:%d\n",
                (size_t) code->raw(),
                code->instructionsSize(),
                script->filename(),
                script->lineno);
    } else if (IonSpewEnabled(IonSpew_PerfBlock)) {
        size_t cur = (size_t) code->raw();
        size_t funcEnd = cur + code->instructionsSize();

        for (uint32_t i = 0; i < basicBlocks_.length(); i++) {
            Record &r = basicBlocks_[i];

            size_t start = masm.actualOffset(r.start.offset());
            size_t end = masm.actualOffset(r.end.offset());
            size_t size = end - start;

            start += (size_t) code->raw();

            if (cur < start) {
                fprintf(fp_,
                        "%x %x %s:%d\n",
                        cur, start - cur,
                        script->filename(),
                        script->lineno);
            }
            cur = end;

            fprintf(fp_,
                    "%x %x Block-%d(%s:%d:%d)\n",
                    start, size,
                    r.id, r.filename, r.lineNumber, r.columnNumber);
        }

        if (cur < funcEnd) {
            fprintf(fp_,
                    "%x %x %s:%d\n",
                    cur, funcEnd - cur,
                    script->filename(),
                    script->lineno);
        }
    }
}
