#include "gv2alloca.h"

#include "llvm/ADT/SetVector.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/Cloning.h"

using namespace llvm;

namespace sc::opt::gv2alloca {
PreservedAnalyses GV2AllocaPass::run(llvm::Module &M,
                                     llvm::ModuleAnalysisManager &MAM) {
  bool Changed = false;
  auto *MainF = M.getFunction("main");
  assert(MainF && "Main function must exist.");

  unsigned AllocaAddrSpace = M.getDataLayout().getAllocaAddrSpace();

  for (auto &GV : make_early_inc_range(M.globals())) {
    Changed = true;
    SmallSetVector<Function *, 8> GVUserFuncs;
    for (auto *U : GV.users()) {
      if (auto *I = dyn_cast<Instruction>(U)) {
        auto *F = I->getFunction();
        if (!GVUserFuncs.contains(F))
          GVUserFuncs.insert(F);
      }
    }
    bool FoundFuncs = true;
    while (FoundFuncs) {
      FoundFuncs = false;
      for (auto *F : GVUserFuncs) {
        for (auto *U : F->users()) {
          assert(isa<CallInst>(U) &&
                 "Functions can only be called according to the spec.");
          auto *CI = cast<CallInst>(U);
          auto *CIF = CI->getFunction();
          if (!GVUserFuncs.contains(CIF)) {
            FoundFuncs = true;
            GVUserFuncs.insert(CIF);
          }
        }
      }
    }

    auto *GVElTy = GV.getType()->getNonOpaquePointerElementType();
    auto *GVAI = new AllocaInst(GVElTy, AllocaAddrSpace, nullptr,
                                GV.getName() + ".gvalloc",
                                MainF->getEntryBlock().getFirstNonPHI());

    SmallDenseMap<Function *, Function *> ReplaceMap;

    for (auto *F : GVUserFuncs) {
      if (F == MainF)
        continue;

      auto *FTy = F->getFunctionType();
      SmallVector<Type *> ParamTypes(FTy->param_begin(), FTy->param_end());
      ParamTypes.push_back(GV.getType());
      auto *NewFTy =
          FunctionType::get(F->getReturnType(), ParamTypes, F->isVarArg());
      auto *NewF = Function::Create(NewFTy, F->getLinkage(),
                                    F->getName() + ".gvalloc", M);
      auto *NewFGVArg = NewF->getArg(NewF->arg_size() - 1);
      ValueToValueMapTy VMap;
      SmallVector<ReturnInst *> Returns;
      for (auto [Arg, NewArg] : zip(F->args(), NewF->args())) {
        NewArg.setName(Arg.getName());
        VMap[&Arg] = &NewArg;
      }
      // VMap[&GV] = NewFGVArg;
      CloneFunctionInto(NewF, F, VMap,
                        CloneFunctionChangeType::LocalChangesOnly, Returns);
      auto ShouldReplaceGV = [NewF](Use &U) {
        if (auto *I = dyn_cast<Instruction>(U.getUser())) {
          auto *IF = I->getFunction();
          return IF == NewF;
        }
        // Note: this requires running the ConstExprEliminatePass beforehand.
        return false;
      };
      GV.replaceUsesWithIf(NewFGVArg, ShouldReplaceGV);
      ReplaceMap[F] = NewF;
    }

    auto ShouldReplaceGV = [MainF](Use &U) {
      if (auto *I = dyn_cast<Instruction>(U.getUser())) {
        auto *IF = I->getFunction();
        return IF == MainF;
      }
      // Note: this requires running the ConstExprEliminatePass beforehand.
      return false;
    };
    GV.replaceUsesWithIf(GVAI, ShouldReplaceGV);

    for (auto *F : GVUserFuncs) {
      if (F == MainF)
        continue;

      auto *NewF = ReplaceMap[F];

      for (auto *U : make_early_inc_range(F->users())) {
        assert(isa<CallInst>(U) &&
               "Functions can only be called according to the spec.");
        auto *CI = cast<CallInst>(U);
        auto *CIF = CI->getFunction();
        if (GVUserFuncs.contains(CIF) && CIF != MainF)
          continue;

        SmallVector<Value *> Args(CI->arg_begin(), CI->arg_end());
        if (CIF != MainF) {
          auto *GVArg = CIF->getArg(CIF->arg_size() - 1);
          Args.push_back(GVArg);
        } else {
          Args.push_back(GVAI);
        }
        auto *NewCI = CallInst::Create(NewF, Args);
        NewCI->insertAfter(CI);
        CI->replaceAllUsesWith(NewCI);
        CI->eraseFromParent();
      }

      // This is required to remove recursive functions.
      F->replaceAllUsesWith(UndefValue::get(F->getType()));
      F->eraseFromParent();
    }

    GV.eraseFromParent();
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

extern "C" ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "SWPPGV2Alloca", "v0.1",
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "gv2alloca") {
                    MPM.addPass(GV2AllocaPass());
                    return true;
                  }
                  return false;
                });
          }};
}
} // namespace sc::opt::gv2alloca
