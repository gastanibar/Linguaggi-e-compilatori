#ifndef LLVM_TRANSFORMS_LOOPWALK_H
#define LLVM_TRANSFORMS_LOOPWALK_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Constants.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
namespace llvm {
        class LoopAss : public PassInfoMixin<LoopAss> {
               public : PreservedAnalyses run(Loop &L, LoopAnalysisManager &LAM,
						LoopStandardAnalysisResults &LAR,
						LPMUpdater &LU);
        };
}
#endif

