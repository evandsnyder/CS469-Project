//
// Created by arch1t3ct on 9/23/20.
//

#ifndef CS469_PROJECT_NETWORK_H
#define CS469_PROJECT_NETWORK_H

#include <netdb.h>
#include <errno.h>
#include <resolv.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

const SSL_METHOD* method;
int sockfd;
SSL_CTX *ssl_ctx;
SSL *ssl;


int create_socket(char* hostname, unsigned int port);
int database_connect(char* hostname, int port);
int disconnect();


#endif //CS469_PROJECT_NETWORK_H
