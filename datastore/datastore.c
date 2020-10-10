#include <stdio.h>
#include <argp.h>
#include <stdlib.h>

#include "../globals.h"
#include "network.h"

char secure_compare(char * bufa, char * bufb, size_t len);
void cleanup_connection(SSL * ssl, int clientFd);
int parse_conf_file(void *args);

struct Arguments {
    int listenPort;
    char *psk;
    char *filename;
};
static struct argp_option options[] = {
        {"listen-port",'l',"<port>", 0, "Port to listen on. Default: 6644"},
        {"key",'k',"<key>", 0, "Pre-shared key used to authenticate remote server."},
        {"config", 'c', "<filename>", 0, "A config file that can be used in lieu of CLI arguments. This will override all CLI arguments."},
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
        case 'c':
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
    arguments.listenPort = DEFAULT_BACKUP_PORT;
    arguments.psk = "";
    arguments.filename = NULL;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);
    if(arguments.filename != NULL) parse_conf_file(&arguments);

    printf("Hello from datastore!\n");
    printf("Provided args:\n");
    printf("\tListen port: %d\n", arguments.listenPort);
    printf("\tConfig file: %s\n", arguments.filename ? arguments.filename: "NULL");

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
        // TODO: errno, etc
        // accept the client
        int clientFd = accept(serverFd, NULL, NULL);
        if (clientFd < 0) {
            fprintf(stderr, "Could not create client socket\n");
            continue;
        }

        printf("Got client\n");

        // start up ssl
        SSL * ssl = SSL_new(ssl_ctx);
        if (!ssl) {
            fprintf(stderr, "Could not create SSL*\n");
            cleanup_connection(NULL, clientFd);
            continue;
        }
        if (SSL_set_fd(ssl, clientFd) < 0) {
            fprintf(stderr, "Could not set fd\n");
            cleanup_connection(ssl, clientFd);
            continue;
        }
        if (SSL_accept(ssl) != 1) {
            fprintf(stderr, "Could not establish secure connection\n");
            ERR_print_errors_fp(stderr);
            cleanup_connection(ssl, clientFd);
            continue;
        }

        // accept replication command and data
        char buffer[BUFFER_SIZE];
        int rcount;
        rcount = SSL_read(ssl, buffer, BUFFER_SIZE);
        if (rcount < strlen(command) || !secure_compare(buffer, command, strlen(command))) {
            fprintf(stderr, "Unknown command\n");
            cleanup_connection(ssl, clientFd);
            continue;
        }

        int fileFd = open("items.bk.db", O_WRONLY | O_CREAT, 0666);
        if (fileFd < 0) {
            fprintf(stderr, "Unable to open output file\n");
            cleanup_connection(ssl, clientFd);
            continue;
        }

        if (rcount > strlen(command)) {
            rcount = write(fileFd, buffer + strlen(command), rcount - strlen(command));
            if (rcount < 0) {
                fprintf(stderr, "Unable to write to output file\n");
                close(fileFd);
                cleanup_connection(ssl, clientFd);
                continue;
            }
        }

        while((rcount = SSL_read(ssl, buffer, BUFFER_SIZE)) > 0) {
            rcount = write(fileFd, buffer, rcount);
            if (rcount < 0)
                break;
        }
        close(fileFd);

        if ((SSL_get_shutdown(ssl) & SSL_RECEIVED_SHUTDOWN) == 0) {
            // we stopped getting data but the server didn't shut things down
            // must be an error
            fprintf(stderr, "Unable to write to output file\n");
            cleanup_connection(ssl, clientFd);
            continue;
        }

        printf("copy done\n");

        rcount = SSL_write(ssl, "SUCCESS", strlen("SUCCESS"));
        if (rcount <= 0) {
            fprintf(stderr, "Unable to send success message\n");
            cleanup_connection(ssl, clientFd);
            continue;
        }

        printf("shutting down\n");
        SSL_shutdown(ssl);
        cleanup_connection(ssl, clientFd);

        printf("Client done\n");
    }

    free(command);

    return 0;
}

void cleanup_connection(SSL * ssl, int clientFd) {
    if (ssl)
        SSL_free(ssl);
    close(clientFd);
}

char secure_compare(char * bufa, char * bufb, size_t len) {
    char ret = 1;
    for(int i=0; i<len; ++i)
        if (bufa[i] != bufb[i])
            ret = 0;
    return ret;
}

int parse_conf_file(void *args){
    struct Arguments *arguments = (struct Arguments *)args;
    char field[BUFFER_SIZE];
    char value[BUFFER_SIZE];
    int val;
    char *stop;

    FILE *file;
    file = fopen(arguments->filename, "r");
    if(file == NULL){
        fprintf(stderr, "Error opening config file: %s\n", strerror(errno));
        return -1;
    }

    while(fscanf(file, "%127[^=]=%127[^\n]%*c", field, value) == 2){
        stop = NULL;
        if(strcmp(field, "PORT") == 0){
            val = strtol(value, &stop, 10);
            if(stop == NULL || *stop != '\0'){
                fprintf(stderr, "Error interpreting port: %s\n", value);
                fclose(file);
                return -1;
            }
            arguments->listenPort = val;
        }

        if(strcmp(field, "BACKUP_PSK") == 0){
            arguments->psk = strdup(value);
        }

        bzero(field, BUFFER_SIZE);
        bzero(value, BUFFER_SIZE);
    }


    return 0;
}
