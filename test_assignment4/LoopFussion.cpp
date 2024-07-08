#include "llvm/Transforms/Utils/LoopFussion.h"
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Dominators.h>
#include <llvm/ADT/DepthFirstIterator.h>
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
using namespace llvm;

bool areAdjacent(Loop *Lj, Loop *Lk) {
        SmallVector<BasicBlock *, 4> ExitJ;
        Lj->getUniqueNonLatchExitBlocks(ExitJ); // ottengo i blocchi d'uscita non successori del latch
        for(auto *BB : ExitJ) {
          if(Lk->isGuarded() and BB != dyn_cast<BasicBlock>(Lk->getLoopGuardBranch())){
            outs() << "loop guarded\n";
            return false;
          }
          if(!Lk->isGuarded() and Lk->contains(BB)) return false;
        }
        return true;
  }

bool areControlFlowEquivalent(Loop *Lj, Loop *Lk, DominatorTree &DT, PostDominatorTree &PDT) {
    // Verifica se Lj domina Lk e se Lk post-domina Lj
    SmallVector<BasicBlock *, 4> ExitJ;
    Lj->getExitingBlocks(ExitJ); //prendo i blocchi nel loop che hanno successori al di fuori
    for(auto *BB : ExitJ){
        if(BB->getTerminator()->getNumSuccessors()==0) outs() << "niente successori\n";
        else {
          BasicBlock *nextb = BB->getTerminator()->getSuccessor(0);
          //controllo se il successore è al di fuori del loop
          if(!Lk->contains(nextb)){
              if(!DT.dominates(BB, nextb)) outs() << "non domina\n";
              if(!PDT.dominates(nextb, BB)) outs() << "non domina\n";
              if(!DT.dominates(BB, nextb) || !PDT.dominates(nextb, BB)) return false;
          }
          else outs() << "blocco nel loop 1\n";
        }
    }

    return true;
  }

bool haveSameIterationCount(Loop *Lj, Loop *Lk, ScalarEvolution &SE) {
    // Verifica se Lj e Lk hanno lo stesso numero di iterazioni

    // In caso di esito positivo, restituisce un SCEVContant maggiore o uguale al valore returend di getBackedgeTakenCount. Se tale valore non può
    // essere calcolato, restituisce l'oggetto SCEVCouldNotCompute.
    const SCEV *TripCountJ = SE.getConstantMaxBackedgeTakenCount(Lj);
    const SCEV *TripCountK = SE.getConstantMaxBackedgeTakenCount(Lk);

    // Controllo che i metodi precedenti non abbiano ritornato SCEVCouldNotCompute
    if (isa<SCEVCouldNotCompute>(TripCountJ) || isa<SCEVCouldNotCompute>(TripCountK)) {
        outs() << "SCEVCould not compute\n";
        return false;
    }
    //estraggo i valori
    const APInt &ValueJ = cast<SCEVConstant>(TripCountJ)->getAPInt();
    const APInt &ValueK = cast<SCEVConstant>(TripCountK)->getAPInt();
    // infine controllo se il numero di iterazioni   uguale, con getValue ottengo un ConstantInt, di cui poi ottengo l'intero con getZExtValue
    return ValueJ == ValueK;
  }

bool hasNegativeDistanceDependencies(Loop *Lj, Loop *Lk, DependenceInfo &DA) {
    // Itera attraverso tutte le istruzioni in Lj
    for (auto *BBJ : Lj->blocks()) {
        for (auto &IJ : *BBJ) {
            // Itera attraverso tutte le istruzioni in Lk
            for (auto *BBK : Lk->blocks()) {
                for (auto &IK : *BBK) {
                    // Ottieni la dipendenza tra IJ e IK
                    if (auto Dep = DA.depends(&IJ, &IK, true)) {
                        // ritorna null se non c'è dipendenza, quindi continuo con il ciclo
                        // ora verifichiamo se c'è una distanza negativa
                        for (unsigned Level = 1; Level <= Dep->getLevels(); ++Level) { // itero su tutti i livelli di dipendenza
										       // (ossia attraverso le varie ipotetiche nidificazioni)
                            const SCEV *Distance = Dep->getDistance(Level);
                            if(isa<SCEVConstant>(Distance)){
                                const APInt &DistanceValue = dyn_cast<SCEVConstant>(Distance)->getAPInt();
                                if (DistanceValue.isNegative()) {//ottengo la distanza tra le due istruzioni
                                    // ossia quante iterazioni separano l'uso di una variabile nel secondo loop dalla definizione
                                    // di quella variabile nel primo loop
                                    outs()<<"Distanza a negativa trovata\n";
                                    return true; // se è negativa ritorno true
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return false; // Nessuna dipendenza a distanza negativa trovata
}

bool canFuseLoops(Loop *Lj, Loop *Lk, LoopInfo &LI, DominatorTree &DT, PostDominatorTree &PDT, ScalarEvolution &SE, DependenceInfo &DI) {
    // Condizione 1: Lj e Lk devono essere adiacenti
    if (!areAdjacent(Lj, Lk)) {
      outs() << "non sono adiacenti\n";
      return false;
    }

    // Condizione 2: Lj e Lk devono iterare lo stesso numero di volte
    if (!haveSameIterationCount(Lj, Lk, SE)) {
      outs() << "non hanno lo stesso numero di iterazioni\n";
      return false;
    }

    // Condizione 3: Lj e Lk devono essere equivalenti nel flusso di controllo
    if (!areControlFlowEquivalent(Lj, Lk, DT, PDT)){
        outs() << "non sono control flow equivalent\n";
        return false;
    }
    // Condizione 4: Non ci devono essere dipendenze a distanza negativa
    if (hasNegativeDistanceDependencies(Lj, Lk, DI)) return false;

    return true;
  }

//////////////////////////////////////////////

bool fuseLoops(Loop *Lj, Loop *Lk, LoopInfo &LI, ScalarEvolution &SE, DominatorTree &DT) {
    // Ottenere le variabili di induzione dei loop
    PHINode *IV1 = Lj->getCanonicalInductionVariable();
    PHINode *IV2 = Lk->getCanonicalInductionVariable();
    if (!IV1) {
        outs() << "Impossibile trovare la variabile di induzione di Lj.\n";
        return false;
    }
    if (!IV2) {
        outs() << "Impossibile trovare la variabile di induzione di Lk.\n";
        return false;
    }
    outs() << "Variabili di induzione trovate\n";

    // Sostituire gli usi della variabile di induzione del loop 2
    for (auto *BB : Lk->blocks()) {
        for (auto &I : *BB) {
            for (auto &Op : I.operands()) {
                if (Op == IV2) {
                    Op.set(IV1);
                }
            }
        }
    }
    outs() << "Variabili di induzione cambiate\n";

    // Modificare il CFG per unire i corpi dei loop
    BasicBlock *Header1 = Lj->getHeader();
    BasicBlock *Latch1 = Lj->getLoopLatch();
    BasicBlock *Header2 = Lk->getHeader();
    BasicBlock *Latch2 = Lk->getLoopLatch();
    BasicBlock *Exit2 = Lk->getExitBlock();
    BasicBlock *LastB1 = nullptr, *LastB2 = nullptr, *FirstB2 = nullptr;

    // Ottengo il primo blocco del body del loop 2
    if (BranchInst *Term = dyn_cast<BranchInst>(Header2->getTerminator())) {
        if (Term->isConditional()) {
            FirstB2 = Term->getSuccessor(0);
        }
    }

    // Trovare l'ultimo blocco del corpo del loop 1
    for (BasicBlock *Pred : predecessors(Latch1)) {
        if (Lj->contains(Pred)) {
            outs() << "LastB1 trovato: " << Pred->getName() << "\n";
            LastB1 = Pred;
            break;
        }
    }

    // Trovare l'ultimo blocco del corpo del loop 2
    for (BasicBlock *Pred : predecessors(Latch2)) {
        if (Lk->contains(Pred)) {
            outs() << "LastB2 trovato: " << Pred->getName() << "\n";
            LastB2 = Pred;
            break;
        }
    }

    // Collegare il body del loop 1 all'header del loop 2
    if (BranchInst *Term = dyn_cast<BranchInst>(Header1->getTerminator())) {
        if (Term->isConditional()) {
            Value *Condition = Term->getCondition();
            BasicBlock *TrueBlock = Term->getSuccessor(0);
            Term->eraseFromParent();
            BranchInst::Create(TrueBlock, Exit2, Condition, Header1);
        } else {
            outs() << "Header1 non ha condizione\n";
        }
    } else {
        outs() << "Terminatore dell'header non trovato\n";
    }

    if(LastB1){
      Instruction *Term = LastB1->getTerminator();
      if(Term){
        Term->eraseFromParent();
        BranchInst::Create(LastB1, FirstB2); //collegare il body del loop 1 al body del loop 2
      }
      else outs() << "term non trovato\n";
    }

    // Rimuovere il branch dal header del loop 2 al body del loop 2
    if(Header2){
      Instruction *Term2 = Header2->getTerminator();
      if(Term2){
        Term2->eraseFromParent();
        BranchInst::Create(Header2, Latch2);  //collegare l'header del loop 2 al latch del loop 2
      }
      else outs() << "term2 non trovato\n";
    }


    // Rimuovere il branch dal body del loop 2 al latch del loop 2
    if(LastB2){
      Instruction *Term4 = LastB2->getTerminator();
      if(Term4){
        Term4->eraseFromParent();
        BranchInst::Create(LastB2, Latch1); // Collegare il body del loop 2 al latch del loop 1
      }
      else outs() << "term4 non trovato\n";
    }

    return true;
}


///////////////////////////////////////////

PreservedAnalyses LoopFussion::run(Function &F, FunctionAnalysisManager &FAM) {
  outs() << "Inizio esecuzione\n";
  LoopInfo &LI = FAM.getResult<LoopAnalysis>(F);
  outs() << "LoopInfo corretto\n";
  DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);
  outs() << "DominatorTRee corretto\n";
  PostDominatorTree &PDT = FAM.getResult<PostDominatorTreeAnalysis>(F);
  outs() << "PostDominatorTree corretto\n";
  ScalarEvolution &SE = FAM.getResult<ScalarEvolutionAnalysis>(F);
  outs() << "ScalarEvolution corretto\n";
  DependenceInfo &DI = FAM.getResult<DependenceAnalysis>(F);
  outs() << "DependenceInfo corretto\n";

  outs() << "inizio ottimizzazione\n";
    for (auto It = LI.begin(), E = LI.end(); It != E; It++) {
      outs()<<"esecuzione loop\n";
      Loop* L = *It;
      if(It == E) break;
      auto nextL = It + 1;
      if(nextL == E) break;
      Loop* L2 = *nextL;
      if (canFuseLoops(L, L2, LI, DT, PDT, SE, DI)) {
        outs() << "Posso fondere i due loop\n";
        fuseLoops(L, L2, LI, SE, DT);
      }
    }

  return PreservedAnalyses::all();
}


