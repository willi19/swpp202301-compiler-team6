#include "opt.h"

#include "../static_error.h"
#include "llvm/Analysis/CGSCCPassManager.h"

#include "opt/branchpredict.h"
#include "opt/load2aload.h"
#include "opt/ArithmeticPass.h"
#include "opt/loop2sum.h"

#include "print_ir.h"

#include "llvm/Transforms/Utils/Mem2Reg.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"

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
    llvm::FunctionPassManager FPM;
    llvm::CGSCCPassManager CGPM;
    llvm::ModulePassManager MPM;

    // Add loop-level opt passes below
    FPM.addPass(llvm::SimplifyCFGPass());
    FPM.addPass(createFunctionToLoopPassAdaptor(loop2sum::Loop2SumPass()));
 
    // Add function-level opt passes below
    FPM.addPass(llvm::PromotePass());
    FPM.addPass(arithmeticPass::ArithmeticPass());
    FPM.addPass(branchpredict::BranchPredictPass());
    FPM.addPass(load2aload::Load2AloadPass());

    CGPM.addPass(llvm::createCGSCCToFunctionPassAdaptor(std::move(FPM)));
    // Add CGSCC-level opt passes below

    MPM.addPass(llvm::createModuleToPostOrderCGSCCPassAdaptor(std::move(CGPM)));
    // Add module-level opt passes below

    MPM.run(*__M, __MAM);

    sc::print_ir::printIRIfVerbose(*__M, "After optimization");
  } catch (const std::exception &e) {
    return RetType::Err(OptInternalError(e));
  }

  return RetType::Ok(std::move(__M));
}
} // namespace sc::opt
