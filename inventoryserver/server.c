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

typedef struct {
    pthread_t thread_id;
    int socketfd;
    int open;
    SSL_CTX* ctx;
    struct queue_root* queue;
} client_data;

static struct argp_option options[] = {
        {"listen-port",'l',"<port>", 0, "Port to listen on. Default: 4466"},
        {"backup-inventoryserver", 's', "<inventoryserver>", 0, "Server to backup to. Default: localhost"},
        {"backup-port", 'p', "<port", 0, "Port of backup inventoryserver. Default: 6644"},
        {"config-file", 'f', "<filename>", 0, "A config file that can be used in lieu of CLI arguments. This will override all CLI arguments."},
        {"backup-interval",'i',"<n:H>", 0, "How frequently to backup the database. The time format is time:unit. Acceptable units are [H]ours, [m]inutes, [s]econds. Default: 24:H"},
        {0}
};

static void *malloc_aligned(unsigned int size){
    void *ptr;
    int r = posix_memalign(&ptr, 1024, size);
    if(r != 0){
        perror("Could not allocated memory in malloc_aligned");
        return NULL;
    }

    memset(ptr, 0, size);
    return ptr;
}

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
    SSL_CTX *ssl_ctx;

    client_data clients[MAX_CLIENTS];
    pthread_t database_thread;
    struct queue_root *db_queue;
    int err, i;

    arguments.listenPort = DEFAULT_SERVER_PORT;
    arguments.server = DEFAULT_SERVER;
    arguments.backupPort = DEFAULT_BACKUP_PORT;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    printf("Hello from inventoryserver!\n");
    printf("Provided args:\n");
    printf("\tListen port: %d\n", arguments.listenPort);
    printf("\tServer: %s:%d\n", arguments.server, arguments.backupPort);
    printf("\tConfig file: %s\n", arguments.filename ? arguments.filename: "NULL");

    // Initializing global writer queue
    db_queue = ALLOC_QUEUE_ROOT();
    struct queue_head *sample_item = malloc_aligned(sizeof(struct queue_head));
    INIT_QUEUE_HEAD(sample_item, "INITIALIZATION", NULL);
    queue_put(sample_item, db_queue);

    // Need to spawn Database server
    err = pthread_create(&database_thread, NULL, handle_database_thread, (void *)db_queue);
    if(err != 0){
        fprintf(stderr, "Server: Could not initialize database thread: %d\n", err);
    }

    init_locks();
    init_openssl();
    ssl_ctx = create_new_context();
    configure_context(ssl_ctx);

    // Initialize clients
    for(i = 0;i < MAX_CLIENTS; i++){
        clients[i].open = 1;
        clients[i].ctx = ssl_ctx;
        clients[i].queue = db_queue;
    }

    unsigned int sockfd;

    sockfd = create_socket(arguments.listenPort);
    if(sockfd < 0){
        fprintf(stderr, "Could not create socket\n");
        exit(-1);
    }
    fprintf(stdout, "Server: Listening for network connections!\n");
    while(true){
        int client;
        struct sockaddr_in addr;
        unsigned int len = sizeof(addr);
        // char client_addr[INET_ADDRSTRLEN];

        client = accept(sockfd, (struct sockaddr*)&addr, &len);
        if(client < 0){
            fprintf(stderr, "Could not accept client\n");
            break;
        }

        // Find first open
        // Spawn new Thread with client here
        for(i =0; i < MAX_CLIENTS && !clients[i].open; ++i);
        if(i == MAX_CLIENTS) continue; // Already at max clients

        client_data *data = (client_data*)malloc(sizeof(client_data));
        data->socketfd = client;
        data->queue = db_queue;
        err = pthread_create(&clients[i].thread_id, NULL, client_thread, &clients[i]);
        pthread_detach(clients[i].thread_id);
        break;
    }

    SSL_CTX_free(ssl_ctx);
    cleanup_openssl();
    close(sockfd);

    return 0;
}

void *handle_database_thread(void *request_queue){
    struct queue_root *db_queue = (struct queue_root*)request_queue;

    // Read the database every 10ms, operate if actions
    int flag= 1;
    while(flag){
        flag = 0;
        struct queue_head *msg = queue_get(request_queue);
        fprintf(stdout, "Message received: %s\n", msg->operation);
        usleep(10000);
    }

    fprintf(stdout, "Server: Handling Database operations!\n");
}

void *client_thread(void *data){
    char buffer[BUFFER_SIZE];
    client_data *client_info = (client_data*)data;
    SSL* ssl = SSL_new(client_info->ctx);
    SSL_set_fd(ssl, client_info->socketfd );
    if(SSL_accept(ssl) <= 0){
        fprintf(stderr, "Server: Could not establish a secure connection:\n");
        ERR_print_errors_fp(stderr);
    }

    bzero(buffer, BUFFER_SIZE);
    // Listen for AUTH request
    int rcount = SSL_read(ssl, buffer, BUFFER_SIZE);
    if(rcount < 0){
        fprintf(stderr, "Could not read from client: %s\n", strerror(errno));
    }

    fprintf("Message Received: %s\n", buffer);

    SSL_free(ssl);
    close(client_info->socketfd);
    client_info->open = 1;
    pthread_exit(NULL);
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