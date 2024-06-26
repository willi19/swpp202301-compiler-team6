cmake_minimum_required(VERSION 3.13.0)
project(swpp-compiler VERSION 2023.1.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

# Specify LLVM directory to use
# set(LLVM_ROOT ~/swpp202301/llvm-swpp)
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVM in: ${LLVM_BINARY_DIR}")

add_compile_options(-fPIC)

add_library(Fs ${SRC_DIR}/fs.cpp)

add_library(SCArgs ${SRC_DIR}/args.cpp)

add_library(SCParser OBJECT ${SRC_DIR}/lib/parser.cpp)
target_include_directories(SCParser PRIVATE ${LLVM_INCLUDE_DIRS})
llvm_map_components_to_libnames(parser_llvm_libs asmparser)
target_link_libraries(SCParser ${parser_llvm_libs})

add_library(SCPrintIR ${SRC_DIR}/lib/print_ir.cpp)
target_include_directories(SCPrintIR PRIVATE ${LLVM_INCLUDE_DIRS})
llvm_map_components_to_libnames(printer_llvm_libs core support)
target_link_libraries(SCPrintIR ${printer_llvm_libs})

set(SRC_BACKEND_DIR ${SRC_DIR}/lib/backend)
add_library(SCBackendAssembly OBJECT
                ${SRC_BACKEND_DIR}/assembly/inst.cpp
                ${SRC_BACKEND_DIR}/assembly/int_t.cpp
                ${SRC_BACKEND_DIR}/assembly/register_t.cpp
                ${SRC_BACKEND_DIR}/assembly/width_t.cpp)

add_library(SCBackendAnalysis OBJECT ${SRC_BACKEND_DIR}/analysis.cpp)
target_include_directories(SCBackendAnalysis PRIVATE ${LLVM_INCLUDE_DIRS})

add_library(SCBackendEmitter OBJECT ${SRC_BACKEND_DIR}/emitter.cpp)
target_include_directories(SCBackendEmitter PRIVATE ${LLVM_INCLUDE_DIRS})

add_library(SCBackendSymbol OBJECT ${SRC_BACKEND_DIR}/symbol.cpp
                ${SRC_BACKEND_DIR}/symbol/base.cpp)
target_include_directories(SCBackendSymbol PRIVATE ${LLVM_INCLUDE_DIRS})

function(add_backend_pass pass_name file_name)
    add_library(${pass_name} OBJECT ${SRC_BACKEND_DIR}/${file_name})
    target_include_directories(${pass_name} PRIVATE ${LLVM_INCLUDE_DIRS})
endfunction()

add_backend_pass(SCBackendCheckOraclePass check_oracle.cpp)
add_backend_pass(SCBackendConstExprElimPass const_expr_eliminate.cpp)
add_backend_pass(SCBackendGEPElimPass gep_eliminate.cpp)
add_backend_pass(SCBackendGVElimPass gv_eliminate.cpp)
add_backend_pass(SCBackendAllocaElimPass alloca_eliminate.cpp)
add_backend_pass(SCBackendGEPConstCombPass gep_const_combine.cpp)
add_backend_pass(SCBackendPHIPreprocessPass phi_preprocess.cpp)
add_backend_pass(SCBackendRegAllocPass register_allocate.cpp)

add_library(SCBackend ${SRC_DIR}/lib/backend.cpp)
target_include_directories(SCBackend PRIVATE ${LLVM_INCLUDE_DIRS})
llvm_map_components_to_libnames(backend_llvm_libs analysis scalaropts)
target_link_libraries(SCBackend PRIVATE
                        SCPrintIR SCBackendAssembly SCBackendAnalysis
                        SCBackendEmitter SCBackendSymbol SCBackendCheckOraclePass
                        SCBackendConstExprElimPass SCBackendGEPElimPass
                        SCBackendGVElimPass SCBackendAllocaElimPass
                        SCBackendGEPConstCombPass SCBackendPHIPreprocessPass
                        SCBackendRegAllocPass ${backend_llvm_libs})

set(SRC_OPT_DIR ${SRC_DIR}/lib/opt)
add_library(OptPasses INTERFACE)

# llvm_map_components_to_libnames(pass_llvm_libs passes analysis core support transformutils scalaropts)
llvm_map_components_to_libnames(pass_llvm_libs passes analysis core support transformutils vectorize scalaropts coverage instcombine)
function(add_opt_pass pass_name file_name)
    add_library(${pass_name} ${SRC_OPT_DIR}/${file_name})
    target_include_directories(${pass_name} PRIVATE ${LLVM_INCLUDE_DIRS})
    target_link_libraries(${pass_name} PRIVATE ${pass_llvm_libs})
    target_link_libraries(OptPasses INTERFACE ${pass_name})
    add_test(NAME ${pass_name}
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/utils/testpass.sh
            ${LLVM_TOOLS_BINARY_DIR} ${pass_name} $<TARGET_FILE:${pass_name}>
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endfunction()

# /================= ADD OPT PASSES BELOW ================/
add_opt_pass(arithmetic arithmeticpass.cpp)
add_opt_pass(branchpredict branchpredict.cpp)
add_opt_pass(load2aload load2aload.cpp)
add_opt_pass(heap2stack heap2stack.cpp)
add_opt_pass(Loop2SumPass loop2sum.cpp)
add_opt_pass(intrinsic_elim intrinsic_eliminate.cpp)
add_opt_pass(FunctionInlinePass functioninline.cpp)
add_opt_pass(IncrDecrPass incrdecr.cpp)
add_opt_pass(oracle oracle.cpp)
add_opt_pass(removefree removefree.cpp)
add_opt_pass(phierase phierase.cpp)
add_opt_pass(Add2SumPass add2sum.cpp)
add_opt_pass(reordermem reordermem.cpp)
add_opt_pass(gv2alloca gv2alloca.cpp)

# /================== DO NOT EDIT BELOW ==================/
add_library(SCOpt ${SRC_DIR}/lib/opt.cpp)
target_include_directories(SCOpt PRIVATE ${LLVM_INCLUDE_DIRS})
llvm_map_components_to_libnames(opt_llvm_libs analysis passes transformutils scalaropts instcombine ipo)
target_link_libraries(SCOpt PRIVATE SCPrintIR SCBackendConstExprElimPass OptPasses ${opt_llvm_libs})

set(PROJECT_LIB "SComp")
add_library(${PROJECT_LIB} ${SRC_DIR}/lib.cpp)
target_include_directories(${PROJECT_LIB} PRIVATE ${LLVM_INCLUDE_DIRS})
llvm_map_components_to_libnames(comp_llvm_libs passes analysis)
target_link_libraries(${PROJECT_LIB} PRIVATE
                         Fs SCBackend SCPrintIR SCOpt SCParser ${comp_llvm_libs})

add_executable(${PROJECT_NAME} ${SRC_DIR}/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_LIB} SCArgs)

# check if Doxygen is installed
find_package(Doxygen)
if (DOXYGEN_FOUND)
    # set input and output files
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile)

    # request to configure the file
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")

    add_custom_target(docs ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM )
else (DOXYGEN_FOUND)
  message("Doxygen should be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)

enable_testing()
# Add tests, subdirectories, etc below
# Tests for custom passes are added through add_opt_pass

add_test(NAME simplifycfg
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/utils/testpass.sh
        ${LLVM_TOOLS_BINARY_DIR} simplifycfg
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME mem2reg
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/utils/testpass.sh
        ${LLVM_TOOLS_BINARY_DIR} mem2reg
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME tailcallelim
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/utils/testpass.sh
        ${LLVM_TOOLS_BINARY_DIR} tailcallelim
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME gvn
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/utils/testpass.sh
        ${LLVM_TOOLS_BINARY_DIR} gvn
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
