#include "opt.h"

#include "../static_error.h"
#include "opt/arithmeticpass.h"

#include "opt/branchpredict.h"
#include "opt/heap2stack.h"
#include "opt/incrdecr.h"
#include "opt/load2aload.h"
#include "opt/loop2sum.h"
#include "opt/functioninline.h"
#include "opt/oracle.h"

#include "print_ir.h"

#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Scalar/TailRecursionElimination.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"

using namespace std::string_literals;

namespace sc::opt {
OptInternalError::OptInternalError(const std::exception &__e) noexcept {
  message = "exception thrown from opt\n"s + __e.what();
}

Result<std::unique_ptr<llvm::Module>, OptInternalError>
optimizeIR(std::unique_ptr<llvm::Module> &&__M,
           llvm::ModuleAnalysisManager &__MAM) noexcept {
  using RetType = Result<std::unique_ptr<llvm::Module>, OptInternalError>;

  try {
    llvm::LoopPassManager LPM;
    llvm::FunctionPassManager FPM;
    llvm::CGSCCPassManager CGPM;
    llvm::ModulePassManager MPM;

    // Add loop-level opt passes below
    FPM.addPass(llvm::SimplifyCFGPass());
    FPM.addPass(createFunctionToLoopPassAdaptor(loop2sum::Loop2SumPass()));
    FPM.addPass(llvm::SimplifyCFGPass());
    FPM.addPass(llvm::createFunctionToLoopPassAdaptor(std::move(LPM)));
    // Add function-level opt passes below
    FPM.addPass(llvm::PromotePass());
    FPM.addPass(llvm::TailCallElimPass());
    FPM.addPass(llvm::GVNPass());
    FPM.addPass(branchpredict::BranchPredictPass());
    FPM.addPass(load2aload::Load2AloadPass());
    FPM.addPass(arithmeticpass::ArithmeticPass());

    CGPM.addPass(llvm::createCGSCCToFunctionPassAdaptor(std::move(FPM)));
    // Add CGSCC-level opt passes below

    MPM.addPass(llvm::createModuleToPostOrderCGSCCPassAdaptor(std::move(CGPM)));
    // Add module-level opt passes below
    MPM.addPass(functioninline::FunctionInlinePass());
    MPM.addPass(heap2stack::Heap2StackPass());
    MPM.addPass(llvm::VerifierPass());
    MPM.addPass(oracle::OraclePass());
    FPM.addPass(incrdecr::IncrDecrPass());

    MPM.run(*__M, __MAM);

    sc::print_ir::printIRIfVerbose(*__M, "After optimization");
  } catch (const std::exception &e) {
    return RetType::Err(OptInternalError(e));
  }

  return RetType::Ok(std::move(__M));
}
} // namespace sc::opt
