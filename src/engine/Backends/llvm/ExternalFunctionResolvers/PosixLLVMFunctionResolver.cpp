#include <omniscript/engine/Backends/LLVM/IRGenerator.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolver.h>

// PosixResolver Implementation
llvm::Function* PosixResolver::resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) {
    if (!PlatformInfo::isUnixLike()) {
        return nullptr;
    }
    
    if (isPthreadFunction(name) || isSocketFunction(name) || isMathFunction(name)) {
        auto func = llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            name,
            generator.getCurrentModule()
        );
        
        func->setCallingConv(llvm::CallingConv::C);
        return func;
    }
    
    return nullptr;
}

bool PosixResolver::isPthreadFunction(const std::string& name) {
    static const std::unordered_set<std::string> pthreadFunctions = {
        "pthread_create", "pthread_join", "pthread_detach", "pthread_exit",
        "pthread_self", "pthread_equal", "pthread_cancel", "pthread_kill",
        "pthread_mutex_init", "pthread_mutex_destroy", "pthread_mutex_lock",
        "pthread_mutex_unlock", "pthread_mutex_trylock",
        "pthread_cond_init", "pthread_cond_destroy", "pthread_cond_wait",
        "pthread_cond_signal", "pthread_cond_broadcast", "pthread_cond_timedwait"
    };
    
    return pthreadFunctions.find(name) != pthreadFunctions.end();
}

bool PosixResolver::isSocketFunction(const std::string& name) {
    static const std::unordered_set<std::string> socketFunctions = {
        "socket", "bind", "listen", "accept", "connect", "send", "recv",
        "sendto", "recvfrom", "close", "shutdown", "setsockopt", "getsockopt",
        "select", "poll", "epoll_create", "epoll_ctl", "epoll_wait"
    };
    
    return socketFunctions.find(name) != socketFunctions.end();
}

bool PosixResolver::isMathFunction(const std::string& name) {
    static const std::unordered_set<std::string> mathFunctions = {
        "sinf", "cosf", "tanf", "asinf", "acosf", "atanf", "atan2f",
        "sinhf", "coshf", "tanhf", "expf", "logf", "log10f", "powf",
        "sqrtf", "floorf", "ceilf", "fabsf", "fmodf"
    };
    
    return mathFunctions.find(name) != mathFunctions.end();
}

std::vector<std::string> PosixResolver::getRequiredLibraries(const std::string& name) {
    std::vector<std::string> libs;
    
    if (isPthreadFunction(name)) {
        libs.push_back("pthread");
    }
    if (isMathFunction(name)) {
        libs.push_back("m");
    }
    if (isSocketFunction(name)) {
        libs.push_back("c"); // Usually part of libc
    }
    
    return libs;
}