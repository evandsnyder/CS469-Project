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


/**
 * Creates, binds, and starts listening on a server socket bound to the given
 * port.
 *
 * @param port The local port to bind to.
 * @return An open and listening socket fd, or -1 on error
 */
int create_socket(unsigned int port);

/**
 * Helper functions to initialize and shutdown the openssl library for the
 * application.
 */
void init_openssl();
void cleanup_openssl();

/**
 * Creates a new SSL_CTX for an openssl server
 *
 * @return A pointer to the new SSL_CTX, or NULL on error
 */
SSL_CTX* create_new_context();

/**
 * Configures the given SSL_CTX to use the appropriate encryption configuration.
 * Loads the defined CERTIFICATE_FILE and KEY_FILE.
 *
 * @param ssl_ctx The context to configure.
 * @return 0 on success, -1 on error
 */
int configure_context(SSL_CTX* ssl_ctx);



#endif //CS469_PROJECT_NETWORK_H
