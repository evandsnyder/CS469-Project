//
// Created by arch1t3ct on 10/6/20.
//

#include "globals.h"
#include <stdio.h>

void *malloc_aligned(unsigned int size){
    void *ptr;
    int r = posix_memalign(&ptr, 1024, size);
    if(r != 0){
        perror("Could not allocated memory in malloc_aligned");
        return NULL;
    }

    memset(ptr, 0, size);
    return ptr;
}

void freeItem(Item* item){
    free(item->description);
    free(item->name);
    free(item);
}

char* serialize_item(Item *item, char* result){
    asprintf(&result, "%d\n%s\n%d\n%d\n%d\n%d\n%d\n%f\n%d\n%s%c",
             item->id, // id
             item->name, // name
             item->armor, // armor
             item->health, // health
             item->mana, // mana
             item->sellPrice, // sellPrice
             item->damage, // damage
             item->critChance, // critical
             item->range, // range
             item->description,
             RECORD_SEPARATOR);
    return result;
}

void deserialize_item(char *buf, Item *item) {
    item->name = malloc(sizeof(char) * 256);
    item->description = malloc(sizeof(char)*256);

    sscanf(buf, "%d\n%[^\n]\n%d\n%d\n%d\n%d\n%d\n%lf\n%d\n%[^\x1e]%c",
           &item->id,
           item->name,
           &item->armor,
           &item->health,
           &item->mana,
           &item->sellPrice,
           &item->damage,
           &item->critChance,
           &item->range,
           item->description,
           NULL
    );
}
