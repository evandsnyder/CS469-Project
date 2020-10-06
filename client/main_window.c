//
// Created by arch1t3ct on 10/1/20.
//

#include "main_window.h"

GtkWidget* createButton;
GtkWidget* modifyButton;
GtkWidget* deleteButton;
GtkWidget* mainWindow;
GtkWidget* listBox;

G_MODULE_EXPORT void editItemDialog(){
    // Will be used for editing and creating both hopefully
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

    gtk_builder_connect_signals(builder, NULL);
    g_object_unref(G_OBJECT(builder));

    gtk_widget_show(mainWindow);
}