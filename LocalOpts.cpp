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

bool multiInstructionOptimization(BasicBlock &B) {
  for(auto &I : B) {
    Instruction &Inst = I;
//    outs() << "---------------\n";
    if(Inst.getOpcode() == 17) return false;
    if(Inst.getNextNode() != nullptr) {
      unsigned int opcode = -1;
      ConstantInt *value = nullptr;
      Value *var = nullptr;

      if(Inst.getOpcode() == Instruction::Add) opcode=Instruction::Sub;
      if(Inst.getOpcode() == Instruction::Sub) opcode=Instruction::Add;
//      outs() << "#####: tipo di operazione " << Inst.getOpcode() << " \n";
//      outs() << "#####: operazione cercata " << opcode << " \n";
      int i = 1;
      for(auto *Iter = Inst.op_begin(); Iter != Inst.op_end(); ++Iter) {
        if(ConstantInt *C = dyn_cast<ConstantInt>(Iter)) {
          value = C;
          var = Inst.getOperand(i);
        }
        i--;
      }
//      outs() << "#####: valore constante " << value->getValue() << " \n";
      for(User *U : Inst.users()) {
        Instruction *InstJ = dyn_cast<Instruction>(U);
//        outs() << "\n#####: cast " << InstJ << " \n";
        if(! (InstJ) ) break;
//        outs() << "#####: operazione " << InstJ->getOpcode() << " \n";
        if(InstJ->getOpcode() == opcode) {
//          outs() << "#####: operazione trovata \n";
          for(auto *Iter = InstJ->op_begin(); Iter != InstJ->op_end(); ++Iter) {
            if(ConstantInt *C = dyn_cast<ConstantInt>(Iter)) {
              if(C != value) break;
              APInt app(32, 0);
              ConstantInt *CC = ConstantInt::get(value->getType()->getContext(), app);
              Instruction *TempInst = BinaryOperator::Create(Instruction::Add, var, CC);
              TempInst->insertAfter(InstJ);
              InstJ->replaceAllUsesWith(TempInst);
            }
          }
        }
      }
    }
  }
  return true;
}

bool runOnBasicBlock(BasicBlock &B) {
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
    //if (runOnBasicBlock(*Iter)) {
    //  Transformed = true;
    //}
    multiInstructionOptimization(*Iter);
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

