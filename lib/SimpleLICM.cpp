/* SimpleLICM.cpp
 *
 * Basic loop-invariant code motion (LICM) example for LLVM-Tutor.
 * Only hoists register-to-register, side-effect-free instructions.
 */

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/CFG.h"

#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/ValueTracking.h"

#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

using namespace llvm;

static bool isRegToRegInvariantCandidate(Instruction *I, Loop *L) {
  if (isa<PHINode>(I)) return false;
  if (I->isTerminator()) return false;
  if (I->mayReadOrWriteMemory()) return false;
  if (I->mayHaveSideEffects()) return false;
  if (isa<AllocaInst>(I)) return false;
  return true;
}

static bool operandsAreInvariant(Instruction *I, Loop *L,
                                 const SmallPtrSetImpl<Instruction *> &InvariantSet) {
  for (Value *Op : I->operands()) {
    if (isa<Constant>(Op)) continue;
    if (auto *OpI = dyn_cast<Instruction>(Op)) {
      if (L->contains(OpI) && !InvariantSet.count(OpI))
        return false;
    }
  }
  return true;
}

struct SimpleLICM : public PassInfoMixin<SimpleLICM> {
  PreservedAnalyses run(Loop &L, LoopAnalysisManager &AM,
                        LoopStandardAnalysisResults &AR,
                        LPMUpdater &) {
    DominatorTree &DT = AR.DT;
    BasicBlock *Preheader = L.getLoopPreheader();
    if (!Preheader) {
      errs() << "No preheader, skipping loop\n";
      return PreservedAnalyses::all();
    }


    SmallPtrSet<Instruction *, 32> InvariantSet;
    bool changed = true;
    while (changed) {
      changed = false;
      for (BasicBlock *BB : L.blocks()) {
        for (Instruction &InstRef : *BB) {
          Instruction *I = &InstRef;
          if (!isRegToRegInvariantCandidate(I, &L)) continue;
          if (InvariantSet.count(I)) continue;
          if (operandsAreInvariant(I, &L, InvariantSet)) {
            InvariantSet.insert(I);
            changed = true;
          }
        }
      }
    }


    SmallPtrSet<Instruction *, 32> Moved;
    for (Instruction *I : InvariantSet) {
      if (!isSafeToSpeculativelyExecute(I)) continue;
      if (!dominatesAllLoopExits(I, &L, DT)) continue;
      errs() << "Hoisting: " << *I << "\n";
      I->moveBefore(Preheader->getTerminator());
      Moved.insert(I);
    }

    return PreservedAnalyses::none();
  }

  bool dominatesAllLoopExits(Instruction *I, Loop *L, DominatorTree &DT) {
    SmallVector<BasicBlock *, 8> ExitBlocks;
    L->getExitBlocks(ExitBlocks);
    for (BasicBlock *EB : ExitBlocks)
      if (!DT.dominates(I, EB))
        return false;
    return true;
  }
};


llvm::PassPluginLibraryInfo getSimpleLICMPluginInfo() {
  errs() << "SimpleLICM plugin loaded\n";
  return {LLVM_PLUGIN_API_VERSION, "simple-licm", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, LoopPassManager &LPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "simple-licm") {
                    LPM.addPass(SimpleLICM());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getSimpleLICMPluginInfo();
}
