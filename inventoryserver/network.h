//
// Created by arch1t3ct on 9/23/20.
//

#ifndef CS469_PROJECT_NETWORK_H
#define CS469_PROJECT_NETWORK_H

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define CERTIFICATE_FILE  "cert.pem"
#define KEY_FILE          "key.pem"
#define USE_OPENSSL

static pthread_mutex_t *lockarray;

int create_socket(unsigned int port);
void init_openssl();
void cleanup_openssl();
SSL_CTX* create_new_context();
int configure_context(SSL_CTX* ssl_ctx);



#endif //CS469_PROJECT_NETWORK_H
