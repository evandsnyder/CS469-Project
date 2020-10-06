#define _GNU_SOURCE

#include <stdio.h>
#include <argp.h>
#include <stdlib.h>
#include <pthread.h>
#include <sqlite3.h>
#include <fcntl.h>

#include "../globals.h"
#include "network.h"
#include "queue.h"

#define CLIENT_GET 1
#define CLIENT_PUT 2
#define CLIENT_MOD 3
#define CLIENT_DEL 4

void *handle_database_thread(void *data);
void *client_thread(void *data);
void *timer_thread_handler(void *data);
int db_login(sqlite3 *db, char *username, char *password);
int authenticate(const char *hash, char *password);
static error_t parse_args(int key, char *arg, struct argp_state *state);
int parse_conf_file(void *args);
int parse_interval(char *interval);
char * marshalItems(sqlite3_stmt *stmt);

struct Arguments {
    int listenPort;
    char *server;
    int backupPort;
    char *backupPsk;
    char *filename;
    char *database;
    int interval;
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
    char *backupServer;
    int backupPort;
    char *backupPsk;
} db_info;

typedef struct {
    struct queue_root* queue;
    int interval;
} timer_info;

static struct argp_option options[] = {
        {"listen-port",'l',"<port>", 0, "Port to listen on. Default: 4466"},
        {"backup-inventoryserver", 's', "<inventoryserver>", 0, "Server to backup to. Default: localhost"},
        {"backup-port", 'p', "<port>", 0, "Port of backup inventoryserver. Default: 6644"},
        {"backup-key", 'k', "<key>", 0, "Pre-shared key to authenticate to backup server."},
        {"config", 'c', "<filename>", 0, "A config file that can be used in lieu of CLI arguments. This will override all CLI arguments."},
        {"database", 'd', "<filename>", 0, "SQLite 3 database file to use for the application. Default: items.db"},
        {"backup-interval",'i',"<n:H>", 0, "How frequently to backup the database. The time format is time:unit. Acceptable units are [H]ours, [m]inutes, [s]econds. Default: 24:H"},
        {0}
};



struct argp argp = { options, parse_args, 0, "A program to manage remote database queries."};
int main(int argc, char *argv[]){

    struct Arguments arguments = {0};
    SSL_CTX *ssl_ctx;

    client_data clients[MAX_CLIENTS];
    pthread_t database_thread;
    pthread_t timer_thread;
    struct queue_root *db_queue;
    int err, i;

    arguments.listenPort = DEFAULT_SERVER_PORT;
    arguments.server = DEFAULT_SERVER;
    arguments.backupPort = DEFAULT_BACKUP_PORT;
    arguments.backupPsk = "";
    arguments.database = DEFAULT_DATABASE;
    arguments.interval = DEFAULT_INTERVAL;
    arguments.filename = NULL;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);
    if(arguments.filename != NULL) parse_conf_file(&arguments);

    printf("Hello from inventoryserver!\n");
    printf("Provided args:\n");
    printf("\tListen port: %d\n", arguments.listenPort);
    printf("\tServer: %s:%d\n", arguments.server, arguments.backupPort);
    printf("\tConfig file: %s\n", arguments.filename ? arguments.filename: "NULL");
    printf("\tBackup interval: %d seconds\n", arguments.interval);

    // Initializing global writer queue
    db_queue = ALLOC_QUEUE_ROOT();
    struct queue_head *sample_item = malloc_aligned(sizeof(struct queue_head));
    INIT_QUEUE_HEAD(sample_item, "INITIALIZATION", NULL);
    queue_put(sample_item, db_queue);

    db_info *info = (db_info*)malloc(sizeof(db_info));
    info->database = arguments.database;
    info->queue = db_queue;
    info->backupServer = arguments.server;
    info->backupPort = arguments.backupPort;
    info->backupPsk = arguments.backupPsk;

    // Need to spawn Database server
    err = pthread_create(&database_thread, NULL, handle_database_thread, (void *)info);
    if(err != 0){
        fprintf(stderr, "Server: Could not initialize database thread: %d\n", err);
        return -1;
    }

    timer_info *timer = (timer_info*)malloc(sizeof(timer_info));
    timer->interval = arguments.interval;
    timer->queue = db_queue;
    err = pthread_create(&timer_thread, NULL, timer_thread_handler, (void*)timer);
    if(err != 0){
        fprintf(stderr, "Server: Could not initialize timer thread: %d\n", err);
        return -1;
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

        client = accept((int)sockfd, (struct sockaddr*)&addr, &len);
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

/**
 * This thread handles all of the database operations. This includes, all standard CRUD operations
 * as well as the periodic synchronization with the datastore server.
 * @param data
 * @return
 */
void *handle_database_thread(void *data){
    db_info *info = (db_info*)data;
    struct queue_root *db_queue = info->queue;

    char request_data[BUFFER_SIZE];
    const char *selectItemsQuery = "SELECT * from items;";
    const char *insertItemQuery = "INSERT INTO items(name, description, armorPoints, healthPoints, manaPoints, sellPrice, damage, critChance, range) VALUES (?,?,?,?,?,?,?,?,?)";

    // Setup Database for operations
    char *valid_schema_query = "SELECT name FROM sqlite_master WHERE type='table' AND name='items' OR name='users'";
    sqlite3 *db = NULL;
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
    // SYNC, AUTH, and TERM are also available.
    int flag= 1;
    while(flag){
        struct queue_head *msg = queue_get(db_queue);
        struct queue_head *response = malloc(sizeof(struct queue_head));

        if(msg != NULL){
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

            if(strcmp("GET", msg->operation) == 0){
                fprintf(stdout, "Database: processing get\n");
                retCode = sqlite3_prepare_v2(db, selectItemsQuery, -1, &stmt, NULL);
                if(retCode != SQLITE_OK) {
                    fprintf(stderr, "SQL Error: %s\n", sqlite3_errmsg(db));
                    INIT_QUEUE_HEAD(response, "FAILURE", NULL);
                    continue;
                }
                INIT_QUEUE_HEAD(response, marshalItems(stmt), NULL);
                queue_put(response,msg->response_queue);
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

            if(strcmp(msg->operation, "SYNC") == 0){
                SSL_CTX * ssl_ctx = NULL;
                SSL * ssl = NULL;
                int backupSockFd;
                int dbFileFd = 0;
                char success = 1;

                fprintf(stdout, "Beginning synchronization!\n");

                // NOTE: this doesn't loop. We use it for an early-return on error
                while (1) {
                    // TODO: errno, etc

                    // Close database handle.
                    sqlite3_close(db);

                    ssl_ctx = create_new_client_context();
                    ssl = SSL_new(ssl_ctx);

                    // Open connection to remote server
                    backupSockFd = create_client_socket(info->backupServer, info->backupPort);
                    if (backupSockFd < 0) {
                        fprintf(stderr, "Error reopening database file for sync\n");
                        success = 0;
                        break;
                    }
                    SSL_set_fd(ssl, backupSockFd);
                    if (SSL_connect(ssl) != 1) {
                        fprintf(stderr, "Could not establish secure connection\n");
                        ERR_print_errors_fp(stderr);
                        success = 0;
                        break;
                    }

                    // Open as standard file,
                    dbFileFd = open(info->database, O_RDONLY);
                    if (dbFileFd < 0) {
                        fprintf(stderr, "Error reopening database file for sync\n");
                        success = 0;
                        break;
                    }

                    char buffer[BUFFER_SIZE];

                    // stream it to the server
                    sprintf(buffer, "REPLICATE %s\n", info->backupPsk);
                    SSL_write(ssl, buffer, strlen(buffer));

                    int rcount;
                    while ((rcount = read(dbFileFd, buffer, BUFFER_SIZE)) > 0) {
                        rcount = SSL_write(ssl, buffer, rcount);
                        if (rcount < 0) {
                            fprintf(stderr, "Error writing to socket\n");
                            success = 0;
                            break;
                        }
                    }
                    if (rcount < 0)
                        break;
                    SSL_shutdown(ssl);
                    // get success response back
                    rcount = 1;
                    while ((rcount = SSL_read(ssl, buffer, BUFFER_SIZE)) > 0)
                        ;

                    if (strncmp(buffer, "SUCCESS", strlen("SUCCESS")) != 0)
                        success = 0;

                    break;
                }
                
                // shut down connection to remote server
                if (ssl)
                    SSL_free(ssl);
                if (ssl_ctx)
                    SSL_CTX_free(ssl_ctx);
                if(backupSockFd)
                    close(backupSockFd);

                // close file
                if(dbFileFd)
                    close(dbFileFd);

                // reopen as database again.
                retCode = sqlite3_open(info->database, &db);
                if(retCode != SQLITE_OK){
                    fprintf(stderr, "Error opening database after sync\n");
                    success = 0;
                }

                if(success)
                    INIT_QUEUE_HEAD(response, "SUCCESS", NULL);
                else
                    INIT_QUEUE_HEAD(response, "FAILURE", NULL);
            }

            // Response here
            if(msg->response_queue != NULL)
                queue_put(response, msg->response_queue);
            // msg needs to be freed and response should be de-referenced
            free_queue_message(msg);
            response = NULL;
        }

        usleep(10000); // Wait 10 ms between reads
    }
}

/**
 * Each client gets its own thread to recieve network requests from.
 * Requests are relayed to the database thread through a thread safe queue.
 *
 * @param data
 * @return
 */
void *client_thread(void *data){
    char buffer[BUFFER_SIZE];
    client_data *client_info = (client_data*)data;
    int opFlag = 0;

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
    query = NULL; // Remove reference to it

    // Check the response queue until a response is received;
    struct queue_head *response = malloc_aligned(sizeof(struct queue_head));
    do{
        response = queue_get(msgQueue);
        usleep(10000);
    }while(response == NULL);

    fprintf(stdout, "CLIENT_THREAD_%d Message Received: %s\n", socketfd, response->operation);

    bzero(buffer, BUFFER_SIZE);
    strncpy(buffer, response->operation, BUFFER_SIZE);
    int validLogin = 1;
    if(strcmp("FAILURE", response->operation) == 0){
        validLogin = 0;
    }
    SSL_write(ssl, buffer, (int)strlen(buffer));

    while(validLogin){

        bzero(buffer, BUFFER_SIZE);
        if(SSL_read(ssl, buffer, BUFFER_SIZE) < 0) {
            fprintf(stderr, "Error reading from client: %s\n", strerror(errno));
            validLogin = 0;
            continue;
        }
        if(strlen(buffer) == 0){
            continue;
        }

        fprintf(stdout, "Message received: %s\n", buffer);

        query = (struct queue_head*)malloc(sizeof(struct queue_head));
        INIT_QUEUE_HEAD(query, buffer, msgQueue);
        // fprintf(stdout, "%s\n", query->operation);
        queue_put(query, client_info->queue);

        if(strncmp("GET", buffer, strlen(buffer)) == 0){
            opFlag = CLIENT_GET;
        }

        response = NULL;
        do{
            response = queue_get(msgQueue);
            usleep(10000);
        } while( response == NULL);

        switch(opFlag){
            case CLIENT_GET:
                if((rcount = SSL_write(ssl, response->operation, (int)strlen(response->operation))) < 0){
                    fprintf(stderr, "Error writing to client: %s\n", strerror(errno));
                    validLogin = 0;
                    continue;
                }
                fprintf(stdout,"wrote %d, bytes\n", rcount);

                break;
            case CLIENT_PUT:
            case CLIENT_MOD:
            case CLIENT_DEL:
            default:
                fprintf(stderr, "No supported yet\n");
                break;
        }

        // Free the response and get ready for the next one
        free_queue_message(response);
    }

    SSL_free(ssl);
    close(client_info->socketfd);
    client_info->open = 1;
    pthread_exit(NULL);
}

/**
 * This method is a rudimentary implentation of a backup timer.
 * It sleeps for a time defined by the user, and then sends a message
 * to the database thread to conduct the backup operation.
 *
 * @param data timer info.
 * @return NULL
 */
void* timer_thread_handler(void *data){
    fprintf(stdout, "Initializing Backup thread\n");
    timer_info *info = (timer_info*)data;

    struct queue_head *sync_message = malloc_aligned(sizeof(struct queue_head));

    while(1){
        sleep(info->interval);
        free(sync_message);
        sync_message= malloc_aligned(sizeof(struct queue_head));
        if(sync_message == NULL){
            fprintf(stderr, "Could not create synchronization message");
            exit(-1);
        }
        INIT_QUEUE_HEAD(sync_message, "SYNC", NULL);
        queue_put(sync_message, info->queue);
    }
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

    char *hash = malloc(sizeof(char)*BUFFER_SIZE);
    strncpy(hash, (char*)sqlite3_column_text(stmt, 0), BUFFER_SIZE);
    sqlite3_finalize(stmt);
    int r = authenticate(hash, password);

    free(hash);

    return r;
}

/**
 * Method that takes hash from database and password from user to
 * Determine if there is a valid login.
 *
 * @param hash
 * @param password
 * @return
 */
int authenticate(const char *hash, char *password){
    char salt[SALT_LENGTH];
    strncpy(salt, hash, SALT_LENGTH);
    salt[11] = '\0';

    return strncmp(hash, crypt(password, salt), BUFFER_SIZE);
}

static error_t parse_args(int key, char *arg, struct argp_state *state){
    struct Arguments *arguments = state->input;
    char *pEnd;
    int interval;


    switch(key){
        case 'l':
            arguments->listenPort = (int)strtol(arg, &pEnd,10);
            break;
        case 's':
            arguments->server = arg;
            break;
        case 'p':
            arguments->backupPort = (int)strtol(arg, &pEnd,10);
            break;
        case 'k':
            arguments->backupPsk = arg;
            break;
        case 'c':
            arguments->filename = arg;
            break;
        case 'd':
            arguments->database = arg;
            break;
        case 'i':
            interval = parse_interval(arg);
            if(interval <= 0){
                fprintf(stderr, "Invalid Timer value %s\n", arg);
            }
            arguments->interval = interval;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

/**
 * parse_conf_file reads a user defined file to pull program paramters
 *
 * @param args arguments struct
 * @return
 */
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

        if(strcmp(field, "BACKUP_SERVER") == 0){
            arguments->server = strdup(value);
        }

        if(strcmp(field, "BACKUP_PORT") == 0){
            val = strtol(value, &stop, 10);
            if(stop == NULL || *stop != '\0'){
                fprintf(stderr, "Error interpreting Backup server port: %s\n", value);
                fclose(file);
                return -1;
            }
            arguments->backupPort = val;
        }

        if(strcmp(field, "BACKUP_PSK") == 0){
            arguments->backupPsk = strdup(value);
        }

        if(strcmp(field, "DATABASE") == 0){
            arguments->database = strdup(value);
        }

        if(strcmp(field, "INTERVAL") == 0){
            val = parse_interval(value);
            if(val < 0){
                fprintf(stderr, "Invalid time interval: %s\n", value);
                fclose(file);
                return -1;
            }
            arguments->interval = val;
        }

        bzero(field, BUFFER_SIZE);
        bzero(value, BUFFER_SIZE);
    }


    return 0;
}

/**
 * Method to interpret the interval period format
 * @param interval
 * @return
 */
int parse_interval(char* interval){
    // Split on the ':'
    char *stop;
    char *token;
    int num;
    int mult;

    token = strtok(interval, ":");
    num = strtol(token, &stop, 10);
    if(stop == NULL){
        return -1; // Invalid Number
    }
    token = strtok(NULL, ":");
    if(strlen(token) != 1){
        return -1;
    }
    switch(token[0]){
        // Hours
        case 'H':
        case 'h':
            mult = 60*60;
            break;
        // Minutes
        case 'M':
        case 'm':
            mult = 60;
            break;
        // Seconds
        case 'S':
        case 's':
            mult = 1;
            break;
        default:
            return -1;
    }


    return num * mult;
}


char * marshalItems(sqlite3_stmt *stmt){
    size_t cur_size =1024*4;
    size_t mem_size = 1024*4;
    char* result = (char*)malloc(sizeof(char)*mem_size); // Allocate 4Kb
    bzero(result, sizeof(char)*mem_size);
    strncat(result, "DATA ", 5);

    int r = SQLITE_OK;
    r = sqlite3_step(stmt);
    while(r == SQLITE_ROW ){
        char* curItem;
        asprintf(&curItem, "%d%c%s%c%d%c%d%c%d%c%d%c%d%c%f%c%d%c%s%c",
                 sqlite3_column_int(stmt, 0), // id
                 UNIT_SEPARATOR,
                 (const char*)sqlite3_column_text(stmt, 1), // name
                 UNIT_SEPARATOR,
                 sqlite3_column_int(stmt, 2), // armor
                 UNIT_SEPARATOR,
                 sqlite3_column_int(stmt, 3), // health
                 UNIT_SEPARATOR,
                 sqlite3_column_int(stmt, 4), // mana
                 UNIT_SEPARATOR,
                 sqlite3_column_int(stmt, 5), // sellPrice
                 UNIT_SEPARATOR,
                 sqlite3_column_int(stmt, 6), // damage
                 UNIT_SEPARATOR,
                 sqlite3_column_double(stmt, 7), // critical
                 UNIT_SEPARATOR,
                 sqlite3_column_int(stmt, 8), //range
                 UNIT_SEPARATOR,
                 (const char*)sqlite3_column_text(stmt, 9),
                 RECORD_SEPARATOR
         );

        // Need to append
        if(strlen(curItem) + strlen(result) >= cur_size){ // Amount of data is too large, we need to add more memory
            cur_size += mem_size;
            result = realloc(result, cur_size);
        }
        strncat(result, curItem, strlen(curItem));
        free(curItem);
        r = sqlite3_step(stmt);
    }

    // Replace last character with group separator
    // -1 should be \x0, -2 should be RECORD_SEPARATOR
    result[strlen(result)-2] = GROUP_SEPARATOR;

    return result;
}