

#ifndef GLOBALS_H
#define GLOBALS_H

#include <string.h>
#include <stdlib.h>

#define MAX_ITEMS 128
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

typedef struct {
    int id;
    char *name;
    int armor;
    int health;
    int mana;
    int sellPrice;
    int damage;
    double critChance;
    int range;
    char *description;
} Item;

void *malloc_aligned(unsigned int size);

typedef struct ll_node_tag {
    Item* data;
    struct ll_node_tag* next;
} ll_node;

typedef struct ll_tag{
    ll_node *head;
    ll_node *tail;
} linked_list;

Item *allocItem(int id, const char* name, int armor, int health, int mana, int sellPrice, int dmg, double critChance, int range, const char* desc);
void freeItem(Item* item);

void ll_append(linked_list *ll, ll_node* node);
ll_node *create_node(Item *i);
linked_list * create_ll();
void freeLinkedList(linked_list* ll);

#endif

// IGNORE THIS STUFF FOR NOW. JUST Leftovers from the SQL Sample. WILL CLEANUP LATER

/*
static int callback(void *ignore, int argc, char ** argv, char **azColName){
    return 0;
}

void append(Item *items){

}

char *strdup(const char *s){
    size_t len = strlen(s) + 1;
    char *r = malloc(len);
    if(!r) return NULL;
    return memcpy(r, s, len);
}

Item *allocItem(int id, const char* name, const char *desc, int armor, int health, int dmg, double critChance){
    Item *item = malloc(sizeof(Item));
    if(item == NULL)
        return NULL;

    item->id = id;
    item->name = strdup(name);
    if(!item->name){
        free(item);
        return NULL;
    }

    item->description = strdup(desc);
    if(!item->description){
        free(item);
        return NULL;
    }

    item->armor = armor;
    item->health = health;
    item->damage = dmg;
    item->critChance = critChance;

    return item;
}

int main(int argc, char* argv[]){
    sqlite3 *db;
    char *errMsg = 0;
    char *sql;

    int retCode;
    // This will create or open the database
    retCode = sqlite3_open("test.db", &db);
    if(retCode){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return -1;
    } else {
        fprintf(stdout, "Database opened successfully\n");
    }

    // Create the table:
    sql = "DROP TABLE IF EXISTS items;" \
            "CREATE TABLE IF NOT EXISTS items (" \
            "id INTEGER PRIMARY KEY AUTOINCREMENT," \
            "name TEXT NOT NULL," \
            "description TEXT NOT NULL," \
            "armor INT," \
            "health INT," \
            "damage INT," \
            "critChance REAL" \
            ");";

    retCode = sqlite3_exec(db, sql, callback, 0, &errMsg);
    if(retCode != SQLITE_OK){
        fprintf(stderr, "SQL Error: %s\n", errMsg);
        sqlite3_free(errMsg);
    }else {
        fprintf(stdout, "Table successfully created\n");
    }

    // Insert into the table
    sql = "INSERT INTO items (name, description, armor) VALUES ('Iron Helmet', 'A Sturdy iron helmet for protection', 10);" \
    "INSERT INTO items (name, description, damage, critChance) VALUES ('Steel Longsword', 'A simple, reliable sword', 25, 0.65);"
    ;

    retCode = sqlite3_exec(db, sql, callback, 0, errMsg);
    if(retCode != SQLITE_OK){
        fprintf(stderr, "SQL Error: %s\n", errMsg);
        sqlite3_free(errMsg);
    }else {
        fprintf(stdout, "Table Updated successfully\n");
    }

    // Select
    sql = "SELECT * FROM items;";

    sqlite3_stmt *stmt;
    retCode = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if(retCode != SQLITE_OK){
        fprintf(stderr, "SQL Error: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    Item items[MAX_ITEMS];
    int open = 0;
    while((retCode = sqlite3_step(stmt)) == SQLITE_ROW){
        int id = sqlite3_column_int(stmt, 0);
        const char *name = sqlite3_column_text(stmt, 1);
        const char *desc = sqlite3_column_text(stmt, 2);
        int armor = sqlite3_column_int(stmt, 3);
        int health = sqlite3_column_int(stmt, 4);
        int damage = sqlite3_column_int(stmt, 5);
        double critChance = sqlite3_column_double(stmt, 6);

        Item *i = allocItem(id, name, desc, armor, health, damage, critChance);
        if(!i){
            fprintf(stderr, "Error creating Item\n");
            break;
        }

        items[open] = *i;
        open++;
    }

    fprintf(stdout, "Loaded Items:\n");
    for(int i =0; i < open; i++){
        fprintf(stdout, "\t%d: {%s, %s, %d, %d, %d, %.02f}\n", items[i].id, items[i].name, items[i].description,items[i].armor, items[i].health, items[i].damage, items[i].critChance);
    }

    sqlite3_close(db);
    return 0;
}
*/