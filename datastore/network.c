#include "network.h"

int create_socket(unsigned int port){
    int s;
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    s = socket(AF_INET, SOCK_STREAM, 0);
    if(s < 0){
        return -1;
    }

    if(bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        return -1;

    if(listen(s, 1) < 0)
        return -1;

    return s;
}

void init_openssl(){
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl(){
    EVP_cleanup();
}

SSL_CTX* create_new_context(){
    const SSL_METHOD* ssl_method;
    SSL_CTX* ssl_ctx;

    ssl_method = SSLv23_server_method();
    ssl_ctx = SSL_CTX_new(ssl_method);
    if(ssl_ctx == NULL){
        return NULL;
    }

    return ssl_ctx;
}

int configure_context(SSL_CTX* ssl_ctx){
    SSL_CTX_set_ecdh_auto(ssl_ctx, 1);

    if(SSL_CTX_use_certificate_file(ssl_ctx, CERTIFICATE_FILE, SSL_FILETYPE_PEM) <= 0)
        return -1;

    if(SSL_CTX_use_PrivateKey_file(ssl_ctx, KEY_FILE, SSL_FILETYPE_PEM) <= 0)
        return -1;

    return 0;
}

