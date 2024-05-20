//===-- LocalOpts.cpp - Example Transformations --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#include "llvm/Transforms/Utils/LoopWalk.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/IRBuilder.h"
#include <llvm/IR/Constants.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Dominators.h>
using namespace llvm;

// Funzione che controlla se un'istruzione è LoopInvariant
bool isLoopInvariant(Instruction& Inst, Loop& L, const std::vector <Instruction*> &invariantInstructions) {
	bool isInvariant = true;
	for (auto* Iter = Inst.op_begin(); Iter != Inst.op_end(); ++Iter) {
		//ottengo l'operando
		Value* Operand = *Iter;
		if (ConstantInt* C = dyn_cast<ConstantInt>(Operand)) { //controllo se è una costante
			isInvariant = true; //se così fosse forse è invariant
		}
		else if (Instruction* I = dyn_cast<Instruction>(Operand)) { //controllo l'istruzione generatrice
			//ottengo il BB dell'istruzione e controllo se è nel loop
			if (L.contains(I->getParent())) {
				isInvariant = false; //se è nel Loop forse è false
				for (const Instruction* inst : invariantInstructions) { //ma controllo anche se l'istruzione da cui dipende è LI
					if (inst == I) {
						isInvariant = true;
						//se l'istruzione generatrice è LI allora anche questa lo è, quindi LI true
					}
				}
			}
			else
				isInvariant = true;
		}
		if (!isInvariant)
			return false;
	}
	return true;
}

//funzione che restituisce true se tutte le uscite sono dominate dal BB
bool dominatesAllExit(BasicBlock* BB, const SmallVector<BasicBlock*>& exitBlocks, DominatorTree& DT) {
	//per ogni BasicBlock fuori dal loop
	for (BasicBlock* BB_succ : exitBlocks) {
		if (!DT.properlyDominates(BB, BB_succ)) {
			return false; //se non lo domino ritorno false
		}
	}
	//se li domino tutti ritorno true
	return true;
}

//controllo se il blocco dell'istruzione domina tutti i blocchi delle istruzioni che la usano
bool dominatesAllUses(Instruction* I, Loop& L, DominatorTree& DT) {
	BasicBlock* BB = I->getParent(); //ottengo il BasicBlock che contiene l'istruzione
	Value* assignedValue = I; //faccio un cast a Value, così da poterne prendere gli users
	for (User* U : assignedValue->users()) { //per ogni user che usa l'istruzione
		if (Instruction* UserInst = dyn_cast<Instruction>(U)) { //controllo che sia un'istruzione
            		llvm::BasicBlock *UserBB = UserInst->getParent();
            		// Controllo se l'uso si trova dentro il loop.
            		if (L.contains(UserBB)) {
                	// Verifico se il blocco BB domina il blocco del punto d'uso.
                		if (!DT.properlyDominates(BB, UserBB)) {
                    			// Se non domina, ritorna false.
                    			return false;
                		}
            		}
        	}
    	}
    	//se sono arrivato a questo punto vuol dire che il BB domina tutti gli usi
	return true;
}

//controllo che tutte le dipendenze di un'istruzione siano nel vettore di istruzioni da spostare
bool dependenciesMoved(Instruction* I, const std::vector<Instruction*>& candidateInst, Loop& L)  {
// per ogni operando dell'istruzione
	for (Value* Op : I->operands()) {
		if (Instruction* OpInst = dyn_cast<Instruction>(Op)) { //controllo che sia un'istruzione
		    if (L.contains(OpInst)){ //se l'itruzione è nel loop
		    	    bool found = false;   
			    for (Instruction* InvariantInst : candidateInst) { //controllo che sia da spostare
				if (InvariantInst == OpInst) {
				    found = true;
				    break;  // L'istruzione invariante è stata trovata, quindi cercherò per il prossimo operando
				}
			    }
			    if (!found) {
				return false;  // Se non trovo l'istruzione invariante, ritorno false
			    }
		    }		    	
		}
    	}
    return true;  // Le istruzioni di ogni operando sono state trovate nel vettore, quindi ritorno true
}

PreservedAnalyses LoopAss::run(Loop &L, LoopAnalysisManager &LAM, LoopStandardAnalysisResults &LAR, LPMUpdater &LU) {
	std::vector <Instruction*> invariantInstructions; // = new vector<Instruction>();
	for (auto BI = L.block_begin(); BI != L.block_end(); ++BI) {
		BasicBlock* BB = *BI;
		for (auto& Inst : *BB){
			if (isLoopInvariant(Inst, L, invariantInstructions))
				invariantInstructions.push_back(&Inst); // gli passo l'istruzione stessa, e non l'indirizzo
		}
	}
	
//Con il metodo di L ottengo tutti i blocchi successoti del loop
	SmallVector<BasicBlock*> exitBlocks;
	L.getExitBlocks(exitBlocks);

	//ciclo per controllare le istruzioni candidate alla code motion
	std::vector <Instruction*> candidateInst;
	DominatorTree& DT = LAR.DT;
	//Per ogni istruzione LoopInvariant
	for (auto* I : invariantInstructions) { 
		//ottengo il BasicBlock in cui si trova
		BasicBlock* BB = I->getParent();
		//se tale BB domina le uscite e gli usi
		if (dominatesAllExit(BB, exitBlocks, DT) && dominatesAllUses(I, L, DT))
			candidateInst.push_back(I); //la aggiungo come istruzione candidata
	}
	
	//sposto le istruzioni nel preheader
	BasicBlock* PreHeader = L.getLoopPreheader();
	for (Instruction* I : candidateInst) {
		if (dependenciesMoved(I, candidateInst, L)) {
			I->moveBefore(PreHeader->getTerminator());
		}
	}
	return PreservedAnalyses::all();
}

