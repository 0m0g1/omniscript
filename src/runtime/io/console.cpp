#include <omniscript/runtime/io/console.h>
#include <omniscript/utils.h>
#include <omniscript/runtime/String.h>
#include <omniscript/runtime/Number.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
#include <sys/resource.h>
#endif


/*
* The console object
*/

ConsoleObject::ConsoleObject() {
    name = "console";
    addMethod("log", [this](const ArgumentDefinition& args) {
        log(args, LogLevel::LOG);
        return SymbolTable::ValueType{};
    });
    addMethod("info", [this](const ArgumentDefinition& args) {
        info(args);
        return SymbolTable::ValueType{};
    });
    addMethod("warn", [this](const ArgumentDefinition& args) {
        warn(args);
        return SymbolTable::ValueType{};
    });
    addMethod("error", [this](const ArgumentDefinition& args) {
        error(args);
        return SymbolTable::ValueType{};
    });
    addMethod("debug", [this](const ArgumentDefinition& args) {
        debug(args);
        return SymbolTable::ValueType{};
    });
    addMethod("assert", [this](const ArgumentDefinition& args) {
        assertCondition(args);
        return SymbolTable::ValueType{};
    });
    addMethod("time", [this](const ArgumentDefinition& args) {
        startTime(args);
        return SymbolTable::ValueType{};
    });
    addMethod("timeEnd", [this](const ArgumentDefinition& args) {
        endTime(args);
        return SymbolTable::ValueType{};
    });
    addMethod("table", [this](const ArgumentDefinition& args) {
        displayTable(args);
        return SymbolTable::ValueType{};
    });
    addMethod("clear", [this](const ArgumentDefinition& args) {
        std::cout << "\033[2J\033[1;1H";
        return SymbolTable::ValueType{};
    });
    addMethod("input", [this](const ArgumentDefinition& args) {
        return input(args);
    });
    addMethod("pause", [this](const ArgumentDefinition& args) {
        return SymbolTable::ValueType{};
    });
    addMethod("clearLine", [this](const ArgumentDefinition& args) {
        if (args.size() > 0) {   
             std::invalid_argument("clear line takes no arguments");
        }
        std::cout << "\033[K";  // ANSI escape code to clear the current line
        return SymbolTable::ValueType{};
    });
}

void ConsoleObject::log(const ArgumentDefinition& args, LogLevel level) {
    if (level == DEBUG_LOG && !debugEnabled) return;
    if (args.empty()) return; // No arguments to log
    // Iterate through all arguments
    bool printNewline = true;
    int argsSize = args.size();

    if (args.size() > 0 && std::holds_alternative<bool>(args.back())) {
        printNewline = std::get<bool>(args.back());
        argsSize--;
    }

    std::ostringstream messageStream;
    for (size_t i = 0; i < argsSize; ++i) {
        auto& arg = args[i];

        std::string messagePart;

        // Convert argument to string
        if (std::holds_alternative<std::string>(arg)) {
            messagePart = std::get<std::string>(arg);
        } else if (auto objPtr = std::get_if<std::shared_ptr<Object>>(&arg)) {
            if (*objPtr) {
                messagePart = (*objPtr)->toString();
            } else {
                messagePart = "null"; // Handle null shared_ptr
            }
        } 
        // else if (isTruthy(arg)) {
        //     auto object = primitiveToObject(arg);
        //     if (auto number = std::dynamic_pointer_cast<Number>(object)) {
        //         messagePart = number->toString();
        //     } else if (auto string = std::dynamic_pointer_cast<String>(object)) {
        //         messagePart = string->getValue();
        //     } else if (auto array = std::dynamic_pointer_cast<Array>(object)) {
        //         messagePart = array->toString();
        //     } else {
        //         messagePart = object->toString();
        //     }
        // }
        else {
            messagePart = "null"; // Default for falsy or unrecognized values
        }

        messageStream << messagePart;
        if (i < args.size() - 1) {
            messageStream << " "; // Separate arguments with a space
        }
    }

    // Extract final message
    std::string finalMessage = messageStream.str();

    // Determine log color and print
    switch (level) {
        case LOG:
            std::cout << "\033[0;37m" << finalMessage << "\033[0m";
            break;
        case INFO:
            std::cout << "\033[1;32m" << finalMessage << "\033[0m";
            break;
        case WARN:
            std::cout << "\033[1;33m" << finalMessage << "\033[0m";
            break;
        case ERR:
            std::cerr << "\033[1;31mError: " << finalMessage << "\033[0m";
            break;
        case DEBUG_LOG:
            std::cout << "\033[0;34m" << finalMessage << "\033[0m";
            break;
    }

    // Determine newline behavior
    

    if (printNewline) {
        std::cout << std::endl;
    }
}


void ConsoleObject::info(const ArgumentDefinition& args) {
    log(args, LogLevel::INFO);
}

void ConsoleObject::warn(const ArgumentDefinition& args) {
    log(args, LogLevel::WARN);
}

void ConsoleObject::error(const ArgumentDefinition& args) {
    log(args, LogLevel::ERR);
}

void ConsoleObject::debug(const ArgumentDefinition& args) {
    log(args, LogLevel::DEBUG_LOG);
}

void ConsoleObject::assertCondition(const ArgumentDefinition& args) {
    if (args.size() < 2 || !std::holds_alternative<bool>(args[0]) || !std::holds_alternative<std::string>(args[1])) {
        throw std::invalid_argument("assert requires a condition and a message as arguments");
    }

    bool condition = std::get<bool>(args[0]);
    const std::string& message = std::get<std::string>(args[1]);

    if (!condition) {
        log({message}, LogLevel::ERR);
    }
}

void ConsoleObject::startTime(const ArgumentDefinition& args) {
    if (args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
        throw std::invalid_argument("time requires exactly one string argument as a label");
    }

    const std::string& label = std::get<std::string>(args[0]);
    timers[label] = std::chrono::high_resolution_clock::now();
}

void ConsoleObject::endTime(const ArgumentDefinition& args) {
    if (args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
        throw std::invalid_argument("timeEnd requires exactly one string argument as a label");
    }

    const std::string& label = std::get<std::string>(args[0]);
    auto end = std::chrono::high_resolution_clock::now();

    if (timers.find(label) != timers.end()) {
        auto start = timers[label];
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        log({label + ": " + std::to_string(duration) + "ms"}, INFO);
        timers.erase(label);
    } else {
        log({"Timer \"" + label + "\" does not exist."}, WARN);
    }
}

void ConsoleObject::displayTable(const ArgumentDefinition& args) {
    // if (args.size() < 2 || !std::holds_alternative<std::vector<std::string>>(args[1])) {
    //     throw std::invalid_argument("Table requires at least two arguments: data and headers.");
    // }

    // if (headers.size() != 2) {
    //     error("Error: Header size mismatch!");
    //     return;
    // }

    // const int colWidth = 15; // Set a fixed column width for neat alignment

    // // Print headers with fixed width
    // std::cout << std::left;
    // for (const auto& header : headers) {
    //     std::cout << std::setw(colWidth) << header;
    // }
    // std::cout << "\n";

    // // Print a divider
    // std::cout << std::string(colWidth * headers.size(), '-') << "\n";

    // // Print each item
    // for (const auto& item : items) {
    //     std::cout << std::setw(colWidth) << item << "\n";
    // }
}

SymbolTable::ValueType ConsoleObject::input(const ArgumentDefinition& args) {
    if (args.size() < 1 || !std::holds_alternative<std::string>(args[0])) {
        throw std::invalid_argument("input requires a prompt message as a string argument");
    }

    const std::string& prompt = std::get<std::string>(args[0]);
    std::cout << prompt;
    std::string userInput;
    std::getline(std::cin, userInput); // Read the user's input from the console
    return SymbolTable::ValueType(userInput); // Return the input as a string
}

// Pause the console until user input
void ConsoleObject::pause(const ArgumentDefinition& args) {
    std::string message = "Press any key to continue...";
    if (args.size() > 0 && std::holds_alternative<std::string>(args[0])) {
        message = std::get<std::string>(args[0]);
    }
    std::cout << message;
    std::cin.get();  // Wait for user input before proceeding
}

// Log memory usage (cross-platform)
void ConsoleObject::logMemoryUsage(const ArgumentDefinition& args) {
    if (!args.empty()) {
        std::cerr << "logMemoryUsage does not use arguments.\n";
    }
    #if defined(_WIN32)
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(MEMORYSTATUSEX);
        if (GlobalMemoryStatusEx(&status)) {
            std::cout << "Memory usage: " << status.ullTotalPhys / 1024 / 1024 << " MB\n";
        } else {
            std::cout << "Failed to get memory usage on Windows.";
        }
    #elif defined(__linux__) || defined(__APPLE__)
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        std::cout << "Memory usage: " << usage.ru_maxrss / 1024 << " MB\n";  // Linux/macOS
    #endif
}