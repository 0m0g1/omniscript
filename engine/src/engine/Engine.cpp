#include <omniscript/Engine.h>
#include <iostream>

#if defined(OMNI_HAS_LLVM)
  #include <llvm/IR/LLVMContext.h>
  #include <llvm/IR/Module.h>
  #include <llvm/IR/IRBuilder.h>
  #include <llvm/Support/raw_ostream.h>
#endif

namespace Omniscript {


Engine::Engine(int argc, char** argv)
  : m_argc(argc), m_argv(argv) {}
  
int Engine::run() {
    std::cout << "hello world\n";

#if defined(OMNI_HAS_LLVM)
    llvm::LLVMContext ctx;
    auto m = std::make_unique<llvm::Module>("test", ctx);
    llvm::IRBuilder<> b(ctx);

    auto* fnTy = llvm::FunctionType::get(b.getInt32Ty(), false);
    auto* fn = llvm::Function::Create(fnTy, llvm::Function::ExternalLinkage, "main", m.get());
    auto* entry = llvm::BasicBlock::Create(ctx, "entry", fn);
    b.SetInsertPoint(entry);
    b.CreateRet(b.getInt32(0));

    llvm::outs() << *m << "\n";
#else
    std::cout << "(LLVM backend not enabled in this build)\n";
#endif

    return 0;
}

}
