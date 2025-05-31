/* Simple test program for libcrypto (OpenSSL) CMAC ([found on](https://gist.github.com/enkore/56c756d32197f65ae7769e7f9e0a5d35))
 */

import std;
#include <stdlib.h>

#include <openssl/cmac.h>
#include <openssl/core_names.h>
#include <openssl/evp.h>

template<auto fn>
using fn_constant = std::integral_constant<decltype(fn), fn>;
using cmac_ptr = std::unique_ptr<EVP_MAC, fn_constant<EVP_MAC_free>>;
using cmac_ctx_ptr = std::unique_ptr<EVP_MAC_CTX, fn_constant<EVP_MAC_CTX_free>>;

int wmain()
{
    cmac_ptr cmac{EVP_MAC_fetch(nullptr, "CMAC", nullptr)};
    if (!cmac) {
        return EXIT_FAILURE;
    }
    cmac_ctx_ptr ctx{EVP_MAC_CTX_new(cmac.get())};
    if (!cmac) {
        return EXIT_FAILURE;
    }

    /* The underlying cipher to be used */
    const EVP_CIPHER* cipher = EVP_aes_256_cbc();
    const char* cypherName = EVP_CIPHER_get0_name(cipher);
    const int keyLength = EVP_CIPHER_get_key_length(cipher);
    const OSSL_PARAM params[2] = {OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_CIPHER, const_cast<char*>(cypherName), 0),
                                  OSSL_PARAM_construct_end()};

    std::vector<unsigned char> key(keyLength, '\0');
    unsigned int msgLength = 1024 * 1024 * 1024;
    std::vector<unsigned char> msg(msgLength, '\0');

    auto t0 = std::chrono::steady_clock::now();
    if (!EVP_MAC_init(ctx.get(), key.data(), key.size(), params)) {
        return EXIT_FAILURE;
    }
    if (!EVP_MAC_update(ctx.get(), msg.data(), msgLength)) {
        return EXIT_FAILURE;
    }
    size_t resultLength = 0;
    EVP_MAC_final(ctx.get(), nullptr, &resultLength, 0);
    std::vector<unsigned char> result(resultLength, '\0');
    if (!EVP_MAC_final(ctx.get(), result.data(), &resultLength, resultLength)) {
        return EXIT_FAILURE;
    }
    auto t1 = std::chrono::steady_clock::now();
    std::chrono::duration<double> tdelta = t1 - t0;

    std::wcout << std::format(L"CMAC-AES-256'd {} bytes in {} ({:.1f} MiB/s)", msgLength, tdelta, msgLength / tdelta.count() / 1024 / 1024);

    return 0;
}
