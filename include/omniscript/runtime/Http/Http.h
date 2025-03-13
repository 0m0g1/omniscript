// #ifndef HTTP_H
// #define HTTP_H
// #include <omniscript/omniscript_pch.h>
// #include <omniscript/runtime/object.h>

// // using libcurl
// class HTTP : public Object {
// public:
//     HTTP();
//     std::string get(const std::string& url, const std::map<std::string, std::string>& headers = {});
//     std::string post(const std::string& url, const std::string& data, const std::map<std::string, std::string>& headers = {});
//     std::string put(const std::string& url, const std::string& data, const std::map<std::string, std::string>& headers = {});
//     std::string del(const std::string& url, const std::map<std::string, std::string>& headers = {});
//     std::map<std::string, std::string> head(const std::string& url, const std::map<std::string, std::string>& headers = {});

// private:
//     void registerMethods();
//     void registerProperties();
//     static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userData);  // Static method
//     CURL* setupCurl(const std::string& url, const std::map<std::string, std::string>& headers, std::string& responseBuffer);
// };

// #endif
