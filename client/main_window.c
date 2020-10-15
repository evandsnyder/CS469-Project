//
// Created by arch1t3ct on 10/1/20.
//

#include "main_window.h"
#include "network.h"


/**
 * Struct to hold all relevant widgets for the item editor
 */
struct editItemWidget{
    GtkWidget *editItemSaveButton;
    GtkWidget *editItemCancelButton;
    GtkLabel *itemId;
    GtkEntry *itemName;
    GtkEntry *itemDescription;
    GtkSpinButton *itemArmor;
    GtkSpinButton *itemPrice;
    GtkSpinButton *itemMana;
    GtkSpinButton *itemHealth;
    GtkSpinButton *itemDamage;
    GtkSpinButton *itemCrit;
    GtkSpinButton *itemRange;

};

GtkWidget* createButton;
GtkWidget* modifyButton;
GtkWidget* deleteButton;
GtkWidget* mainWindow;
GtkWidget* editItemDialogWidget;
GtkListStore* itemListStore;
GtkBuilder *builder;
GtkTreeView* itemTreeView;
GtkTreeSelection *selection;
GtkTreeIter selectedIter;
GtkTreeModel *itemModel;
struct editItemWidget *itemEditor;

/**
 * ENUM used for assigning columns for the treeview
 */
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

/**
 * Display a yes/no dialog for user confirmation
 * @param widget
 * @param msg
 * @return
 */
gboolean displayConfirmationDialog(GtkWidget *widget, char* msg){
    GtkWidget *confirmDialog = gtk_message_dialog_new(GTK_WINDOW(widget),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,"%s", msg);

    int result = gtk_dialog_run(GTK_DIALOG(confirmDialog));
    gtk_widget_destroy(confirmDialog);

    switch(result){
        case GTK_RESPONSE_YES:
            return TRUE;
        case GTK_RESPONSE_NO:
        default:
            return FALSE;
    }
}

/**
 * Widget to close edit dialog without causing issues with the destroy event
 * @param widget
 * @param event
 * @param data
 * @return
 */
gboolean onWidgetDelete(GtkWidget *widget, GdkEvent *event, gpointer data){
    gtk_widget_hide(widget);
    return TRUE;
}

/**
 * Signal handler for pressing cancel button in the item editor
 * @param widget
 * @param data
 */
void cancelItemEdit(GtkWidget* widget, gpointer data){
    gtk_widget_hide(GTK_WIDGET(data));
}

/**
 * Signal handler for pressing the save button in the item editor
 *
 * This will either create a PUT or a MOD request depending on if
 * we are editing an item or creating a new one.
 *
 * @param widget
 * @param data
 */
void saveItemEdit(GtkWidget* widget, gpointer data){
    gtk_widget_hide(GTK_WIDGET(data));

    // Get data from all relevant fields
    Item *item = (Item*)malloc(sizeof(Item));
    bzero(item->name, BUFFER_SIZE);
    bzero(item->description, BUFFER_SIZE);

    item->id = atoi(gtk_label_get_text(itemEditor->itemId));
    snprintf(item->name, BUFFER_SIZE, "%s", (char*)gtk_entry_get_text(itemEditor->itemName));
    item->armor = (int)gtk_spin_button_get_value(itemEditor->itemArmor);
    item->health = (int)gtk_spin_button_get_value(itemEditor->itemHealth);
    item->mana = (int)gtk_spin_button_get_value(itemEditor->itemMana);
    item->sellPrice = (int)gtk_spin_button_get_value(itemEditor->itemPrice);
    item->damage = (int)gtk_spin_button_get_value(itemEditor->itemDamage);
    item->critChance = gtk_spin_button_get_value(itemEditor->itemCrit);
    item->range = (int)gtk_spin_button_get_value(itemEditor->itemRange);
    snprintf(item->description, BUFFER_SIZE, "%s", (char*)gtk_entry_get_text(itemEditor->itemDescription));

    char* serialized_item = NULL;
    serialized_item = serialize_item(item, serialized_item);

    char* msg = (char*)calloc(sizeof(char), strlen(serialized_item) + 5);
    if(item->id == -1){
        strncat(msg, "PUT ", 5);
    } else {
        strncat(msg, "MOD ", 5);
    }

    strcat(msg, serialized_item);
    if(SSL_write(ssl, msg, strlen(msg)) < 0){
        display_error_dialog("Error communicating with server");
    }

    free(item);
    free(msg);
    free(serialized_item);

    char response[BUFFER_SIZE] = {0};
    SSL_read(ssl, response, BUFFER_SIZE);
    if(strcmp(response, "FAILURE") == 0){
        display_error_dialog("Unable to add to database");
    }else{
        get_all_items_from_database();
    }
}

/**
 * Creates a delete request for the currently selected item.
 * Will display a confirmation dialog to ensure user decision
 * @param widget
 * @param user_data
 */
void deleteItemHandler(GtkWidget* widget, gpointer user_data){

    int id = -1;
    if(gtk_tree_selection_get_selected(selection, &itemModel, &selectedIter)) {
        gtk_tree_model_get(itemModel, &selectedIter,
                           ID, &id, -1);
    }

    char* msg1 = (char*)malloc(sizeof(char)*BUFFER_SIZE);
    sprintf(msg1, "Are you sure you want to delete this item? (ID: %d)", id);

    if(displayConfirmationDialog(GTK_WIDGET(user_data), msg1) == FALSE){
        return;
    }
    free(msg1);

    char msg[BUFFER_SIZE];
    sprintf(msg, "DEL %d", id);
    if(SSL_write(ssl, msg, strlen(msg)) < 0){
        display_error_dialog("Error communicating with server");
    }

    char response[BUFFER_SIZE] = {0};
    SSL_read(ssl, response, BUFFER_SIZE);
    if(strcmp(response, "FAILURE") == 0){
        display_error_dialog("Could not delete item from database");
    } else {
        get_all_items_from_database();
    }
}

/**
 * Method for displaying the item editor with for a given item
 * Sets the appropriate fields from the item
 * @param item - Item being edited
 */
void openItemEditor(Item *item){

    char t[BUFFER_SIZE] = {0};
    sprintf(t, "%d", item->id);

    gtk_label_set_text(itemEditor->itemId, t);
    sprintf(t, "%s", item->name);
    gtk_entry_set_text(itemEditor->itemName, t);
    gtk_spin_button_set_value(itemEditor->itemArmor, item->armor);
    gtk_spin_button_set_value(itemEditor->itemHealth, item->health);
    gtk_spin_button_set_value(itemEditor->itemMana, item->mana);
    gtk_spin_button_set_value(itemEditor->itemPrice, item->sellPrice);
    gtk_spin_button_set_value(itemEditor->itemDamage, item->damage);
    gtk_spin_button_set_value(itemEditor->itemCrit, item->critChance);
    gtk_spin_button_set_value(itemEditor->itemRange, item->range);
    gtk_entry_set_text(itemEditor->itemDescription, item->description);

    freeItem(item);

    gtk_widget_show(editItemDialogWidget);

}

/**
 * Signal handler for pressing the edit item button
 * Will pull the relevant information from the table and create an Item to be edited
 * @param user_data
 */
G_MODULE_EXPORT void editItemDialog(gpointer user_data){
    Item *item = (Item*)malloc(sizeof(Item));
    bzero(item->name, BUFFER_SIZE);
    bzero(item->description, BUFFER_SIZE);

    gchar* name = (gchar*)malloc(sizeof(char)*BUFFER_SIZE);
    bzero(name, BUFFER_SIZE);
    gchar* desc = (gchar*)malloc(sizeof(char)*BUFFER_SIZE);
    bzero(name, BUFFER_SIZE);

    // Get selected Item
    if(gtk_tree_selection_get_selected(selection, &itemModel, &selectedIter)){
        gtk_tree_model_get(itemModel, &selectedIter,
                ID, &item->id,
                NAME, &name,
                ARMOR, &item->armor,
                HEALTH, &item->health,
                MANA, &item->mana,
                SELL_PRICE, &item->sellPrice,
                DAMAGE, &item->damage,
                CRIT_CHANCE, &item->critChance,
                RANGE, &item->range,
                DESCRIPTION, &desc,
                -1);
    }

    if(name == NULL){
        display_error_dialog("Could not load item for editing");
    }

    snprintf(item->name, BUFFER_SIZE, "%s", name);
    snprintf(item->description, BUFFER_SIZE, "%s", desc);
    g_free(name);
    g_free(desc);

    // Craft the Item
    openItemEditor(item);
}

/**
 * Signal Handler for creating a new Item
 *
 * Creates a new item with empty field values. an ID of -1 is used to indicate that the item is new
 *
 * @param data
 */
G_MODULE_EXPORT void newItemDialog(gpointer data){

    Item *item = (Item*)malloc(sizeof(Item));
    bzero(item->name, BUFFER_SIZE);
    bzero(item->description, BUFFER_SIZE);
    item->id = -1;
    item->armor = 0;
    item->health = 0;
    item->mana = 0;
    item->sellPrice = 0;
    item->damage = 0;
    item->critChance = 0.0;
    item->range = 0;

    openItemEditor(item);
}

/**
 * Helper method for displaying an error message to the user
 * @param msg
 */
void display_error_dialog(char* msg){
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(mainWindow), GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Error %s", msg);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/**
 * Method queries the database for all entities.
 * Will wipe the tree and re-display all found entities
 */
void get_all_items_from_database(){
    // One extra char to guarantee null-termination
    char buffer[BUFFER_SIZE + 1] = {0};
    sprintf(buffer, "GET ALL");
    SSL_write(ssl, buffer, strlen(buffer));

    bzero(buffer, BUFFER_SIZE);
    SSL_read(ssl, buffer, BUFFER_SIZE);
    if(strcmp("FAILURE", buffer) == 0){
        display_error_dialog("Could not Retrieve items from database");
        return;
    }

    size_t mem_size = 1024*4;
    size_t cur_size = 1024*4;
    char* allItems = (char*)malloc(sizeof(char)*mem_size);
    bzero(allItems, mem_size);
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


    // EXPERIMENTAL
    if(strlen(buffer) + strlen(allItems) >= cur_size){
        cur_size += mem_size;
        allItems = realloc(allItems, cur_size);
    }
    strncat(allItems, buffer, strlen(buffer)-1);

    // Need to remove first SUCCESS\n bytes
    allItems += 8;

    // Need to tokenize now...
    char* token;
    char str2[] = {RECORD_SEPARATOR, '\0'};
    Item *items[MAX_ITEMS];
    bzero(items, MAX_ITEMS);
    Item *newItem;
    int available = 0;

    token = strtok(allItems, str2);
    while(token != NULL){
        newItem = (Item*)malloc(sizeof(Item));
        deserialize_item(token, newItem);
        items[available++] = newItem;
        token = strtok(NULL, str2);
    }

    free(token);
    allItems -=8; // Must account for the removed bytes
    free(allItems);

    gtk_list_store_clear(itemListStore);


    for(int i = 0; i < available; i++){
        GtkTreeIter iter;
        gtk_list_store_append(itemListStore, &iter);
        gtk_list_store_set(itemListStore, &iter,
                ID, items[i]->id,
                NAME, items[i]->name,
                ARMOR, items[i]->armor,
                HEALTH, items[i]->health,
                MANA, items[i]->mana,
                SELL_PRICE, items[i]->sellPrice,
                DAMAGE, items[i]->damage,
                CRIT_CHANCE, items[i]->critChance,
                RANGE, items[i]->range,
                DESCRIPTION, items[i]->description,
                -1);
        freeItem(items[i]);
    }

    GtkTreeIter iter;
    gtk_tree_model_get_iter_first(itemModel, &iter);
    gtk_tree_selection_select_iter(selection, &iter);
}

/**
 * Loads the main UI from the UI file and binds all the relevant variables
 * Connects all action signals.
 */
void create_main_ui(){
    GError *err = NULL;

    builder = gtk_builder_new();

    if(!gtk_builder_add_from_file(builder, "./mainUI.glade", &err)){
        g_warning("%s", err->message);
        g_free(err);
        exit(-1);
    }

    itemEditor = (struct editItemWidget*)malloc(sizeof(struct editItemWidget));

    mainWindow = GTK_WIDGET(gtk_builder_get_object(builder, "mainWindow"));
    createButton = GTK_WIDGET(gtk_builder_get_object(builder, "createButton"));
    modifyButton = GTK_WIDGET(gtk_builder_get_object(builder, "modifyButton"));
    deleteButton = GTK_WIDGET(gtk_builder_get_object(builder, "deleteButton"));
    itemListStore = GTK_LIST_STORE(gtk_builder_get_object(builder, "itemListStore"));
    itemTreeView = GTK_TREE_VIEW(gtk_builder_get_object(builder, "itemTreeView"));
    editItemDialogWidget = GTK_WIDGET(gtk_builder_get_object(builder, "editItemDialog"));

    itemEditor->editItemSaveButton = GTK_WIDGET(gtk_builder_get_object(builder, "editItemSaveButton"));
    itemEditor->editItemCancelButton = GTK_WIDGET(gtk_builder_get_object(builder, "editItemCancelButton"));
    itemEditor->itemId = GTK_LABEL(gtk_builder_get_object(builder, "itemId"));
    itemEditor->itemName = GTK_ENTRY(gtk_builder_get_object(builder, "itemName"));
    itemEditor->itemDescription = GTK_ENTRY(gtk_builder_get_object(builder, "itemDescription"));
    itemEditor->itemArmor = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "itemArmor"));
    itemEditor->itemPrice = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "itemPrice"));
    itemEditor->itemMana = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "itemMana"));
    itemEditor->itemHealth = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "itemHealth"));
    itemEditor->itemDamage = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "itemDamage"));
    itemEditor->itemCrit = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "itemCrit"));
    itemEditor->itemRange = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "itemRange"));

    itemModel = gtk_tree_view_get_model(itemTreeView);

    g_signal_connect(itemEditor->editItemCancelButton, "clicked", G_CALLBACK(cancelItemEdit), editItemDialogWidget);
    g_signal_connect(itemEditor->editItemSaveButton, "clicked", G_CALLBACK(saveItemEdit), editItemDialogWidget);
    g_signal_connect(editItemDialogWidget, "delete-event", G_CALLBACK(onWidgetDelete), NULL);

    gtk_window_set_transient_for(GTK_WINDOW(editItemDialogWidget), GTK_WINDOW(mainWindow));

    gtk_builder_connect_signals(builder, NULL);
    g_object_unref(G_OBJECT(builder));

    selection = gtk_tree_view_get_selection(itemTreeView);
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

    g_signal_connect(modifyButton, "clicked", G_CALLBACK(editItemDialog), NULL);
    g_signal_connect(createButton, "clicked", G_CALLBACK(newItemDialog), NULL);
    g_signal_connect(deleteButton, "clicked", G_CALLBACK(deleteItemHandler), G_OBJECT(mainWindow));

    get_all_items_from_database();


    gtk_widget_show(mainWindow);
}
