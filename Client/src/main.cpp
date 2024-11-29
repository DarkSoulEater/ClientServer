#include "Client.hpp"
#include "ParseArg.hpp"

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

int main(const int argc, const char* argv[]) {
    OPENSSL_init_ssl(0, nullptr);
    
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    #if OPENSSL_VERSION_MAJOR < 3
    ERR_load_BIO_strings(); // deprecated since OpenSSL 3.0
    #endif
    ERR_load_crypto_strings();

    auto args = ParseArgs(argc, argv, Device::Client);
    Client client(args.proto, args.port, args.ip);
    client.Start();
    return 0;
}