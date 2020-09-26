//
// Created by arch1t3ct on 9/26/20.
//

#include "sqlite3.h"
#include <stdio.h>
#include <unistd.h>
#include <crypt.h>
#include <string.h>

#define PASSWORD_LENGTH 32
#define HASH_LENGTH     264

int main(int argc, char *argv[]){
    char  hash[HASH_LENGTH];
    const char *const seedchars = "./0123456789"
                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz";
    unsigned long int seed[2];

    char salt[] = "$5$........";

    if(argc != 3){
        fprintf(stdout, "Usage: %s <username> <password>\n\n", argv[0]);
        return -1;
    }

    if(strlen(argv[1]) > PASSWORD_LENGTH || strlen(argv[2]) > PASSWORD_LENGTH){
        fprintf(stderr, "Username or password too long. Max 32 Characters\n");
        return -1;
    }

    FILE *f = fopen("/dev/random", "r");
    fread(&seed[0], sizeof(unsigned long int), 2, f);
    fclose(f);

    for (int i = 0; i < 8; i++)
        salt[3+i] = seedchars[(seed[i/5] >> (i%5)*6) & 0x3f];

    strncpy(hash, crypt(argv[2], salt), HASH_LENGTH);
    fprintf(stdout, "Computed hash: %s\n", hash);
    bzero(argv[2], PASSWORD_LENGTH);

    sqlite3* db ;
    int retCode = sqlite3_open("items.db", &db);
    if(retCode != SQLITE_OK){
        fprintf(stderr, "Could not open db: %d", sqlite3_errcode(db));
        return -1;
    }

    char* sql = "INSERT INTO users(username, password) VALUES (?, ?)";
    sqlite3_stmt *stmt;
    if((retCode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL)) != SQLITE_OK){
        fprintf(stderr, "Error preparing query\n");
        sqlite3_close(db);
        return -1;
    }

    if((retCode = sqlite3_bind_text(stmt, 1, argv[1], strlen(argv[1]), NULL)) != SQLITE_OK){
        fprintf(stderr, "Error preparing query\n");
        sqlite3_close(db);
        return -1;
    }

    if((retCode = sqlite3_bind_text(stmt, 2, hash, HASH_LENGTH, NULL)) != SQLITE_OK){
        fprintf(stderr, "Error preparing query\n");
        sqlite3_close(db);
        return -1;
    }

    retCode = sqlite3_step(stmt);
    if(retCode != SQLITE_OK && retCode != SQLITE_DONE){
        fprintf(stderr, "Error writing to database: %d", sqlite3_errcode(db));
        sqlite3_close(db);
        return -1;
    }

    fprintf(stdout, "Successfully added user %s to database\n", argv[1]);
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 0;

}