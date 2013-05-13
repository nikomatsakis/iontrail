/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
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

// Determines whether a function is compatible for parallel execution.
// Removes basic blocks containing unsafe MIR operations from the
// graph and replaces them with MParBailout blocks.
class ParallelArrayAnalysis
{
    MIRGenerator *mir_;
    MIRGraph &graph_;

    bool removeResumePointOperands();
    void replaceOperandsOnResumePoint(MResumePoint *resumePoint, MDefinition *withDef);

  public:
    ParallelArrayAnalysis(MIRGenerator *mir,
                          MIRGraph &graph)
      : mir_(mir),
        graph_(graph)
    {}

    bool analyze();
};

} // namespace ion
} // namespace js

#endif // jsion_parallel_array_analysis_h
