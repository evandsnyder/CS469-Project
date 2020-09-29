#include <stdio.h>
#include <argp.h>
#include <stdlib.h>

#include "../globals.h"
#include "network.h"

struct Arguments {
    int listenPort;
};
static struct argp_option options[] = {
        {"listen-port",'l',"<port>", 0, "Port to listen on. Default: 6644"},
        {0}
};

static error_t parse_args(int key, char *arg, struct argp_state *state){
    struct Arguments *arguments = state->input;
    char *pEnd;

    if(key != 'l'){
        return ARGP_ERR_UNKNOWN;
    }

    arguments->listenPort = strtol(arg, &pEnd,10);
    return 0;
}

struct argp argp = { options, parse_args, 0, "A program to manage remote database queries."};
int main(int argc, char *argv[]){
    struct Arguments arguments = {0};
    arguments.listenPort = DEFAULT_BACKUP_PORT;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    printf("Hello from datastore!\n");

    int serverFd = create_socket(arguments.listenPort);
    if (serverFd < 0) {
        fprintf(stderr, "Could not create socket\n");
        exit(-1);
    }

    SSL_CTX * ssl_ctx = create_new_context();
    configure_context(ssl_ctx);

    while (1) {
        // accept the client
        int clientFd = accept(serverFd, NULL, NULL);
        if (clientFd < 0) {
            fprintf(stderr, "Could not create client socket\n");
            break;
        }

        // start up ssl
        SSL * ssl = SSL_new(ssl_ctx);
        if (!ssl) {
            fprintf(stderr, "Could not create SSL*\n");
            break;
        }
        if (SSL_set_fd(ssl, clientFd) < 0) {
            fprintf(stderr, "Could not set fd\n");
            break;
        }
        if (SSL_accept(ssl) != 1) {
            fprintf(stderr, "Could not establish secure connection\n");
            ERR_print_errors_fp(stderr);
            break;
        }

        // TODO: accept replication auth, command, and data
        char buffer[BUFFER_SIZE];
        int rcount = SSL_read(ssl, buffer, BUFFER_SIZE);
        if (rcount < 0) {
            fprintf(stderr, "Could not read data\n");
            ERR_print_errors_fp(stderr);
            break;
        }

        // shut it down
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(clientFd);
    }

    return 0;
}
