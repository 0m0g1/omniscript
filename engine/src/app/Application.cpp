#include <omniscript/Application.h>
#include <iostream>

#if defined(OMNI_HAS_LLVM)
  #include <llvm/IR/LLVMContext.h>
  #include <llvm/IR/Module.h>
  #include <llvm/IR/IRBuilder.h>
  #include <llvm/Support/raw_ostream.h>
#endif

namespace Omniscript {


Application::Application(int argc, char** argv)
  : m_argc(argc), m_argv(argv), m_engine(argc, argv) {}
  
int Application::run() {
    return m_engine.run();
}

}
