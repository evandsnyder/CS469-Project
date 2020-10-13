//
// Created by arch1t3ct on 10/1/20.
//

#include "main_window.h"
#include "network.h"



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

    GtkAdjustment* armorAdjustment;
    GtkAdjustment* priceAdjustment;
    GtkAdjustment* manaAdjustment;
    GtkAdjustment* healthAdjustment;
    GtkAdjustment* damageAdjustment;
    GtkAdjustment* critAdjustment;
    GtkAdjustment* rangeAdjustment;

};

GtkWidget* createButton;
GtkWidget* modifyButton;
GtkWidget* deleteButton;
GtkWidget* mainWindow;
GtkWidget* listBox;
GtkWidget* editItemDialogWidget;
GtkListStore* itemListStore;
GtkBuilder *builder;
GtkTreeView* itemTreeView;
GtkTreeSelection *selection;
GtkTreeIter selectedIter;
GtkTreeModel *itemModel;
struct editItemWidget *itemEditor;


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


gboolean onWidgetDelete(GtkWidget *widget, GdkEvent *event, gpointer data){
    gtk_widget_hide(widget);
    return TRUE;
}

void cancelItemEdit(GtkWidget* widget, gpointer data){
    printf("Canceled!\n");
    gtk_widget_hide(GTK_WIDGET(data));
}

void saveItemEdit(GtkWidget* widget, gpointer data){
    printf("Saved!\n");
    gtk_widget_hide(GTK_WIDGET(data));

    // Get data from all relevant fields
    Item *item = (Item*)malloc(sizeof(Item));
    item->name = (char*)malloc(sizeof(char)*BUFFER_SIZE);
    item->description = (char*)malloc(sizeof(char)*BUFFER_SIZE);

    // gtk_label_get_text();
}

void openItemEditor(Item *item){

    char t[BUFFER_SIZE] = {0};
    sprintf(t, "%d", item->id);

    gtk_label_set_text(itemEditor->itemId, t);
    gtk_entry_set_text(itemEditor->itemName, item->name);
    gtk_spin_button_set_value(itemEditor->itemArmor, item->armor);
    gtk_spin_button_set_value(itemEditor->itemHealth, item->health);
    gtk_spin_button_set_value(itemEditor->itemMana, item->mana);
    gtk_spin_button_set_value(itemEditor->itemPrice, item->sellPrice);
    gtk_spin_button_set_value(itemEditor->itemDamage, item->damage);
    gtk_spin_button_set_value(itemEditor->itemCrit, item->critChance);
    gtk_spin_button_set_value(itemEditor->itemRange, item->range);
    gtk_entry_set_text(itemEditor->itemDescription, item->description);

    gtk_widget_show(editItemDialogWidget);

}

G_MODULE_EXPORT void editItemDialog(gpointer user_data){
    Item *item = (Item*)malloc(sizeof(Item));
    item->name = (char*)malloc(sizeof(char)*BUFFER_SIZE);
    bzero(item->name, BUFFER_SIZE);
    item->description = (char*)malloc(sizeof(char)*BUFFER_SIZE);
    bzero(item->description, BUFFER_SIZE);

    // Get selected Item
    if(gtk_tree_selection_get_selected(selection, &itemModel, &selectedIter)){
        gtk_tree_model_get(itemModel, &selectedIter,
                ID, &item->id,
                NAME, &item->name,
                ARMOR, &item->armor,
                HEALTH, &item->health,
                MANA, &item->mana,
                SELL_PRICE, &item->sellPrice,
                DAMAGE, &item->damage,
                CRIT_CHANCE, &item->critChance,
                RANGE, &item->range,
                DESCRIPTION, &item->description,
                -1);
    }

    if(item->name == NULL){
        display_error_dialog("Could not load item for editing");
    } else {
        fprintf(stdout, "Selected Item: %s: %s\n", item->name, item->description);
    }

    // Craft the Item
    openItemEditor(item);
}

G_MODULE_EXPORT void newItemDialog(gpointer data){
    fprintf(stdout, "Creating new item\n");

    Item *item = (Item*)malloc(sizeof(Item));
    item->name = (char*)malloc(sizeof(char)*BUFFER_SIZE);
    item->description = (char*)malloc(sizeof(char)*BUFFER_SIZE);
    item->id = -1;
    item->name = "\0";
    item->description = "\0";
    item->armor = 0;
    item->health = 0;
    item->mana = 0;
    item->sellPrice = 0;
    item->damage = 0;
    item->critChance = 0.0;
    item->range = 0;

     openItemEditor(item);
}

void display_error_dialog(char* msg){
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(mainWindow), GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Error %s", msg);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

Item * get_all_items_from_database(){
    // One extra char to guarantee null-termination
    char buffer[BUFFER_SIZE + 1] = {0};
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

    return items;
}

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
    listBox = GTK_WIDGET(gtk_builder_get_object(builder, "listBox"));
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
    itemEditor->armorAdjustment = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "armorAdjustment"));
    itemEditor->priceAdjustment = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "priceAdjustment"));
    itemEditor->manaAdjustment = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "manaAdjustment"));
    itemEditor->healthAdjustment = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "healthAdjustment"));
    itemEditor->damageAdjustment = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "damageAdjustment"));
    itemEditor->critAdjustment = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "critAdjustment"));
    itemEditor->rangeAdjustment = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "rangeAdjustment"));

    g_signal_connect(itemEditor->editItemCancelButton, "clicked", G_CALLBACK(cancelItemEdit), editItemDialogWidget);
    g_signal_connect(itemEditor->editItemSaveButton, "clicked", G_CALLBACK(saveItemEdit), editItemDialogWidget);
    g_signal_connect(editItemDialogWidget, "delete-event", G_CALLBACK(onWidgetDelete), NULL);

    gtk_window_set_transient_for(GTK_WINDOW(editItemDialogWidget), GTK_WINDOW(mainWindow));

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
        item = &items[++i];
    }


    selection = gtk_tree_view_get_selection(itemTreeView);
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

    g_signal_connect(modifyButton, "clicked", G_CALLBACK(editItemDialog), NULL);
    g_signal_connect(createButton, "clicked", G_CALLBACK(newItemDialog), NULL);


    gtk_widget_show(mainWindow);
}