#pragma once
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <string>
#include <format>
#include "DataBuffer.hpp"
#include "Console.hpp"

#define Log(MSG)                                                \
    if (console_) {                                             \
        if (is_server_) {                                       \
            console_->Print(std::format("[TLS]: [{}] {}", id_, (MSG)));   \
        } else {                                                \
            console_->Print(std::format("[TLS]: {}", (MSG)));   \
        }                                                       \
    }


#define Err(MSG)                                                    \
    if (console_) {                                                 \
        console_->Print(std::format("[TLS]: ERROR!: {}", (MSG)));   \
    }

#define GetState() std::format("STATE: {}", SSL_state_string_long(ssl_))

class TLS {
  private:
    SSL *ssl_;
    BIO *input_;
    BIO *output_;
    bool is_server_;
    Console* console_;
    ID id_;

    enum SSLStatus {
        OK,
        WantIO,
        Fail
    };

    void print_ssl_error() { // TODO:
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
            } else if (!BIO_should_retry(bio)) {
                Err("BIORead faild");
                return "";
            }
        } while (cnt > 0);
        return res;
    }

  public:
    std::string Handshake() {
        Log("Send handshake")
        Log(GetState());
        int res = SSL_do_handshake(ssl_);
        Log(GetState());

        auto status = SSLGetStatus(ssl_, res);
        if (status == SSLStatus::WantIO) {
            return BIORead(output_);
        }
        return std::string();
    }

    TLS(SSL_CTX* ctx, bool is_server, Console* console = nullptr)
            : is_server_(is_server)
            , console_(console) {
        ssl_ = SSL_new(ctx);

        if (is_server) {
            SSL_set_accept_state(ssl_);
        } else {
            SSL_set_connect_state(ssl_);
        }

        input_ = BIO_new(BIO_s_mem());
        output_ = BIO_new(BIO_s_mem());

        SSL_set_bio(ssl_, input_, output_);
    }

    std::string Decode(DataBuffer& data) {
        std::string want_send_data;
        size_t data_size = data.Size();
        size_t data_start = 0;
        while (data_size) {
            int cnt = BIO_write(input_, data.Buffer() + data_start, data_size);
            if (cnt <= 0) {
                Err("decode write in bio falid");
                data.Clear();
                break;
            }

            data_start += cnt;
            data_size -= cnt;

            if (!SSL_is_init_finished(ssl_)) {
                Log(GetState());
                int res = SSL_do_handshake(ssl_);
                Log(GetState());
            }

            int res;
            char buff[1024];
            std::string str;
            do {
                res = SSL_read(ssl_, buff, sizeof(buff));
                if (res > 0) {
                    str.append(buff, res);
                }
            } while(res > 0);

            data.Clear();
            if (!str.empty()){
                data.Resize(str.size());
                memcpy(data.Buffer(), str.c_str(), str.size());
            }

            auto status = SSLGetStatus(ssl_, res);
            if (status == SSLStatus::WantIO) {
                want_send_data += BIORead(output_);
            }

            if (status == SSLStatus::Fail) {
                print_ssl_error();
                Err("Decode faild");
            }
        }
        return want_send_data;
    }

    void Encode(std::string& msg) {
        if (!SSL_is_init_finished(ssl_)) {
            return;
        }

        SSL_write(ssl_, msg.c_str(), msg.size());
        msg = BIORead(output_);
    }

    void SetID(ID id) {
        id_ = id;
    }
};

#undef Log