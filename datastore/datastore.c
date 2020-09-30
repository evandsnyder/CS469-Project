#include <stdio.h>
#include <argp.h>
#include <stdlib.h>

#include "../globals.h"
#include "network.h"

char secure_compare(char * bufa, char * bufb, size_t len);

struct Arguments {
    int listenPort;
    char *psk;
};
static struct argp_option options[] = {
        {"listen-port",'l',"<port>", 0, "Port to listen on. Default: 6644"},
        {"key",'k',"<key>", 0, "Pre-shared key used to authenticate remote server."},
        {0}
};

static error_t parse_args(int key, char *arg, struct argp_state *state){
    struct Arguments *arguments = state->input;
    char *pEnd;

    switch(key) {
        case 'l':
            arguments->listenPort = strtol(arg, &pEnd,10);
            break;
        case 'k':
            arguments->psk = arg;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

struct argp argp = { options, parse_args, 0, "A program to manage remote database queries."};
int main(int argc, char *argv[]){
    struct Arguments arguments = {0};
    arguments.listenPort = DEFAULT_BACKUP_PORT;
    arguments.psk = "";

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    printf("Hello from datastore!\n");

    int serverFd = create_socket(arguments.listenPort);
    if (serverFd < 0) {
        fprintf(stderr, "Could not create socket\n");
        exit(-1);
    }

    SSL_CTX * ssl_ctx = create_new_context();
    configure_context(ssl_ctx);

    char * command = malloc(strlen(arguments.psk) + strlen("REPLICATE \n") + 1);
    sprintf(command, "REPLICATE %s\n", arguments.psk);

    while (1) {
        // TODO: handle errors better than a break;

        // accept the client
        int clientFd = accept(serverFd, NULL, NULL);
        if (clientFd < 0) {
            fprintf(stderr, "Could not create client socket\n");
            break;
        }

        printf("Got client\n");

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

        // accept replication command and data
        // TODO: error handling
        char buffer[BUFFER_SIZE];
        int rcount;
        rcount = SSL_read(ssl, buffer, BUFFER_SIZE);
        if (rcount < strlen(command) || !secure_compare(buffer, command, strlen(command))) {
            fprintf(stderr, "Unknown command\n");
            break;
        }

        int fileFd = open("items.bk.db", O_WRONLY | O_CREAT, 0666);
        if (rcount > strlen(command)) {
            write(fileFd, buffer + strlen(command), rcount - strlen(command));
        }

        while((rcount = SSL_read(ssl, buffer, BUFFER_SIZE)) > 0) {
            write(fileFd, buffer, rcount);
        }
        close(fileFd);

        printf("copy done\n");

        SSL_write(ssl, "SUCCESS", strlen("SUCCESS"));

        printf("shutting down\n");
        // shut it down
        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(clientFd);

        printf("Client done\n");
    }

    free(command);

    return 0;
}

char secure_compare(char * bufa, char * bufb, size_t len) {
    char ret = 1;
    for(int i=0; i<len; ++i)
        if (bufa[i] != bufb[i])
            ret = 0;
    return ret;
}
