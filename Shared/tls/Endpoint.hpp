#pragma once

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

class EncryptPoint {
  public:
    SSL *ssl_;
    BIO *input_;
    BIO *output_;
    bool is_server_;

    enum SSLStatus {
        OK,
        WantIO,
        Fail
    };

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


void print_ssl_state()
{
  const char * current_state = SSL_state_string_long(ssl_);
  printf("SSL-STATE: %s\n", current_state);
//   if (current_state != client.last_state) {
//     if (current_state)
//       printf("SSL-STATE: %s\n", current_state);
//     client.last_state = current_state;
//   }
}

  public:
    EncryptPoint(SSL_CTX* ctx, bool is_server) : is_server_(is_server) {
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

    bool Handshake(std::string& s) {
        assert(is_server_ == false);
        HNAD:{}
        char buf[1024];
        print_ssl_state();
        int rs = SSL_do_handshake(ssl_);
        print_ssl_state();
        if (rs != 1) {
            if (rs == 0) {
                perror("hs0");
            } else {
                perror("hs<0");
            }
            auto err = SSL_get_error(ssl_, rs);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                int n = 0;
                int k = 0;
                do {    
                    std::cout << "K = " << k++ << "\n";
                    n = BIO_read(output_, buf, sizeof(buf));
                    std::cerr << "OK\n";
                if (n > 0) {
                    // queue_encrypted_bytes(buf, n);
                    for (int i = 0; i < n; ++i) {
                        std::cerr << buf[i];
                    }
                    std::cerr << "\n";
                    s.append(buf, n);
                } else if (!BIO_should_retry(output_))
                    return false;
                } while (n>0);
                perror("OKKKK");
            } else {
                ERR_print_errors_fp(stderr);
            }
            // ERR_print_errors_fp(stderr);
            return false;
        }
        return true;
    }

    std::string Decode(DataBuffer& data) {
        std::string want_send_data;
        size_t data_size = data.Size();
        size_t data_start = 0;
        while (data_size) {
            int cnt = BIO_write(input_, data.Buffer() + data_start, data_size);
            if (cnt <= 0) {
                // console_.Log("");
                // TODO:
                data.Clear();
                break;
            }

            data_start += cnt;
            data_size -= cnt;

            if (!SSL_is_init_finished(ssl_)) {
                // console_.Log(std::format("SSL-STATE: {}", SSL_state_string_long(encr_point_->ssl_)));
                int res = SSL_do_handshake(ssl_);
                // console_.Log(std::format("SSL-STATE: {}", SSL_state_string_long(encr_point_->ssl_)));
                // if (HandShake()) {
                //     console_.Log("");
                //     break;
                // }

                if (!SSL_is_init_finished(ssl_)) {
                    data.Clear();
                    break;
                }
            }

            int res;
            char buff[1024];
            do {
                res = SSL_read(ssl_, buff, sizeof(buff));
            } while(res > 0);

            data.Clear(); // TODO:

            auto status = SSLGetStatus(ssl_, res);
            // console_.Log(std::format("Status {}", (int)status));
            if (status == SSLStatus::WantIO) {
                // console_.Log("STATUS WANT IO");
                want_send_data += BIORead(output_);
                // Send(want_data);
            }

            // console_.Log(std::format("Server: \"{}\"", data.Buffer()));
        }
        return want_send_data;
    }
    // DataBuffer Encode(Data)
};

struct Endpoint {
    Endpoint(int sock, bool is_server, const SSL_METHOD* method) {
        ctx_ = SSL_CTX_new(method);
        if (!ctx_) {
            // perror();
            ERR_print_errors_fp(stderr);
            abort();
        }

        SSL_CTX_set_min_proto_version(ctx_, TLS1_2_VERSION);
        SSL_CTX_set_options(ctx_, SSL_OP_ALL);
        SSL_CTX_set_default_verify_paths(ctx_);

        if (is_server) {
            SSL_CTX_use_certificate_file(ctx_, "./ca/tls_self_signed_certificate.crt", SSL_FILETYPE_PEM);
            SSL_CTX_use_PrivateKey_file(ctx_, "./ca/tls_private_key.key", SSL_FILETYPE_PEM);

            if (!SSL_CTX_check_private_key(ctx_)) {
                perror("private key check");
                abort();
            }
        }

        ssl_ = SSL_new(ctx_);

        if (is_server) {
            SSL_set_accept_state(ssl_);
        } else {
            SSL_set_connect_state(ssl_);
        }

        // input_ = output_ = BIO_new_socket(sock, 0);
        // SSL_set_bio(ssl_, input_, output_);

        SSL_set_bio(ssl_, BIO_new(BIO_s_mem()), BIO_new(BIO_s_mem()));
        

        int rs = SSL_do_handshake(ssl_);
        if (rs != 1) {
            ERR_print_errors_fp(stderr);
            abort();
        }
    }

  public:
    SSL_CTX *ctx_;
    SSL *ssl_;
    BIO *input_;
    BIO *output_;
};

