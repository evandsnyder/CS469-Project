#include <stdio.h>
#include <argp.h>
#include <stdlib.h>

#include "../globals.h"

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
    return 0;
}