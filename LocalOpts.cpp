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
#include "llvm/IR/IRBuilder.h"
// L'include seguente va in LocalOpts.h
#include <llvm/IR/Constants.h>
using namespace llvm;

void strengthReduction(Instruction &Inst1st, bool mul){
  int i=1;
  Instruction *ShiftInst;
  for(auto *Iter = Inst1st.op_begin(); Iter != Inst1st.op_end(); ++Iter){
        Value *Op = *Iter;
        if(ConstantInt *C = dyn_cast<ConstantInt>(Op)){
            if(C->getValue().isPowerOf2()){
              auto val = C->getValue().exactLogBase2();
              APInt app(32, val);
              ConstantInt *CC = ConstantInt::get(Inst1st.getOperand(i)->getType()->getContext(), app);
              if(mul)
                ShiftInst = BinaryOperator::Create(Instruction::Shl, Inst1st.getOperand(i), CC);
              else 
                ShiftInst = BinaryOperator::Create(Instruction::AShr, Inst1st.getOperand(i), CC);
              ShiftInst->insertAfter(&Inst1st);
              Inst1st.replaceAllUsesWith(ShiftInst);
            }
        }
        i--;
  }
}

bool algebraicIdentity(Instruction &Inst1st, bool add){
  int i=1;
  for(auto *Iter = Inst1st.op_begin(); Iter != Inst1st.op_end(); ++Iter){
      Value *Op = *Iter;
      if(ConstantInt *C = dyn_cast<ConstantInt>(Op)){
        if((C->getValue() == 0 && add) || (C->getValue() == 1 && !add)){
            Instruction &Inst2st = *(Inst1st.getNextNode());
            AllocaInst *ptr = new AllocaInst(Inst1st.getOperand(i)->getType(),0, "", &Inst2st);
            StoreInst *storeInst = new StoreInst(Inst1st.getOperand(i), ptr, &Inst2st);
            LoadInst *loadInst = new LoadInst(Inst1st.getOperand(i)->getType(), ptr, "", &Inst2st);
            Inst1st.replaceAllUsesWith(loadInst);
            return true;
        }
      }
      i--;
  }
  return false;
}

bool runOnBasicBlock(BasicBlock &B) {
    for(auto &Inst1st : B){ 
        //prima di tutto cerco di ottimizzare una Algebraic Identity
        if(Inst1st.getOpcode() == Instruction::Add)
          algebraicIdentity(Inst1st, true);
        else if(Inst1st.getOpcode() == Instruction::Mul){
          if(!algebraicIdentity(Inst1st, false)) //se eseguo la algebraic identity non faccio la strength reduction
              strengthReduction(Inst1st, true);
        }
        else if(Inst1st.getOpcode() == Instruction::SDiv)
          strengthReduction(Inst1st, false);
        
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

