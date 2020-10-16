//
// Created by arch1t3ct on 9/23/20.
//

#include "network.h"
#include <netdb.h>

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

int create_client_socket(char* hostname, unsigned int port){
    int sockfd;
    struct hostent* host;
    struct sockaddr_in dest_addr;

    host = gethostbyname(hostname);
    if(host == NULL){
        fprintf(stderr, "Could not obtain hostname: %s\n", strerror(errno));
        return -1;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        fprintf(stderr, "Could not create socket: %s\n", strerror(errno));
        return -1;
    }

    dest_addr.sin_family=AF_INET;
    dest_addr.sin_port = htons(port);
    dest_addr.sin_addr.s_addr = *(long*)(host->h_addr);

    if(connect(sockfd, (struct sockaddr *) &dest_addr, sizeof(struct sockaddr)) < 0){
        fprintf(stderr, "Could not connect: %s\n", strerror(errno));
        return -1;
    }

    return sockfd;
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

SSL_CTX* create_new_client_context() {
	OpenSSL_add_all_algorithms();
    if (SSL_library_init() < 0) {
        fprintf(stderr, "Could not initialize SSL\n");
        return NULL;
    }

    const SSL_METHOD * method = SSLv23_client_method();

    SSL_CTX * ssl_ctx = SSL_CTX_new(method);
    if (ssl_ctx == NULL) {
        fprintf(stderr, "Could not create SSL Context\n");
        return NULL;
    }

    SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv2);

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

