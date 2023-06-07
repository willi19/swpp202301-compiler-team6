#include "opt.h"

#include "../static_error.h"
#include "backend/const_expr_eliminate.h"
#include "opt/arithmeticpass.h"
#include "opt/add2sum.h"
#include "opt/branchpredict.h"
#include "opt/functioninline.h"
#include "opt/gv2alloca.h"
#include "opt/heap2stack.h"
#include "opt/incrdecr.h"
#include "opt/intrinsic_eliminate.h"
#include "opt/load2aload.h"
#include "opt/oracle.h"
#include "opt/phierase.h"
#include "opt/removefree.h"
#include "opt/reordermem.h"
#include "print_ir.h"

#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/IPO/ArgumentPromotion.h"
#include "llvm/Transforms/IPO/GlobalDCE.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/SROA.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Scalar/TailRecursionElimination.h"

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
    llvm::ModulePassManager MPM;

    // For function simplification
    llvm::FunctionPassManager FPM1;

    FPM1.addPass(llvm::SimplifyCFGPass());
    FPM1.addPass(llvm::InstCombinePass());

    // For unknown reasons, running GVN both before and after TCE is beneficial
    FPM1.addPass(llvm::GVNPass());
    FPM1.addPass(llvm::SimplifyCFGPass());
    FPM1.addPass(llvm::InstCombinePass());
    FPM1.addPass(llvm::TailCallElimPass());
    // TCE produces more GVN opportunities
    FPM1.addPass(llvm::GVNPass());
    FPM1.addPass(llvm::SimplifyCFGPass());
    FPM1.addPass(llvm::InstCombinePass());

    FPM1.addPass(intrinsic_elim::IntrinsicEliminatePass());
    FPM1.addPass(arithmeticpass::ArithmeticPass());
    FPM1.addPass(branchpredict::BranchPredictPass());

    MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM1)));

    // Required for GV2Alloca to not be stuck by constant expressions
    MPM.addPass(backend::ce_elim::ConstExprEliminatePass());
    MPM.addPass(gv2alloca::GV2AllocaPass());
    MPM.addPass(functioninline::FunctionInlinePass());
    MPM.addPass(llvm::GlobalDCEPass());
    MPM.addPass(llvm::createModuleToFunctionPassAdaptor(SROAPass()));
    MPM.addPass(llvm::createModuleToPostOrderCGSCCPassAdaptor(llvm::ArgumentPromotionPass()));

    // For most-optimizations
    llvm::FunctionPassManager FPM2;

    // Inlining produces more GVN opportunities
    // For unknown reasons, SimplifyCFG hits hard in this stage
    // FPM2.addPass(llvm::SimplifyCFGPass());
    FPM2.addPass(llvm::InstCombinePass());
    FPM2.addPass(llvm::GVNPass());
    // FPM2.addPass(llvm::SimplifyCFGPass());
    FPM2.addPass(llvm::InstCombinePass());

    // Required for oracle pass to work well
    FPM2.addPass(intrinsic_elim::IntrinsicEliminatePass());
    FPM2.addPass(arithmeticpass::ArithmeticPass());
    FPM2.addPass(branchpredict::BranchPredictPass());

    MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM2)));

    MPM.addPass(heap2stack::Heap2StackPass());
    MPM.addPass(oracle::OraclePass());

    // For post-oracle optimizations
    llvm::FunctionPassManager FPM3;
    FPM3.addPass(reordermem::ReorderMemPass());
    FPM3.addPass(llvm::GVNPass());
    
    // Produces intrinsic functions, must be after oracle
    FPM3.addPass(load2aload::Load2AloadPass());
    FPM3.addPass(incrdecr::IncrDecrPass());
    // Just to be safe
    FPM3.addPass(intrinsic_elim::IntrinsicEliminatePass());
    FPM3.addPass(phierase::PHIErasePass());
    MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM3)));

    MPM.addPass(removefree::RemoveFreePass());

    MPM.addPass(llvm::VerifierPass());

    MPM.run(*__M, __MAM);

    sc::print_ir::printIRIfVerbose(*__M, "After optimization");
  } catch (const std::exception &e) {
    return RetType::Err(OptInternalError(e));
  }

  return RetType::Ok(std::move(__M));
}
} // namespace sc::opt
