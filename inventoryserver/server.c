#include <stdio.h>
#include <argp.h>
#include <stdlib.h>
#include <pthread.h>

#include "../globals.h"
#include "network.h"
#include "queue.h"

#ifdef USE_OPENSSL
#include <openssl/crypto.h>

static void lock_callback(int mode, int type, char *file, int line);
static unsigned long thread_id(void);
static void init_locks(void);
static void kill_locks();

#endif

struct Arguments {
    int listenPort;
    char *server;
    int backupPort;
    char *filename;
};

struct thread_data {
    int socketfd;
    struct queue_root* queue;
};

static struct argp_option options[] = {
        {"listen-port",'l',"<port>", 0, "Port to listen on. Default: 4466"},
        {"backup-inventoryserver", 's', "<inventoryserver>", 0, "Server to backup to. Default: localhost"},
        {"backup-port", 'p', "<port", 0, "Port of backup inventoryserver. Default: 6644"},
        {"config-file", 'f', "<filename>", 0, "A config file that can be used in lieu of CLI arguments. This will override all CLI arguments."},
        {"backup-interval",'i',"<n:H>", 0, "How frequently to backup the database. The time format is time:unit. Acceptable units are [H]ours, [m]inutes, [s]econds. Default: 24:H"},
        {0}
};

typedef struct {
    int sockfd;
    void* request_queue;
} client_data;

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

void *handle_database_thread(void *request_queue);
void *client_thread(void *data);


struct argp argp = { options, parse_args, 0, "A program to manage remote database queries."};
int main(int argc, char *argv[]){

    struct Arguments arguments = {0};

    pthread_t thread_ids[MAX_CLIENTS];
    pthread_t database_thread;
    struct queue_root *db_queue;
    int err;

    arguments.listenPort = DEFAULT_SERVER_PORT;
    arguments.server = DEFAULT_SERVER;
    arguments.backupPort = DEFAULT_BACKUP_PORT;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    printf("Hello from inventoryserver!\n");
    printf("Provided args:\n");
    printf("\tListen port: %d\n", arguments.listenPort);
    printf("\tServer: %s:%d\n", arguments.server, arguments.backupPort);
    printf("\tConfig file: %s\n", arguments.filename ? arguments.filename: "NULL");

    init_locks();
    memset(thread_ids, 0, MAX_CLIENTS);

    // Initializing global writer queue

    // Need to spawn Database server
    err = pthread_create(&database_thread, NULL, handle_database_thread, (void *)db_queue);
    if(err != 0){
        fprintf(stderr, "Server: Could not initialize database thread: %d\n", err);
    }

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
    fprintf(stdout, "Server: Listening for network connections!\n");
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

        struct thread_data *data;
        data->socketfd = client;
        data->queue = db_queue;
        // Find first open
        // Spawn new Thread with client here
        err = pthread_create(OPEN_SPACE, NULL, client_thread, (void *)data);

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

void *handle_database_thread(void *request_queue){
    struct queue_root *db_queue = (struct queue_root*)request_queue;

    fprintf(stdout, "Server: Handling Database operations!\n");
}

void *client_thread(void *data){
    fprintf(stdout, "Client connected!\n");
}

static void lock_callback(int mode, int type, char *file, int line){
    (void)file;
    (void)line;
    if(mode & CRYPTO_LOCK)
        pthread_mutex_lock(&(lockarray[type]));
    else
        pthread_mutex_unlock(&(lockarray[type]));
}

static unsigned long thread_id(void){
    unsigned long ret;

    ret = (unsigned long)pthread_self();
    return ret;
}

static void init_locks(void){
    int i;
    lockarray = (pthread_mutex_t *)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));

    for(i = 0; i < CRYPTO_num_locks(); i++)
        pthread_mutex_init(&(lockarray[i]), NULL);

    CRYPTO_set_id_callback((unsigned long (*)())thread_id);
    CRYPTO_set_locking_callback((void (*)())lock_callback);
}

static void kill_locks(){
    int i;
    CRYPTO_set_locking_callback(NULL);
    for(i = 0; i < CRYPTO_num_locks(); i++){
        pthread_mutex_destroy(&(lockarray[i]));
    }

    OPENSSL_free(lockarray);
}