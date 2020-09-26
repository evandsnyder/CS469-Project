#include <stdio.h>
#include <argp.h>
#include <stdlib.h>
#include <pthread.h>
#include <sqlite3.h>
#include <crypt.h>

#include "../globals.h"
#include "network.h"
#include "queue.h"

void *handle_database_thread(void *data);
void *client_thread(void *data);
int db_login(sqlite3 *db, char *username, char *password);
void hash_password(char* hash, char* password);
int authenticate(const char *hash, char *password);

struct Arguments {
    int listenPort;
    char *server;
    int backupPort;
    char *filename;
    char *database;
};

typedef struct {
    pthread_t thread_id;
    int socketfd;
    int open;
    SSL_CTX* ctx;
    struct queue_root* queue;
} client_data;

typedef struct {
    struct queue_root* queue;
    char *database;
} db_info;

static struct argp_option options[] = {
        {"listen-port",'l',"<port>", 0, "Port to listen on. Default: 4466"},
        {"backup-inventoryserver", 's', "<inventoryserver>", 0, "Server to backup to. Default: localhost"},
        {"backup-port", 'p', "<port", 0, "Port of backup inventoryserver. Default: 6644"},
        {"config", 'c', "<filename>", 0, "A config file that can be used in lieu of CLI arguments. This will override all CLI arguments."},
        {"database", 'd', "<filename>", 0, "SQLite 3 database file to use for the application. Default: items.db"},
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
        case 'c':
            arguments->filename = arg;
            break;
        case 'd':
            arguments->database = arg;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

struct argp argp = { options, parse_args, 0, "A program to manage remote database queries."};
int main(int argc, char *argv[]){

    struct Arguments arguments = {0};
    SSL_CTX *ssl_ctx;

    client_data clients[MAX_CLIENTS];
    // client_data *clients = (client_data*)malloc(sizeof(client_data)*MAX_CLIENTS);
    pthread_t database_thread;
    struct queue_root *db_queue;
    int err, i;

    arguments.listenPort = DEFAULT_SERVER_PORT;
    arguments.server = DEFAULT_SERVER;
    arguments.backupPort = DEFAULT_BACKUP_PORT;
    arguments.database = DEFAULT_DATABASE;

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

    db_info *info = (db_info*)malloc(sizeof(db_info));
    info->database = arguments.database;
    info->queue = db_queue;

    // Need to spawn Database server
    err = pthread_create(&database_thread, NULL, handle_database_thread, (void *)info);
    if(err != 0){
        fprintf(stderr, "Server: Could not initialize database thread: %d\n", err);
    }

    init_openssl();
    // init_locks();
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

        clients[i].socketfd = client;
        clients[i].queue = db_queue;
        err = pthread_create(&clients[i].thread_id, NULL, client_thread, (void*)&(clients[i]));
        pthread_detach(clients[i].thread_id);
    }

    SSL_CTX_free(ssl_ctx);
    cleanup_openssl();
    close(sockfd);

    return 0;
}

void *handle_database_thread(void *data){
    db_info *info = (db_info*)data;
    struct queue_root *db_queue = info->queue;

    char request_data[BUFFER_SIZE];

    // Setup Database for operations
    char *valid_schema_query = "SELECT name FROM sqlite_master WHERE type='table' AND name='items' OR name='users'";
    sqlite3 *db = NULL;
    char *errMsg = 0;
    char *sql;
    int retCode;
    sqlite3_stmt *stmt = NULL;

    retCode = sqlite3_open(info->database, &db);
    if(retCode != SQLITE_OK){
        fprintf(stderr, "FATAL: Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(-1);
    }
    fprintf(stdout, "Server: Database opened!\n");

    retCode = sqlite3_prepare(db, valid_schema_query, -1, &stmt, 0);
    if(retCode != SQLITE_OK || stmt == NULL){
        fprintf(stderr, "Database: Invalid database schema. Missing user or items table");
        sqlite3_close(db);
        exit(-1);
    }

    // Check that the database has the correct schema. Doesn't check column names
    retCode = sqlite3_step(stmt);
    while(retCode != SQLITE_DONE){
        const char *table = sqlite3_column_text(stmt, 0);
        if(strcmp(table, "items") != 0 && strcmp(table, "users") != 0){
            fprintf(stderr, "Database: Invalid schema. Missing user or items table");
            sqlite3_close(db);
            exit(-1);
        }
        retCode = sqlite3_step(stmt);
    }

    // Read the database every 10ms, operate if actions

    // Operations will be GET, PUT, DEL, and MOD[ify]
    //
    int flag= 1;
    while(flag){
        struct queue_head *msg = queue_get(db_queue);
        struct queue_head *response = malloc(sizeof(struct queue_head));

        if(msg != NULL){
            if(msg->response_queue == NULL) continue;
            char username[BUFFER_SIZE];
            char password[BUFFER_SIZE];
            if(sscanf(msg->operation, "AUTH %s %s", username, password) == 2){
                fprintf(stdout, "DB_THREAD: Authenticating user\n");
                retCode = db_login(db, username, password);
                if(retCode != 0)
                    INIT_QUEUE_HEAD(response, "FAILURE", NULL);
                else
                    INIT_QUEUE_HEAD(response, "SUCCESS", NULL);
            }

            if(sscanf(msg->operation, "GET %s", request_data) == 1){
                // GET all items
            }

            if(sscanf(msg->operation, "PUT %s", request_data) == 1){
                // Insert new item
            }

            if(sscanf(msg->operation, "MOD %s", request_data) == 1){
                // Modify existing item
            }

            if(sscanf(msg->operation, "DEL %s", request_data) == 1){
                // Delete existing
            }

            if(sscanf(msg->operation, "TERM %s", request_data) == 1){
                flag = 0;
            }

            // Response here

            queue_put(response, msg->response_queue);
        }

        usleep(10000);
    }
}
//}

void *client_thread(void *data){
    char buffer[BUFFER_SIZE];
    client_data *client_info = (client_data*)data;

    SSL *ssl = SSL_new(client_info->ctx);
    if(ssl == NULL){
        fprintf(stderr, "Error creating new SSL\n");
    }
    int socketfd = client_info->socketfd;

    if(SSL_set_fd(ssl, socketfd) < 0){
        fprintf(stderr, "Could not bind to secure socket: %s\n", strerror(errno));
        pthread_exit(NULL);
    }

    if(SSL_accept(ssl) <= 0){
        fprintf(stderr, "Server: Could not establish a secure connection:\n");
        ERR_print_errors_fp(stderr);
        pthread_exit(NULL);
    }

    // Prep the message and return queue
    struct queue_root *msgQueue = ALLOC_QUEUE_ROOT();
    struct queue_head *query = malloc_aligned(sizeof(struct queue_head));

    bzero(buffer, BUFFER_SIZE);
    // Listen for AUTH request
    int rcount = SSL_read(ssl, buffer, BUFFER_SIZE);
    if(rcount < 0){
        fprintf(stderr, "Could not read from client: %s\n", strerror(errno));
    }

    // But for now, lets just send it to the database thread for some PoC
    INIT_QUEUE_HEAD(query, buffer, msgQueue);
    queue_put(query, client_info->queue);

    // Check the response queue until a response is received;
    struct queue_head *response = malloc_aligned(sizeof(struct queue_head));
    do{
        response = queue_get(msgQueue);
        usleep(10000);
    }while(response == NULL);

    fprintf(stdout, "CLIENT_THREAD_%d Message Received: %s\n", socketfd, response->operation);

    SSL_free(ssl);
    close(client_info->socketfd);
    client_info->open = 1;
    pthread_exit(NULL);
}

int db_login(sqlite3 *db, char *username, char *password) {
    const char *sql = "SELECT password FROM users where username=? LIMIT 1;";
    sqlite3_stmt *stmt;
    int retCode;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, username, strlen(username), NULL);

    retCode = sqlite3_step(stmt);
    if(retCode == SQLITE_DONE){
        // No Such user exists
        return -1;
    }

    const char *hash = sqlite3_column_text(stmt, 0);
    sqlite3_finalize(stmt);

    fprintf(stdout, "HASH: %s\n", hash);
    return authenticate(hash, password);
}

void hash_password(char *hash, char* password){
    char salt[] = "$5$........";
    unsigned long int seed[2];
    const char *const seedchars = "./0123456789"
                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz";

    FILE *f = fopen("/dev/random", "r");
    fread(&seed[0], sizeof(unsigned long int), 2, f);
    fclose(f);

    for(int i = 0; i < 8; i++){
        salt[3+i] = seedchars[(seed[i/5] >> (i%5)*6) & 0x3f ];
    }

    strncpy(hash, crypt(password, salt), BUFFER_SIZE);
}

int authenticate(const char *hash, char *password){
    char salt[SALT_LENGTH];
    strncpy(salt, hash, SALT_LENGTH);
    salt[11] = '\0';

    fprintf(stdout, "SALT %s\n", salt);

    fprintf(stdout, "Comparing %s to %s\n", password, hash);
    fprintf(stdout, "with hash: %s\n", crypt(password, salt));

    return strncmp(hash, crypt(password, salt), BUFFER_SIZE);
}