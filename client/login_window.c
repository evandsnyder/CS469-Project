//
// Created by arch1t3ct on 9/23/20.
//

#include "login_window.h"
#include "../globals.h"

GtkWidget *usernameBox;
GtkWidget *passwordBox;
GtkWidget *serverBox;
GtkWidget *rememberMeCheckbox;
GtkWidget *loginWindow;
gulong destroyHandler;
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

    getUsernameText();
    getPasswordText();
    getServerAddressText();

    char* tmp;
    char remote_host[BUFFER_SIZE];

    tmp = strchr(serverAddress, ':');
    if(tmp == NULL){
        strncpy(remote_host, serverAddress, BUFFER_SIZE);
    } else {
        strncpy(remote_host, strtok(serverAddress, ":"), BUFFER_SIZE);
        port = (unsigned int) atoi(tmp+sizeof(char));
    }
    database_connect(remote_host, port);

    bzero(serverAddress, BUFFER_SIZE);
    sprintf(serverAddress, "AUTH %s %s", usernameBuffer, passwordBuffer);

    if(SSL_write(ssl, serverAddress, strlen(serverAddress)) <= 0){
        fprintf(stderr, "Error writing to server: %s\n", strerror(errno));
        // need to create popup dialog here
        GtkWidget *dialog = gtk_message_dialog_new(loginWindow, GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Could not connect to server");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        return;
    }

    bzero(serverAddress, BUFFER_SIZE);
    if(SSL_read(ssl, serverAddress, BUFFER_SIZE) <= 0){
        fprintf(stderr, "Error reading from server: %s\n", strerror(errno));
        disconnect();
        exit(-1);
    }

    if(strcmp("FAILURE", serverAddress) == 0){
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(loginWindow),
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    GTK_MESSAGE_ERROR,
                                                    GTK_BUTTONS_CLOSE,
                                                    "Invalid username or password");
        g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_widget_destroy), dialog);
        gtk_dialog_run(GTK_DIALOG(dialog));
        disconnect();
        return;
    }

    g_signal_handler_disconnect(loginWindow, destroyHandler);

    create_main_ui();
    gtk_window_close(GTK_WINDOW(loginWindow));
}

void create_login_ui(){
    GtkBuilder *builder;
    GError *err = NULL;

    builder = gtk_builder_new();

    if(!gtk_builder_add_from_file(builder, "loginUI.glade", &err)){
        g_warning("%s", err->message);
        g_free(err);
        return;
    }

    loginWindow = GTK_WIDGET(gtk_builder_get_object(builder, "loginWindow"));
    usernameBox = GTK_WIDGET(gtk_builder_get_object(builder, "usernameTextBox"));
    passwordBox = GTK_WIDGET(gtk_builder_get_object(builder, "passwordTextBox"));
    serverBox = GTK_WIDGET(gtk_builder_get_object(builder, "serverAddressTextBox"));
    rememberMeCheckbox = GTK_WIDGET(gtk_builder_get_object(builder, "rememberMeCheckbox"));
    GtkWidget* loginButton = GTK_WIDGET(gtk_builder_get_object(builder, "loginButton"));

    destroyHandler = g_signal_connect(loginWindow, "delete-event", G_CALLBACK(gtk_main_quit), NULL);
    // g_signal_connect(loginButton, "clicked", G_CALLBACK (login), (gpointer)"Login");

    gtk_builder_connect_signals(builder, NULL);
    g_object_unref(G_OBJECT(builder));

    gtk_widget_show(loginWindow);
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