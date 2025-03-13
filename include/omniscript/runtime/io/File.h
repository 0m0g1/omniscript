#ifndef File_H
#define File_H

#include <omniscript/omniscript_pch.h>
#include <omniscript/runtime/object.h>
#include <omniscript/debuggingtools/console.h>

class File : public Object {
public:
    // Constructor to open the file with a specific mode (READ/WRITE)
    File(const std::string& path, std::ios::openmode mode);

    // Method to get the file content as a string (for reading)
    std::string getAsString();

    // Method to write to the file
    void write(const std::string& data);
    void write(const std::vector<char>& data);
    void writeln(const std::string& data);
    void writeln(const std::vector<char>& data);

    // Method to flush the file stream
    void flush();

    // Method to set auto flush the file stream
    void setAutoFlush(bool autoFlush);
    void append(const std::string& data);
    void prepend(const std::string& data);
    std::vector<char> getBinaryContent();
    std::string getSize();
    void clear();
    bool exists();
    void rename(const std::string& newPath);
    std::string getLastModified();
    void createBackup(const std::string& backupPath);
    bool isOpen();
    void remove();
    void truncate();
    void seekg(const ArgumentDefinition& args);
    void seekp(const ArgumentDefinition& args);
    long long tellg(const ArgumentDefinition& args);
    long long tellp(const ArgumentDefinition& args);
    bool eof(const ArgumentDefinition& args);
    bool fail(const ArgumentDefinition& args);
    bool good(const ArgumentDefinition& args);
    void clearFlags(const ArgumentDefinition& args);
    std::string getExtension();
    void setPermissions(const std::string& permissions);
    std::string toString(int indentLevel = 0) const override;

    // Method to get the file name
    std::string getName() const;

    // Method to get the file path
    std::string getPath() const;

    // Close the file
    void close();

     // Lock the file
    bool lock() {
        HANDLE hFile = CreateFile(filePath.c_str(), GENERIC_READ | GENERIC_WRITE, 
                                  0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            console.error("Failed to open file for locking!");
            return false;
        }

        // Lock the file (overlapping lock)
        OVERLAPPED overlapped = { 0 };
        if (!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, 0xFFFFFFFF, 0xFFFFFFFF, &overlapped)) {
            console.error("Failed to lock file!");
            return false;
        }
        fileHandle = hFile;
        return true;
    }

    // Unlock the file
    bool unlock() {
        if (!fileHandle) {
            console.error("No file to unlock!");
            return false;
        }

        // Unlock the file
        if (!UnlockFile(fileHandle, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF)) {
            console.error("Failed to unlock file!");
            return false;
        }
        CloseHandle(fileHandle);
        return true;
    }

    // Destructor to ensure file is closed when object is destroyed
    ~File();

private:
    bool autoFlush = true;
    std::fstream file;  // Stream object to handle file
    std::string fileName;
    std::string filePath;  // File path
    HANDLE fileHandle = NULL;
};

#endif