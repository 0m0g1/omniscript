#ifndef PATH_H
#define PATH_H

#include <omniscript/omniscript_pch.h>
#include <omniscript/runtime/object.h>

class Path : public Object {
public:
    Path();

    // Register methods
    void registerMethods();

    // Path operations
    std::string join(const std::vector<std::string>& paths);
    std::string dirname(const std::string& path);
    std::string basename(const std::string& path, const std::string& ext = "");
    std::string extname(const std::string& path);
    std::string resolve(const std::vector<std::string>& paths);
    std::string normalize(const std::string& path);
    bool isAbsolute(const std::string& path);

private:
    std::string removeTrailingSlashes(const std::string& path);
};

#endif // PATH_H