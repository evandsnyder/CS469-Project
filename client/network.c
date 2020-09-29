#include "network.h"

int create_socket(char* hostname, unsigned int port){
    int sockfd;
    struct hostent* host;
    struct sockaddr_in dest_addr;

    host = gethostbyname(hostname);
    if(host == NULL){
        fprintf(stderr, "Could not obtain hostname\n");
        return -1;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        fprintf(stderr, "Could not create socket\n");
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

int database_connect(char* hostname, int port) {
    OpenSSL_add_all_algorithms();
    if (SSL_library_init() < 0) {
        fprintf(stderr, "Could not initialize SSL\n");
        return -1;
    }

    method = SSLv23_client_method();

    ssl_ctx = SSL_CTX_new(method);
    if (ssl_ctx == NULL) {
        fprintf(stderr, "Could not create SSL Context\n");
        return -1;
    }


    SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv2);
    ssl = SSL_new(ssl_ctx);

    sockfd = create_socket(hostname, port);
    if (sockfd <= 0) {
        fprintf(stderr, "Could not create Socket SSL\n");
        return -1;
    }

    SSL_set_fd(ssl, sockfd);

    if (SSL_connect(ssl) != 1) {
        fprintf(stderr, "Could not establish secure connection\n");
        return -1;
    }

    return 1;
}

int disconnect(){
    SSL_free(ssl);
    SSL_CTX_free(ssl_ctx);
    close(sockfd);
}
