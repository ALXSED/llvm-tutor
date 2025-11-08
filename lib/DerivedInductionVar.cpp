/* DerivedInductionVar.cpp 
 *
 * This pass detects derived induction variables using ScalarEvolution.
 *
 * Compatible with New Pass Manager
*/

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/ScalarEvolutionExpander.h"
#include "llvm/ADT/SmallVector.h"

using namespace llvm;

namespace {

class DerivedIVPass : public PassInfoMixin<DerivedIVPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    auto &LI = AM.getResult<LoopAnalysis>(F);
    auto &SE = AM.getResult<ScalarEvolutionAnalysis>(F);

    errs() << "\n=== Derived Induction Variable Pass ===\n";
    errs() << "Function: " << F.getName() << "\n";

    for (Loop *L : LI)
      exploreLoop(L, SE, 0);

    errs() << "=== Eliminating Derived IVs ===\n";

    bool Modified = false;
    for (Loop *L : LI)
      if (removeDerivedIVs(L, SE))
        Modified = true;

    return Modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }

private:
  void exploreLoop(Loop *L, ScalarEvolution &SE, unsigned Depth) {
    if (!L || !L->getHeader())
      return;

    std::string indent(Depth * 2, ' ');
    errs() << indent << "Loop header: " << L->getHeader()->getName()
           << " (depth " << Depth << ")\n";

    for (PHINode &Phi : L->getHeader()->phis()) {
      if (!Phi.getType()->isIntegerTy())
        continue;

      const SCEV *Expr = SE.getSCEV(&Phi);
      if (auto *AR = dyn_cast<SCEVAddRecExpr>(Expr)) {
        if (AR->isAffine()) {
          errs() << indent << "  Detected IV: " << Phi.getName()
                 << " = {" << *AR->getStart() << ", +, "
                 << *AR->getStepRecurrence(SE) << "} <"
                 << L->getHeader()->getName() << ">\n";
        }
      }
    }

    for (Loop *Sub : L->getSubLoops())
      exploreLoop(Sub, SE, Depth + 1);
  }

  void classifyIVs(Loop *L, ScalarEvolution &SE,
                   SmallVectorImpl<PHINode *> &BaseIVs,
                   SmallVectorImpl<PHINode *> &DerivedIVs) {
    BasicBlock *Hdr = L->getHeader();
    if (!Hdr)
      return;

    for (PHINode &Phi : Hdr->phis()) {
      if (!Phi.getType()->isIntegerTy())
        continue;

      const SCEV *Expr = SE.getSCEV(&Phi);
      auto *AR = dyn_cast<SCEVAddRecExpr>(Expr);
      if (!AR || !AR->isAffine())
        continue;

      const SCEV *Step = AR->getStepRecurrence(SE);
      if (isa<SCEVConstant>(Step))
        BaseIVs.push_back(&Phi);
      else
        DerivedIVs.push_back(&Phi);
    }
  }

  bool removeDerivedIVs(Loop *L, ScalarEvolution &SE) {
    if (!L || !L->getHeader())
      return false;

    bool Changed = false;
    BasicBlock *Hdr = L->getHeader();

    errs() << "\nProcessing loop: " << Hdr->getName() << "\n";

    Instruction *InsertPt = nullptr;
    if (auto It = Hdr->getFirstNonPHIIt(); It != Hdr->end())
      InsertPt = &*It;
    else
      InsertPt = Hdr->getTerminator();

    if (!InsertPt) {
      errs() << "  No insertion point found; skipping.\n";
      return false;
    }

    Module *M = Hdr->getModule();
    const DataLayout &DL = M->getDataLayout();
    SCEVExpander Exp(SE, DL, "ivrewrite");

    SmallVector<PHINode *, 8> BaseIVs, DerivedIVs;
    classifyIVs(L, SE, BaseIVs, DerivedIVs);

    if (!BaseIVs.empty()) {
      errs() << "  Basic IVs:\n";
      for (PHINode *P : BaseIVs)
        errs() << "    " << P->getName() << "\n";
    }
    if (!DerivedIVs.empty()) {
      errs() << "  Derived IVs:\n";
      for (PHINode *P : DerivedIVs)
        errs() << "    " << P->getName() << "\n";
    }

    SmallVector<PHINode *, 8> Dead;

    for (PHINode *Phi : DerivedIVs) {
      const SCEV *Expr = SE.getSCEV(Phi);
      auto *AR = dyn_cast<SCEVAddRecExpr>(Expr);
      if (!AR || !AR->isAffine())
        continue;

      const SCEV *Step = AR->getStepRecurrence(SE);
      if (auto *C = dyn_cast<SCEVConstant>(Step)) {
        if (C->getAPInt() == 1)
          continue;
      }

      errs() << "    Expanding " << Phi->getName() << " : " << *AR << "\n";
      Value *New = Exp.expandCodeFor(AR, Phi->getType(), InsertPt);
      if (!New) {
        errs() << "      Expansion failed.\n";
        continue;
      }

      Phi->replaceAllUsesWith(New);
      if (Phi->use_empty())
        Dead.push_back(Phi);

      Changed = true;
    }

    for (PHINode *Phi : Dead) {
      errs() << "    Removing dead PHI: " << Phi->getName() << "\n";
      Phi->eraseFromParent();
    }

    for (Loop *Sub : L->getSubLoops())
      if (removeDerivedIVs(Sub, SE))
        Changed = true;

    return Changed;
  }
};

} // namespace

// --- Pass Registration ---
llvm::PassPluginLibraryInfo getDerivedIVPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "DerivedIVPass", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "derived-iv") {
                    FPM.addPass(DerivedIVPass());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getDerivedIVPassPluginInfo();
}
