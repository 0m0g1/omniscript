#include <omniscript/engine/Backends/LLVM/IRGenerator.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolver.h>

// WindowsAPIResolver Implementation
llvm::Function* WindowsAPIResolver::resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) {
    if (PlatformInfo::getCurrentPlatform() != PlatformInfo::Windows) {
        return nullptr;
    }
    
    if (isKernel32Function(name) || isUser32Function(name) || isGdi32Function(name)) {
        auto func = llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            name,
            generator.getCurrentModule()
        );
        
        // Windows API functions typically use stdcall on x86, but fastcall/Win64 on x64
        if (PlatformInfo::getCurrentArchitecture() == PlatformInfo::x86) {
            func->setCallingConv(llvm::CallingConv::X86_StdCall);
        } else {
            func->setCallingConv(llvm::CallingConv::Win64);
        }
        
        return func;
    }
    
    return nullptr;
}

bool WindowsAPIResolver::isKernel32Function(const std::string& name) {
    static const std::unordered_set<std::string> kernel32Functions = {
        "GetCurrentProcess", "GetCurrentThread", "GetCurrentProcessId", "GetCurrentThreadId",
        "Sleep", "SleepEx", "GetTickCount", "GetTickCount64",
        "CreateFileA", "CreateFileW", "ReadFile", "WriteFile", "CloseHandle",
        "GetLastError", "SetLastError", "FormatMessageA", "FormatMessageW",
        "VirtualAlloc", "VirtualFree", "VirtualProtect", "VirtualQuery",
        "CreateThread", "ExitThread", "TerminateThread", "WaitForSingleObject",
        "CreateMutexA", "CreateMutexW", "ReleaseMutex", "CreateEventA", "CreateEventW"
    };
    
    return kernel32Functions.find(name) != kernel32Functions.end();
}

bool WindowsAPIResolver::isUser32Function(const std::string& name) {
    static const std::unordered_set<std::string> user32Functions = {
        "MessageBoxA", "MessageBoxW", "GetWindowTextA", "GetWindowTextW",
        "FindWindowA", "FindWindowW", "ShowWindow", "UpdateWindow",
        "GetDC", "ReleaseDC", "InvalidateRect", "GetClientRect", "GetWindowRect",
        "SetWindowPos", "MoveWindow", "DestroyWindow"
    };
    
    return user32Functions.find(name) != user32Functions.end();
}

bool WindowsAPIResolver::isGdi32Function(const std::string& name) {
    static const std::unordered_set<std::string> gdi32Functions = {
        "CreateCompatibleDC", "DeleteDC", "SelectObject", "DeleteObject",
        "BitBlt", "StretchBlt", "SetPixel", "GetPixel", "CreateSolidBrush",
        "CreatePen", "LineTo", "MoveTo", "Rectangle", "Ellipse"
    };
    
    return gdi32Functions.find(name) != gdi32Functions.end();
}

std::string WindowsAPIResolver::getRequiredDLL(const std::string& name) {
    if (isKernel32Function(name)) return "kernel32.dll";
    if (isUser32Function(name)) return "user32.dll";
    if (isGdi32Function(name)) return "gdi32.dll";
    return "";
}