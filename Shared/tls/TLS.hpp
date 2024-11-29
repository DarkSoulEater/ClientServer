#pragma once
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <string>
#include <format>
#include "DataBuffer.hpp"
#include "Console.hpp"

#define Log(MSG)            \
    if (console_) {         \
        console_->Log(MSG);  \
    }

class TLS {
  private:
    SSL *ssl_;
    BIO *input_;
    BIO *output_;
    bool is_server_;
    Console* console_;

    enum SSLStatus {
        OK,
        WantIO,
        Fail
    };

    void print_ssl_error()
    {
        BIO *bio = BIO_new(BIO_s_mem());
        ERR_print_errors(bio);
        char *buf;
        size_t len = BIO_get_mem_data(bio, &buf);
        if (len > 0) {
            Log(std::format("SLL-ERROR: {}", buf));
        }
        BIO_free(bio);
    }

    SSLStatus SSLGetStatus(SSL* ssl, int n) {
        switch (SSL_get_error(ssl, n))
        {
        case SSL_ERROR_NONE:
            return SSLStatus::OK;

        case SSL_ERROR_WANT_WRITE:
        case SSL_ERROR_WANT_READ:
            return SSLStatus::WantIO;
        default:
            return SSLStatus::Fail;
        }
    }


    std::string BIORead(BIO* bio) {
        std::string res;
        char buff[1024];
        int cnt = 0;
        do {
            cnt = BIO_read(bio, buff, sizeof(buff));
            if (cnt > 0) {
                res.append(buff, cnt);
            } 
            // else if (!BIO_should_retry(bio)) {
            //     ret
            // }
        } while (cnt > 0);
        return res;
    }

  public:
    std::string Handshake() {
        // console_.Log(std::format("SSL-STATE: {}", SSL_state_string_long(encr_point_->ssl_)));
        int res = SSL_do_handshake(ssl_);
        // console_.Log(std::format("SSL-STATE: {}", SSL_state_string_long(encr_point_->ssl_)));

        auto status = SSLGetStatus(ssl_, res);
        if (status == SSLStatus::WantIO) {
            return BIORead(output_);
        }
        return std::string();
    }

    TLS(SSL_CTX* ctx, bool is_server, Console* console = nullptr) : is_server_(is_server), console_(console) {
        ssl_ = SSL_new(ctx);

        if (is_server) {
            SSL_set_accept_state(ssl_);
        } else {
            SSL_set_connect_state(ssl_);
        }

        input_ = BIO_new(BIO_s_mem());
        output_ = BIO_new(BIO_s_mem());

        SSL_set_bio(ssl_, input_, output_);

        // if (!is_server) {
        //     Handshake();
        // }
    }

    std::string Decode(DataBuffer& data) {
        std::string want_send_data;
        size_t data_size = data.Size();
        size_t data_start = 0;
        while (data_size) {
            int cnt = BIO_write(input_, data.Buffer() + data_start, data_size);
            if (cnt <= 0) {
                Log("Error");
                // TODO:
                data.Clear();
                break;
            }

            data_start += cnt;
            data_size -= cnt;

            if (!SSL_is_init_finished(ssl_)) {
                Log(std::format("SSL-STATE: {}", SSL_state_string_long(ssl_)));
                int res = SSL_do_handshake(ssl_);
                Log(std::format("SSL-STATE: {}", SSL_state_string_long(ssl_)));
                // if (HandShake()) {
                //     console_.Log("");
                //     break;
                // }

                // if (!SSL_is_init_finished(ssl_)) {
                //     data.Clear();
                //     break;
                // }
            }

            Log(std::format("recv: [{}]", data.Size()));

            int res;
            char buff[1024];
            std::string str;
            do {
                res = SSL_read(ssl_, buff, sizeof(buff));
                if (res > 0) {
                    str.append(buff, res);
                    Log("READ");
                } else {
                    Log("STOP");
                }
            } while(res > 0);
            data.Clear(); // TODO:
            if (!str.empty()){
                data.Resize(str.size());
                memcpy(data.Buffer(), str.c_str(), str.size());
                // std::swap(data, DataBuffer(str));
            }

            auto status = SSLGetStatus(ssl_, res);
            // console_.Log(std::format("Status {}", (int)status));
            if (status == SSLStatus::WantIO) {
                // console_.Log("STATUS WANT IO");
                want_send_data += BIORead(output_);
                // Send(want_data);
            }

            if (status == SSLStatus::Fail) {
                print_ssl_error();
            }

            // console_.Log(std::format("Server: \"{}\"", data.Buffer()));
        }
        // Log(std::format("{}", want_send_data));
        return want_send_data;
    }

    void Encode(std::string& msg) {
        if (SSL_is_init_finished(ssl_)) {
            Log("SH not finished");
            return;
        }

        SSL_write(ssl_, msg.c_str(), msg.size());
        msg = BIORead(output_);
    }
};

#undef Log

// enum SSLStatus {
//     OK,
//     WantIO,
//     Fail
// };

// SSLStatus SSLGetStatus(SSL* ssl, int n) {
//     switch (SSL_get_error(ssl, n))
//     {
//     case SSL_ERROR_NONE:
//         return SSLStatus::OK;

//     case SSL_ERROR_WANT_WRITE:
//     case SSL_ERROR_WANT_READ:
//         return SSLStatus::WantIO;
//     default:
//         return SSLStatus::Fail;
//     }
// }

// std::string BIORead(BIO* bio) {
//     std::string res;
//     char buff[1024];
//     int cnt = 0;
//     do {
//         cnt = BIO_read(bio, buff, sizeof(buff));
//         if (cnt > 0) {
//             res.append(buff, cnt);
//         } 
//         // else if (!BIO_should_retry(bio)) {
//         //     ret
//         // }
//     } while (cnt > 0);
//     return res;
// }

// std::string SSLRead(SSL* sll) {

// }