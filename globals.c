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


Item *allocItem(int id, const char* name, int armor, int health, int mana, int sellPrice, int dmg, double critChance, int range, const char* desc){
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
    item->mana = mana;
    item->sellPrice = sellPrice;
    item->range = range;

    return item;
}

void freeItem(Item* item){
    free(item->description);
    free(item->name);
    free(item);
}

void ll_append(linked_list *ll, ll_node* node){
    node->next = NULL;
    if(ll->head == NULL) {
        ll->head = node;
        ll->tail = node;
    }
    ll->tail->next=node;
    ll->tail = node;
}

ll_node *create_node(Item *i){
    ll_node* n = (ll_node*)malloc(sizeof(ll_node));
    n->data = i;
    n->next = NULL;
    return n;
}

linked_list * create_ll(){
    linked_list *ll = (linked_list*)malloc(sizeof(linked_list));
    ll->head = NULL;
    ll->tail = NULL;
    return ll;
}

void freeLinkedList(linked_list* ll){
    ll_node *cur = ll->head;
    while(cur != NULL){
        ll_node *next = cur->next;
        freeItem(cur->data);
        free(cur);
        cur = next;
    }
}