#ifndef LLVM_TRANSFORMS_LOOPFUSSION_H
#define LLVM_TRANSFORMS_LOOPFUSSION_H
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/PostDominators.h"

namespace llvm {
    class LoopFussion : public  PassInfoMixin<LoopFussion> {
          public : PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
    };
}
#endif

