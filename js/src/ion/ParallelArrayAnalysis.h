/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=4 sw=4 et tw=99:
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef jsion_parallel_array_analysis_h__
#define jsion_parallel_array_analysis_h__

#include "MIR.h"
#include "CompileInfo.h"

namespace js {

class StackFrame;

namespace ion {

class MIRGraph;
class AutoDestroyAllocator;

class ParallelCompileContext
{
  private:
    JSContext *cx_;

    // Is a function compilable for parallel execution?
    bool analyzeAndGrowWorklist(MIRGenerator *mir, MIRGraph &graph);

    bool removeResumePointOperands(MIRGenerator *mir, MIRGraph &graph);
    void replaceOperandsOnResumePoint(MResumePoint *resumePoint, MDefinition *withDef);
    bool appendCallTargetsToWorklist(AutoScriptVector& worklist, IonScript *ion);

  public:
    ParallelCompileContext(JSContext *cx)
      : cx_(cx)
    { }

    bool appendToWorklist(AutoScriptVector& worklist, HandleScript script);

    ExecutionMode executionMode() {
        return ParallelExecution;
    }

    // Defined in Ion.cpp, so that they can make use of static fns defined there
    MethodStatus checkScriptSize(JSContext *cx, RawScript script);
    MethodStatus compileTransitively(HandleScript script);
    AbortReason compile(IonBuilder *builder, MIRGraph *graph,
                        ScopedJSDeletePtr<LifoAlloc> &autoDelete);
};


} // namespace ion
} // namespace js

#endif // jsion_parallel_array_analysis_h
