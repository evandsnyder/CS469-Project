//
// Created by arch1t3ct on 10/1/20.
//

#include "main_window.h"
#include "network.h"

GtkWidget* createButton;
GtkWidget* modifyButton;
GtkWidget* deleteButton;
GtkWidget* mainWindow;
GtkWidget* listBox;
GtkListStore* itemListStore;

enum ItemInfo {
    ID = 0,
    NAME,
    ARMOR,
    HEALTH,
    MANA,
    SELL_PRICE,
    DAMAGE,
    CRIT_CHANCE,
    RANGE,
    DESCRIPTION
};

G_MODULE_EXPORT void editItemDialog(){
    // Will be used for editing and creating both hopefully
}

void display_error_dialog(char* msg){
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(mainWindow), GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Error %s", msg);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

Item * get_all_items_from_database(){
    char buffer[BUFFER_SIZE] = {0};
    sprintf(buffer, "GET ALL");
    SSL_write(ssl, buffer, strlen(buffer));

    bzero(buffer, BUFFER_SIZE);
    SSL_read(ssl, buffer, BUFFER_SIZE);
    if(strcmp("FAILURE", buffer) == 0){
        display_error_dialog("Could not Retrieve items from database");
        return NULL;
    }

    size_t mem_size = 1024*4;
    size_t cur_size = 1024*4;
    char* allItems = (char*)malloc(sizeof(char)*mem_size);
    allItems[0] = '\0';
    do{
        if(strlen(buffer) + strlen(allItems) >= cur_size){
            cur_size += mem_size;
            allItems = realloc(allItems, cur_size);
        }
        strncat(allItems, buffer, strlen(buffer));
        // process
        bzero(buffer, BUFFER_SIZE);
        SSL_read(ssl, buffer, BUFFER_SIZE);
    } while(buffer[strlen(buffer)-1] != GROUP_SEPARATOR);

    fprintf(stdout, "RECEIVED DATA: %s\n", allItems);

    // Need to remove first SUCCESS\n bytes
    allItems += 8;

    // Need to tokenize now...
    char* token;
    char str2[] = {RECORD_SEPARATOR, '\0'};
    Item *items = (Item*)malloc(sizeof(Item)*MAX_ITEMS);
    bzero(items, MAX_ITEMS);
    Item *newItem;
    int available = 0;

    token = strtok(allItems, str2);
    while(token != NULL){
        newItem = (Item*)malloc(sizeof(Item));
        deserialize_item(token, newItem);
        items[available++] = *newItem;
        token = strtok(NULL, str2);
    }

//    for(int i =0; i < available; i++){
//    Item* cur = &items[i];
//    fprintf(stdout, "%s {%d, %d, %d, %d, %d, %f, %d, %s}\n",
//            cur->name, cur->armor, cur->health, cur->mana,
//            cur->sellPrice, cur->damage, cur->critChance, cur->range,
//            cur->description);
//    }

    return items;
}

void create_main_ui(){
    GtkBuilder *builder;
    GtkWidget *window;
    GError *err = NULL;

    builder = gtk_builder_new();

    if(!gtk_builder_add_from_file(builder, "./mainUI.glade", &err)){
        g_warning("%s", err->message);
        g_free(err);
        exit(-1);
    }

    mainWindow = GTK_WIDGET(gtk_builder_get_object(builder, "mainWindow"));
    createButton = GTK_WIDGET(gtk_builder_get_object(builder, "createButton"));
    modifyButton = GTK_WIDGET(gtk_builder_get_object(builder, "modifyButton"));
    deleteButton = GTK_WIDGET(gtk_builder_get_object(builder, "deleteButton"));
    listBox = GTK_WIDGET(gtk_builder_get_object(builder, "listBox"));
    itemListStore = GTK_LIST_STORE(gtk_builder_get_object(builder, "itemListStore"));

    gtk_builder_connect_signals(builder, NULL);
    g_object_unref(G_OBJECT(builder));

    Item* items = get_all_items_from_database();

    int i  = 0;
    Item* item = &items[i];
    while(item->name != NULL){
        GtkTreeIter iter;
        gtk_list_store_append(itemListStore, &iter);
        gtk_list_store_set(itemListStore, &iter,
                ID, item->id,
                NAME, item->name,
                ARMOR, item->armor,
                HEALTH, item->health,
                MANA, item->mana,
                SELL_PRICE, item->sellPrice,
                DAMAGE, item->damage,
                CRIT_CHANCE, item->critChance,
                RANGE, item->range,
                DESCRIPTION, item->description,
                -1);

        fprintf(stdout, "%s {%d, %d, %d, %d, %d, %f, %d, %s}\n",
                item->name, item->armor, item->health, item->mana,
                item->sellPrice, item->damage, item->critChance, item->range,
                item->description);
        item = &items[++i];
    }



    gtk_widget_show(mainWindow);
}