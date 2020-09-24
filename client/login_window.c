//
// Created by arch1t3ct on 9/23/20.
//

#include "login_window.h"
#include "network.h"
#include "../globals.h"

GtkWidget *usernameBox;
GtkWidget *passwordBox;
GtkWidget *serverBox;
GtkWidget *rememberMeCheckbox;
char usernameBuffer[BUFFER_SIZE];
char passwordBuffer[BUFFER_SIZE];
char serverAddress[BUFFER_SIZE];

G_MODULE_EXPORT void login(GtkWidget *widget, gpointer arg){
    // validate address
    // connect
    // authenticate
    // if success
    //    close this, open main, pass network socket
    // else
    //    error message
    unsigned int port = DEFAULT_SERVER_PORT;

    printf("Username: %s\n", getUsernameText());
    printf("Password: %s\n", getPasswordText());
    printf("Server address: %s\n", getServerAddressText());

    char* tmp;
    char remote_host[BUFFER_SIZE];

    tmp = strchr(serverAddress, ':');
    if(tmp == NULL){
        strncpy(remote_host, serverAddress, BUFFER_SIZE);
    } else {
        strncpy(remote_host, strtok(serverAddress, ':'), BUFFER_SIZE);
        port = (unsigned int) atoi(tmp+sizeof(char));
    }

    database_connect(remote_host, port);

    bzero(serverAddress, BUFFER_SIZE);
    sprintf(serverAddress, "AUTH %s %s", usernameBuffer, passwordBuffer);

    SSL_write(ssl, serverAddress, strlen(serverAddress));
}

void create_login_ui(){
    GtkBuilder *builder;
    GtkWidget *window;
    GError *err = NULL;

    builder = gtk_builder_new();

    if(!gtk_builder_add_from_file(builder, "../loginUI.glade", &err)){
        g_warning("%s", err->message);
        g_free(err);
        return;
    }

    window = GTK_WIDGET(gtk_builder_get_object(builder, "loginWindow"));
    usernameBox = GTK_WIDGET(gtk_builder_get_object(builder, "usernameTextBox"));
    passwordBox = GTK_WIDGET(gtk_builder_get_object(builder, "passwordTextBox"));
    serverBox = GTK_WIDGET(gtk_builder_get_object(builder, "serverAddressTextBox"));
    rememberMeCheckbox = GTK_WIDGET(gtk_builder_get_object(builder, "rememberMeCheckbox"));

    gtk_builder_connect_signals(builder, NULL);
    g_object_unref(G_OBJECT(builder));

    gtk_widget_show(window);
}

char* getUsernameText(){
    strncpy(usernameBuffer, gtk_entry_get_text(GTK_ENTRY(usernameBox)), BUFFER_SIZE);
    return usernameBuffer;
}

char* getPasswordText(){
    strncpy(passwordBuffer, gtk_entry_get_text(GTK_ENTRY(passwordBox)), BUFFER_SIZE);
    return passwordBuffer;
}

G_MODULE_EXPORT void clearText(){
    gtk_entry_set_text(GTK_ENTRY(usernameBox), "");
    gtk_entry_set_text(GTK_ENTRY(passwordBox), "");
    gtk_entry_set_text(GTK_ENTRY(serverBox), "localhost:4466");
}

char* getServerAddressText(){
    strncpy(serverAddress, gtk_entry_get_text(GTK_ENTRY(serverBox)), BUFFER_SIZE);
    return serverAddress;
}