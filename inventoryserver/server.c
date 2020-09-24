#include <stdio.h>
#include <argp.h>
#include <stdlib.h>

#include "../globals.h"
#include "network.h"

struct Arguments {
    int listenPort;
    char *server;
    int backupPort;
    char *filename;
};

static struct argp_option options[] = {
        {"listen-port",'l',"<port>", 0, "Port to listen on. Default: 4466"},
        {"backup-inventoryserver", 's', "<inventoryserver>", 0, "Server to backup to. Default: localhost"},
        {"backup-port", 'p', "<port", 0, "Port of backup inventoryserver. Default: 6644"},
        {"config-file", 'f', "<filename>", 0, "A config file that can be used in lieu of CLI arguments. This will override all CLI arguments."},
        {"backup-interval",'i',"<n:H>", 0, "How frequently to backup the database. The time format is time:unit. Acceptable units are [H]ours, [m]inutes, [s]econds. Default: 24:H"},
        {0}
};

static error_t parse_args(int key, char *arg, struct argp_state *state){
    struct Arguments *arguments = state->input;
    char *pEnd;


    switch(key){
        case 'l':
            arguments->listenPort = strtol(arg, &pEnd,10);
            break;
        case 's':
            arguments->server = arg;
            break;
        case 'p':
            arguments->backupPort = strtol(arg, &pEnd,10);
            break;
        case 'f':
            arguments->filename = arg;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}


struct argp argp = { options, parse_args, 0, "A program to manage remote database queries."};
int main(int argc, char *argv[]){
    struct Arguments arguments = {0};
    arguments.listenPort = DEFAULT_SERVER_PORT;
    arguments.server = DEFAULT_SERVER;
    arguments.backupPort = DEFAULT_BACKUP_PORT;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    printf("Hello from inventoryserver!\n");
    printf("Provided args:\n");
    printf("\tListen port: %d\n", arguments.listenPort);
    printf("\tServer: %s:%d\n", arguments.server, arguments.backupPort);
    printf("\tConfig file: %s\n", arguments.filename ? arguments.filename: "NULL");


    // start listening
    // handle requests

    SSL_CTX* ssl_ctx;
    unsigned int sockfd;
    unsigned int port;
    char buffer[BUFFER_SIZE];

    init_openssl();
    ssl_ctx = create_new_context();
    configure_context(ssl_ctx);
    sockfd = create_socket(arguments.listenPort);
    while(true){
        SSL* ssl;
        int client;
        int readfd;
        int rcount;
        const char reply[] = "PLACEHOLDER";
        struct sockaddr_in addr;
        unsigned int len = sizeof(addr);
        char client_addr[INET_ADDRSTRLEN];

        client = accept(sockfd, (struct sockaddr*)&addr, &len);
        if(client < 0){
            break;
        }

        inet_ntop(AF_INET, (struct in_addr*)&addr.sin_addr, client_addr, INET_ADDRSTRLEN);
        fprintf(stdout, "Server: Established TCP connection with client (%s) on port %u\n", client_addr, port);

        ssl = SSL_new(ssl_ctx);
        SSL_set_fd(ssl, client);
        if(SSL_accept(ssl) <= 0){
            fprintf(stderr, "Server: Could not establish a secure connection:\n");
            ERR_print_errors_fp(stderr);
        }
    }

    SSL_CTX_free(ssl_ctx);
    cleanup_openssl();
    close(sockfd);

    return 0;
}