

#ifndef GLOBALS_H
#define GLOBALS_H

#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_ITEMS 1024
#define DEFAULT_SERVER_PORT 4466
#define DEFAULT_BACKUP_PORT 6644
#define DEFAULT_SERVER "localhost"
#define DEFAULT_DATABASE "items.db"
#define DEFAULT_INTERVAL 24*60*60
#define BUFFER_SIZE 256
#define MAX_CLIENTS 512
#define SALT_LENGTH 11

#define GROUP_SEPARATOR 0x1d
#define RECORD_SEPARATOR 0x1e
#define UNIT_SEPARATOR 0x1f

#define CLIENT_GET 1
#define CLIENT_PUT 2
#define CLIENT_MOD 3
#define CLIENT_DEL 4

typedef struct {
    int id;
    char name[BUFFER_SIZE];
    int armor;
    int health;
    int mana;
    int sellPrice;
    int damage;
    double critChance;
    int range;
    char description[BUFFER_SIZE];
} Item;

void *malloc_aligned(unsigned int size);

void freeItem(Item* item);
char* serialize_item(Item* item, char* result);
void deserialize_item(char *buf, Item *item);

#endif