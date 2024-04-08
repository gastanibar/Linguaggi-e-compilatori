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
#include <cmath>

// L'include seguente va in LocalOpts.h
#include <llvm/IR/Constants.h>
using namespace llvm;
//Dichiarazione della funzione
int is_Near_Power_Of_Two(int num);

bool multiInstructionOptimization(BasicBlock &B) {
  for(auto &I : B) {
    Instruction &Inst = I;
    if(Inst.getOpcode() == 17) return false;
    if(Inst.getNextNode() != nullptr) {
      unsigned int opcode = -1;
      ConstantInt *value = nullptr;
      Value *var = nullptr;

      if(Inst.getOpcode() == Instruction::Add) opcode=Instruction::Sub;
      if(Inst.getOpcode() == Instruction::Sub) opcode=Instruction::Add;
      int i = 1;
      for(auto *Iter = Inst.op_begin(); Iter != Inst.op_end(); ++Iter) {
        if(ConstantInt *C = dyn_cast<ConstantInt>(Iter)) {
          value = C;
          var = Inst.getOperand(i);
        }
        i--;
      }
      for(User *U : Inst.users()) {
        Instruction *InstJ = dyn_cast<Instruction>(U);
        if(! (InstJ) ) break;
        if(InstJ->getOpcode() == opcode) {
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

void strengthReduction(Instruction &Inst1st, bool mul){
  int i=1;
  Instruction *ShiftInst;
  for(auto *Iter = Inst1st.op_begin(); Iter != Inst1st.op_end(); ++Iter){
        Value *Op = *Iter;
        if(ConstantInt *C = dyn_cast<ConstantInt>(Op)){
            if (mul)
		{ int val=is_Near_Power_Of_Two(C->getValue().getSExtValue());
		  if(val != -1)	{
                    APInt app(32, val);
                    ConstantInt *CC = ConstantInt::get(Inst1st.getOperand(i)->getType()->getContext(), app);
                    ShiftInst = BinaryOperator::Create(Instruction::Shl, Inst1st.getOperand(i), CC);
		    ShiftInst->insertAfter(&Inst1st);
		    if (C->getValue().getSExtValue() < pow(2, val)) {
                        Instruction *SubInst = BinaryOperator::Create(Instruction::Sub, ShiftInst, Inst1st.getOperand(i));
                        SubInst->insertAfter(ShiftInst);
			Inst1st.replaceAllUsesWith(SubInst);
                    }       
                    else if (C->getValue().getSExtValue() > pow(2, val)) {
                        Instruction *AddInst = BinaryOperator::Create(Instruction::Add, ShiftInst, Inst1st.getOperand(i));
                        AddInst->insertAfter(ShiftInst);
			Inst1st.replaceAllUsesWith(AddInst);
                    } 
		    else
			    Inst1st.replaceAllUsesWith(ShiftInst);
            }}
            else
                if(C->getValue().isPowerOf2()){
                    auto val = C->getValue().exactLogBase2();
                    APInt app(32, val);
                    ConstantInt *CC = ConstantInt::get(Inst1st.getOperand(i)->getType()->getContext(), app);
                    ShiftInst = BinaryOperator::Create(Instruction::AShr, Inst1st.getOperand(i), CC);
            	    ShiftInst->insertAfter(&Inst1st);
            	    Inst1st.replaceAllUsesWith(ShiftInst);
            }}
        i--;
  }
}

bool runOnBasicBlock(BasicBlock &B) {
    multiInstructionOptimization(*B);
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




int is_Near_Power_Of_Two(int num) {
//Con un numero negativo o nullo non è possibile fare nulla
    if (num <= 0) {
        return -1;
    } 
    double log2num = log2(num); //logaritmo del numero
    int nearestPower = round(log2num); //intero più vicino, ossia l'esponente più vicino
    int nearestPowerValue = pow(2, nearestPower); //valore della potenza più vicina

    if (abs(nearestPowerValue - num) <= 1) { //controllo che sia effettivamente "vicino"
        return nearestPower;
    } else {
        return -1;
    }
}
