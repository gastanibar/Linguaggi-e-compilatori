//===-- LocalOpts.cpp - Example Transformations --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "llvm/Transforms/Utils/LocalOpts.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/InstrTypes.h"
// L'include seguente va in LocalOpts.h
#include <llvm/IR/Constants.h>
using namespace llvm;

bool runOnBasicBlock(BasicBlock &B) {
    llvm::LLVMContext context;
    for(auto &I : B){ 
    	if(I.getNextNode()!=nullptr){
	    Instruction &Inst1st = I, &Inst2nd = *(I.getNextNode());
	    int i=1; //tengo conto della posizione del registro virtuale
	    if(Inst1st.getOpcode() == Instruction::Mul){
	        for(auto *Iter = Inst1st.op_begin(); Iter != Inst1st.op_end(); ++Iter){
		    Value *Op = *Iter;
		    if(ConstantInt *C = dyn_cast<ConstantInt>(Op)){
			    if(C->getValue().isPowerOf2()){
				    auto val = C->getValue().exactLogBase2();
				    APInt app(32, val);
				    ConstantInt *CC = ConstantInt::get(Inst1st.getOperand(i)->getType()->getContext(), app);
				    Instruction *ShiftInst = BinaryOperator::Create(Instruction::Shl, Inst1st.getOperand(i), CC);
				    ShiftInst->insertAfter(&Inst1st);
				    Inst1st.replaceAllUsesWith(ShiftInst);
	            	    }
		     }
		    i--;
	          }

             }
        }
    }
    return true;
}


bool runOnFunction(Function &F) {
  bool Transformed = false;

  for (auto Iter = F.begin(); Iter != F.end(); ++Iter) {
    if (runOnBasicBlock(*Iter)) {
      Transformed = true;
    }
  }

  return Transformed;
}


PreservedAnalyses LocalOpts::run(Module &M,
                                      ModuleAnalysisManager &AM) {
  for (auto Fiter = M.begin(); Fiter != M.end(); ++Fiter)
    if (runOnFunction(*Fiter))
      return PreservedAnalyses::none();

  return PreservedAnalyses::all();
}

