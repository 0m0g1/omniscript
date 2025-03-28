// #include <omniscript/omniscript_pch.h>
// #include <omniscript/runtime/Http/Http.h>
// #include <omniscript/debuggingtools/console.h>
// #include <curl/curl.h>

// HTTP::HTTP() {
//     registerMethods();
//     registerProperties();
// }

// void HTTP::registerMethods() {
//     addMethod("get", [this](const ArgumentDefinition& args) {
//         if (!std::holds_alternative<std::string>(args[0])) {
//             console.error("Http.get() takes in two arguments: a string and an optional object for headers.");
//         }
//         std::string url = std::get<std::string>(args[0]);
//         return get(url);
//     });
//     addMethod("post", [this](const ArgumentDefinition& args) {
//         if (!std::holds_alternative<std::string>(args[0])) {
//             console.error("Http.post() takes in two arguments: a string and an optional object for headers.");
//         }
//         std::string url = std::get<std::string>(args[0]);
//         std::string data = std::get<std::string>(args[1]);
//         return post(url, data);
//     });
//     addMethod("put", [this](const ArgumentDefinition& args) {
//         if (!std::holds_alternative<std::string>(args[0])) {
//             console.error("Http.put() takes in two arguments: a string and an optional object for headers.");
//         }
//         std::string url = std::get<std::string>(args[0]);
//         std::string data = std::get<std::string>(args[1]);
//         return put(url, data);
//     });
//     addMethod("del", [this](const ArgumentDefinition& args) {
//         if (!std::holds_alternative<std::string>(args[0])) {
//             console.error("Http.del() takes in two arguments: a string and an optional object for headers.");
//         }
//         std::string url = std::get<std::string>(args[0]);
//         return del(url);
//     });
//     addMethod("head", [this](const ArgumentDefinition& args) {
//         if (!std::holds_alternative<std::string>(args[0])) {
//             console.error("Http.head() takes in two arguments: a string and an optional object for headers.");
//         }
//         std::string url = std::get<std::string>(args[0]);
//         // return head(url);
//         return SymbolTable::ValueType{};
//     });
// }

// void HTTP::registerProperties() {
//     // Implement property registration if needed
// }

// CURL* HTTP::setupCurl(const std::string& url, const std::map<std::string, std::string>& headers, std::string& responseBuffer) {
//     CURL* curl = curl_easy_init();
//     if (!curl) throw std::runtime_error("Failed to initialize cURL");

//     // Set URL
//     curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

//     // Set headers
//     struct curl_slist* curlHeaders = nullptr;
//     for (const auto& [key, value] : headers) {
//         std::string header = key + ": " + value;
//         curlHeaders = curl_slist_append(curlHeaders, header.c_str());
//     }
//     curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curlHeaders);

//     // Set write callback
//     curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HTTP::writeCallback);  // Correct use of static method
//     curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);

//     return curl;
// }

// size_t HTTP::writeCallback(void* contents, size_t size, size_t nmemb, std::string* userData) {
//     userData->append(static_cast<char*>(contents), size * nmemb);
//     return size * nmemb;
// }

// std::string HTTP::get(const std::string& url, const std::map<std::string, std::string>& headers) {
//     std::string responseBuffer;
//     CURL* curl = setupCurl(url, headers, responseBuffer);

//     CURLcode res = curl_easy_perform(curl);
//     if (res != CURLE_OK) throw std::runtime_error("GET request failed: " + std::string(curl_easy_strerror(res)));

//     curl_easy_cleanup(curl);
//     return responseBuffer;
// }

// std::string HTTP::post(const std::string& url, const std::string& data, const std::map<std::string, std::string>& headers) {
//     std::string responseBuffer;
//     CURL* curl = setupCurl(url, headers, responseBuffer);

//     // Set POST data
//     curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());

//     CURLcode res = curl_easy_perform(curl);
//     if (res != CURLE_OK) throw std::runtime_error("POST request failed: " + std::string(curl_easy_strerror(res)));

//     curl_easy_cleanup(curl);
//     return responseBuffer;
// }

// std::string HTTP::put(const std::string& url, const std::string& data, const std::map<std::string, std::string>& headers) {
//     std::string responseBuffer;
//     CURL* curl = setupCurl(url, headers, responseBuffer);

//     // Set PUT data
//     curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());

//     // Set the custom request type to PUT
//     curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");

//     CURLcode res = curl_easy_perform(curl);
//     if (res != CURLE_OK) {
//         curl_easy_cleanup(curl);
//         throw std::runtime_error("PUT request failed: " + std::string(curl_easy_strerror(res)));
//     }

//     curl_easy_cleanup(curl);
//     return responseBuffer;
// }

// std::string HTTP::del(const std::string& url, const std::map<std::string, std::string>& headers) {
//     std::string responseBuffer;
//     CURL* curl = setupCurl(url, headers, responseBuffer);

//     // Set DELETE method
//     curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");

//     CURLcode res = curl_easy_perform(curl);
//     if (res != CURLE_OK) throw std::runtime_error("DELETE request failed: " + std::string(curl_easy_strerror(res)));

//     curl_easy_cleanup(curl);
//     return responseBuffer;
// }

// std::map<std::string, std::string> HTTP::head(const std::string& url, const std::map<std::string, std::string>& headers) {
//     std::string responseBuffer;
//     CURL* curl = setupCurl(url, headers, responseBuffer);

//     // Set HEAD method
//     curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);

//     CURLcode res = curl_easy_perform(curl);
//     if (res != CURLE_OK) throw std::runtime_error("HEAD request failed: " + std::string(curl_easy_strerror(res)));

//     // Extract headers
//     std::map<std::string, std::string> responseHeaders;
//     char* contentType;
//     curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &contentType);
//     if (contentType) responseHeaders["Content-Type"] = contentType;

//     curl_easy_cleanup(curl);
//     return responseHeaders;
// }
