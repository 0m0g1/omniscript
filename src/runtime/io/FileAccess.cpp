// #include <omniscript/omniscript_pch.h>
// #include <omniscript/runtime/io/FileAccess.h>
// #include <omniscript/utils.h>
// #include <omniscript/debuggingtools/console.h>
// #include <filesystem>


// FileAccess::FileAccess() {
//     setProperty("READ", static_cast<int>(Mode::READ));
//     setProperty("WRITE", static_cast<int>(Mode::WRITE));
//     setProperty("READWRITE", static_cast<int>(Mode::READWRITE));
//     setProperty("WRITEREAD", static_cast<int>(Mode::WRITEREAD));

//     addMethod("open", [this](const ArgumentDefinition& args) {
//         if (!std::holds_alternative<std::string>(args[0])) {
//             console.error("The first argument in FileAccess.open() should be a string got '" + valueToString(args[0]) + "' instead.");
//         }

//         if (!std::holds_alternative<int>(args[1])) {
//             console.error("The second argument in FileAccess.open() should be a FileAccess mode '" + valueToString(args[1]) + "' instead.");
//         }

//         std::string path = std::get<std::string>(args[0]);
//         Mode mode = static_cast<Mode>(std::get<int>(args[1]));

//         return open(path, mode);
//     });
// }

// std::shared_ptr<File> FileAccess::open(std::string& path, Mode mode) {
//     if (path.empty()) {
//         console.error("Path cannot be empty.");
//         return nullptr;
//     }
//     console.debug("The file path is " + path);
    
//     std::ios::openmode openMode;

//     switch (mode) {
//         case Mode::READ:
//             openMode = std::ios::in;
//             break;
//         case Mode::WRITE:
//             openMode = std::ios::out;
//             break;
//         case Mode::READWRITE:
//             openMode = std::ios::in | std::ios::out;
//             break;
//         case Mode::WRITEREAD:
//             openMode = std::ios::in | std::ios::out | std::ios::trunc;
//             break;
//         default:
//             console.error("Invalid mode provided for file access.");
//             return nullptr;
//     }

//     // Return a File object wrapped in a shared_ptr
//     return std::make_shared<File>(path, openMode);
// }
