#include <omniscript/omniscript_pch.h>
#include <omniscript/runtime/io/File.h>
#include <omniscript/debuggingtools/console.h>
#include <filesystem>

std::string File::toString(int indentLevel) const {
    return "[" + fileName + "]";
}

File::~File() {
    close();  // Ensure the file is closed when the object is destroyed
}

File::File(const std::string& path, std::ios::openmode mode)
    : filePath(path) {
    
    if (mode == std::ios::in) {
        if (!std::filesystem::exists(filePath)) {
            console.error("File does not exist: " + filePath);
            return;  // or throw an exception if desired
        }

        // Optionally, check if the file is accessible
        std::ifstream testStream(filePath);
        if (!testStream) {
            console.error("Cannot read file: " + filePath);
            return;  // or throw an exception
        }
        testStream.close();
    }

    
    file.open(filePath, mode);  // Open file with the specified mode
    
    if (!file.is_open()) {
        console.error("Failed to open file: " + filePath);
    }

    fileName = std::filesystem::path(filePath).filename().string();

    // Add methods
    addMethod("getAsString", [this](const ArgumentDefinition& args) {
        if (!args.empty()) {
            console.error("File.getAsString() doesn't take arguments.");
        }
        return getAsString();
    });

    addMethod("getAsBytes", [this](const ArgumentDefinition& args) {
        if (args.size() > 0) {
            console.error("File.getAsBytes() doesn't take in any arguments");
        }
        return getBinaryContent();
    });

    addMethod("getName", [this](const ArgumentDefinition& args) { return this->fileName; });
    addMethod("getPath", [this](const ArgumentDefinition& args) { return this->filePath; });

    // addMethod("moveTo", [this](const ArgumentDefinition& args) {
    //     if (args.size() != 1 || !std::holds_alternative<int>(args[0])) {
    //         console.error("File.moveTo() expects a single integer argument.");
    //         return SymbolTable::ValueType{};
    //     }
    //     auto position = std::get<int>(args[0]);
    //     std::visit([position](auto& stream) {
    //         if constexpr (std::is_same_v<std::fstream, decltype(stream)> ||
    //                     std::is_same_v<std::ifstream, decltype(stream)>) {
    //             stream.seekg(position);  // Move the get pointer for input
    //         }
    //         if constexpr (std::is_same_v<std::fstream, decltype(stream)> ||
    //                     std::is_same_v<std::ofstream, decltype(stream)>) {
    //             stream.seekp(position);  // Move the put pointer for output
    //         }
    //     }, file);
    //     return SymbolTable::ValueType{};
    // });


    addMethod("close", [this](const ArgumentDefinition& args) {
        close();
        return SymbolTable::ValueType{};
    });

    addMethod("flush", [this](const ArgumentDefinition& args) {
        if (args.size() > 0) {
            console.error("File.flush() doesn't take in any arguments");
        }
        flush();  // Call the flush function
        return SymbolTable::ValueType{};  // Return an empty value if needed
    });

    addMethod("setAutoFlush", [this](const ArgumentDefinition& args) {
        if (args.size() != 1) {
            console.error("File.setAutoFlush() expects 1 argument (boolean)");
        }
        bool autoFlush = std::get<bool>(args[0]);  // Convert argument to boolean
        setAutoFlush(autoFlush);
        return SymbolTable::ValueType{};  // Return an empty value if needed
    });


    addMethod("write", [this](const ArgumentDefinition& args) {
        if (args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
            console.error("File.write() expects exactly one string argument.");
            return SymbolTable::ValueType{};
        }
        write(std::get<std::string>(args[0]));
        return SymbolTable::ValueType{};
    });

    addMethod("writeln", [this](const ArgumentDefinition& args) {
        if (args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
            console.error("File.writeln() expects exactly one string argument.");
            return SymbolTable::ValueType{};
        }
        writeln(std::get<std::string>(args[0]));
        return SymbolTable::ValueType{};
    });

    // Register the append method
    addMethod("append", [this](const ArgumentDefinition& args) {
        if (args.size() != 1) {
            console.error("File.append() expects 1 argument (string)");
        }
        std::string data = std::get<std::string>(args[0]);  // Convert argument to string
        append(data);  // Call the append method
        return SymbolTable::ValueType{};  // Return an empty value if needed
    });

    // Register the prepend method
    addMethod("prepend", [this](const ArgumentDefinition& args) {
        if (args.size() != 1) {
            console.error("File.prepend() expects 1 argument (string)");
        }
        std::string data = std::get<std::string>(args[0]);  // Convert argument to string
        prepend(data);  // Call the prepend method
        return SymbolTable::ValueType{};  // Return an empty value if needed
    });

    addMethod("getSize", [this](const ArgumentDefinition& args) {
        if (args.size() > 0) {
            console.error("File.getSize() doesn't take in any arguments");
        }
        return static_cast<int>(std::filesystem::file_size(filePath));  // Return the file size
    });

    addMethod("clear", [this](const ArgumentDefinition& args) {
        if (args.size() > 0) {
            console.error("File.clear() doesn't take in any arguments");
        }
        // Open the file in write mode to overwrite the content
        file.close();  // Close the file first
        std::ofstream outFile(filePath, std::ios::trunc);  // Open file with truncation mode to clear it
        outFile.close();  // Close after clearing
        return SymbolTable::ValueType{};  // Return an empty value if needed
    });

    addMethod("exists", [this](const ArgumentDefinition& args) {
        if (args.size() > 0) {
            console.error("File.exists() doesn't take in any arguments");
        }
        return std::filesystem::exists(filePath);  // Return true if the file exists
    });

    addMethod("rename", [this](const ArgumentDefinition& args) {
        if (args.size() != 1) {
            console.error("File.rename() expects 1 argument (new file path)");
        }
        std::string newPath = std::get<std::string>(args[0]);
        std::filesystem::rename(filePath, newPath);  // Rename the file
        filePath = newPath;  // Update the internal file path
        return SymbolTable::ValueType{};  // Return an empty value if needed
    });

    // addMethod("getLastModified", [this](const ArgumentDefinition& args) {
    //     if (args.size() > 0) {
    //         console.error("File.getLastModified() doesn't take in any arguments");
    //         return SymbolTable::ValueType{};
    //     }

    //     // Get the last modified time from filesystem
    //     auto ftime = std::filesystem::last_write_time(filePath);  // Get last modified time

    //     // Convert the filesystem clock's time_point to system_clock's time_point
    //     auto duration = ftime.time_since_epoch(); // Get the duration since epoch
    //     auto sctp = std::chrono::system_clock::time_point(duration); // Convert duration to system_clock time_point

    //     // Convert to time_t for formatting
    //     std::time_t lastModified = std::chrono::system_clock::to_time_t(sctp);

    //     return std::string(std::ctime(&lastModified).string());  // Return the time as string
    // });

    addMethod("createBackup", [this](const ArgumentDefinition& args) {
        if (args.size() != 1) {
            console.error("File.createBackup() expects 1 argument (backup file path)");
        }
        std::string backupPath = std::get<std::string>(args[0]);
        std::filesystem::copy(filePath, backupPath);  // Create a copy of the file
        return SymbolTable::ValueType{};  // Return an empty value if needed
    });

    addMethod("isOpen", [this](const ArgumentDefinition& args) {
        if (args.size() > 0) {
            console.error("File.isOpen() doesn't take in any arguments");
        }
        return file.is_open();  // Return true if the file is open
    });

    addMethod("remove", [this](const ArgumentDefinition& args) {
        if (args.size() != 0) {
            console.error("File.remove() doesn't take in any arguments");
        }
        remove();  // Call the remove method to delete the file
        return SymbolTable::ValueType{};  // Return an empty value if needed
    });

    addMethod("truncate", [this](const ArgumentDefinition& args) {
        if (args.size() != 0) {
            console.error("File.truncate() doesn't take in any arguments");
        }
        truncate();  // Call the truncate method to truncate the file's content
        return SymbolTable::ValueType{};  // Return an empty value if needed
    });

    addMethod("getExtension", [this](const ArgumentDefinition& args) {
        if (args.size() > 0) {
            console.error("File.getExtension() doesn't take in any arguments");
        }
        return getExtension();  // Get the file's extension
    });

    addMethod("setPermissions", [this](const ArgumentDefinition& args) {
        if (args.size() != 1) {
            console.error("File.setPermissions() expects 1 argument (permissions mode)");
        }
        std::string permissions = std::get<std::string>(args[0]);
        setPermissions(permissions);  // Set the file's permissions
        return SymbolTable::ValueType{};  // Return an empty value if needed
    });

    addMethod("seekg", [this](const ArgumentDefinition& args) {
        seekg(args);
        return SymbolTable::ValueType{};
    });

    addMethod("seekp", [this](const ArgumentDefinition& args) {
        seekp(args);
        return SymbolTable::ValueType{};
    });

    addMethod("tellg", [this](const ArgumentDefinition& args) {
        return tellg(args);
    });

    addMethod("tellp", [this](const ArgumentDefinition& args) {
        return tellp(args);
    });

    addMethod("eof", [this](const ArgumentDefinition& args) {
        return eof(args);
    });

    addMethod("fail", [this](const ArgumentDefinition& args) {
        return fail(args);
    });

    addMethod("good", [this](const ArgumentDefinition& args) {
        return good(args);
    });

    addMethod("clearFlags", [this](const ArgumentDefinition& args) {
        clearFlags(args);
        return SymbolTable::ValueType{};
    });
}

std::string File::getAsString() {
    std::string content;
    if (file.is_open() && file.good()) {
        std::string line;
        while (std::getline(file, line)) {
            content += line + "\n";  // Append each line with a newline
        }
    } else {
        std::cerr << "Stream is not readable!" << std::endl;
    }
    return content;
}

std::vector<char> File::getBinaryContent() {
    
    // Open the file in binary mode if it hasn't been opened already
    std::ifstream fileInput(filePath, std::ios::binary);
    if (!fileInput.is_open()) {
        console.error("Failed to open file in binary mode");
        return {};
    }

    // Read file content into a vector
    std::vector<char> buffer((std::istreambuf_iterator<char>(fileInput)),
                                std::istreambuf_iterator<char>());
    return buffer;
}

// Write a string to the file
void File::write(const std::string& data) {
    if (lock()) {
        if (file.is_open() && file.good()) {
            file << data;  // Write string data to the file
        } else {
            console.error("Stream is not writable!");
        }
        unlock();
    }
}

void File::write(const std::vector<char>& data) {
    if (lock()) {
        if (file.is_open() && file.good()) {
            file.write(data.data(), data.size());  // Write binary data to the file
        } else {
            console.error("Stream is not writable!");
        }
        unlock();
    }
}

// Write data followed by a newline to the file
void File::writeln(const std::string& data) {
    write(data + "\n");  // Write the string data with newline 
}

void File::writeln(const std::vector<char>& data) {
    write(data);  // Write the binary data
    write(std::vector<char>{'\n'});  // Write a newline after the binary data
}

void File::flush() {
    if (file.is_open() && file.good()) {
        file.flush();  // Flush the output buffer to the file
    }
}

void File::append(const std::string& data) {
    if (file.is_open() && file.good()) {
        file.seekp(0, std::ios::end);  // Move to the end of the file
        file << data;  // Append data at the end
        if (autoFlush) {
            flush();  // Automatically flush if autoFlush is true
        }
    } else {
        console.error("Stream is not writable!");
    }
}

void File::prepend(const std::string& data) {
    if (file.is_open() && file.good()) {
        file.close();  // Close the file before prepending

        // Read the current contents of the file
        std::ifstream inFile(filePath, std::ios::in);
        std::stringstream buffer;
        buffer << inFile.rdbuf();
        std::string currentContent = buffer.str();

        // Open the file in write mode and overwrite with prepended content
        std::ofstream outFile(filePath, std::ios::out);
        outFile << data << currentContent;  // Prepend data to the existing content
    } else {
        console.error("Stream is not writable!");
    }
}

void File::setAutoFlush(bool autoFlush) {
    this->autoFlush = autoFlush;  // Set the flush mode
}

std::string File::getName() const {
    std::filesystem::path path(filePath);
    return path.filename().string();
}

std::string File::getPath() const {
    return filePath;
}

void File::close() {
    if (file.is_open()) {
        file.close();  // Close the file stream
    }
}

std::string File::getSize() {
    return std::to_string(static_cast<int>(std::filesystem::file_size(filePath)));  // Return the file size as a string
}

void File::clear() {
    // Open the file in write mode to overwrite the content
    file.close();  // Close the file first
    std::ofstream outFile(filePath, std::ios::trunc);  // Open file with truncation mode to clear it
    outFile.close();  // Close after clearing
}

bool File::exists() {
    return std::filesystem::exists(filePath);  // Return true if the file exists
}

void File::rename(const std::string& newPath) {
    std::filesystem::rename(filePath, newPath);  // Rename the file
    filePath = newPath;  // Update the internal file path
}

std::string File::getLastModified() {
    // auto ftime = std::filesystem::last_write_time(filePath);  // Get last modified time
    // auto sctp = std::chrono::system_clock::from_time_t(std::chrono::system_clock::to_time_t(ftime));
    // std::time_t lastModified = std::chrono::system_clock::to_time_t(sctp);
    // return std::string(std::ctime(&lastModified));  // Return the time as string
    return "";
}

void File::createBackup(const std::string& backupPath) {
    std::filesystem::copy(filePath, backupPath);  // Create a copy of the file
}

bool File::isOpen() {
    return file.is_open();  // Return true if the file is open
}

void File::remove() {
    if (std::filesystem::exists(filePath)) {
        std::filesystem::remove(filePath);  // Remove the file if it exists
    }
}

void File::truncate() {
    std::ofstream outFile(filePath, std::ios::trunc);  // Open file in truncation mode
    outFile.close();  // This will truncate the file
}

std::string File::getExtension() {
    std::filesystem::path filePathObj(filePath);
    return filePathObj.extension().string();  // Return the file's extension
}

void File::setPermissions(const std::string& permissions) {
    // Convert permissions string to actual mode (simplified for this example)
    std::filesystem::perms perms = std::filesystem::perms::none;
    if (permissions == "read") {
        perms = std::filesystem::perms::owner_read;
    } else if (permissions == "write") {
        perms = std::filesystem::perms::owner_write;
    } else if (permissions == "execute") {
        perms = std::filesystem::perms::owner_exec;
    }
    std::filesystem::permissions(filePath, perms);  // Set the file's permissions
}

void File::seekg(const ArgumentDefinition& args) {
    if (args.size() == 1) {
        std::streampos pos = std::stoll(std::get<std::string>(args[0]));
        file.seekg(pos);
    } else {
        console.error("File.seekg() expects 1 argument");
    }
}

void File::seekp(const ArgumentDefinition& args) {
    if (args.size() == 1) {
        std::streampos pos = std::stoll(std::get<std::string>(args[0]));
        file.seekp(pos);
    } else {
        console.error("File.seekp() expects 1 argument");
    }
}

long long File::tellg(const ArgumentDefinition& args) {
    if (args.size() > 0) {
        console.error("File.tellg() doesn't take any arguments");
    }

    return static_cast<long long>(file.tellg());
    return -1;
}

long long File::tellp(const ArgumentDefinition& args) {
    if (args.size() > 0) {
        console.error("File.tellp() doesn't take any arguments");
    }

    return static_cast<long long>(file.tellp());
    return -1;
}

bool File::eof(const ArgumentDefinition& args) {
    if (args.size() > 0) {
        console.error("File.eof() doesn't take any arguments");
    }

    return file.eof();
    return false;
}

bool File::fail(const ArgumentDefinition& args) {
    if (args.size() > 0) {
        console.error("File.fail() doesn't take any arguments");
    }

    return file.fail();
    return false;
}

bool File::good(const ArgumentDefinition& args) {
    if (args.size() > 0) {
        console.error("File.good() doesn't take any arguments");
    }

    
    return file.good();
    return false;
}

void File::clearFlags(const ArgumentDefinition& args) {
    if (args.size() > 0) {
        console.error("File.clearFlags() doesn't take any arguments");
    }

    file.clear();
}