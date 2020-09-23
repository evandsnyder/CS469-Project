#include <stdio.h>
#include <argp.h>
#include <stdlib.h>

#include "../globals.h"

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
    return 0;
}