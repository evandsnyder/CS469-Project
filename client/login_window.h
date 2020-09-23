//
// Created by arch1t3ct on 9/23/20.
//

#ifndef CS469_PROJECT_LOGIN_WINDOW_H
#define CS469_PROJECT_LOGIN_WINDOW_H

#include <gtk/gtk.h>

#define BUFFER_SIZE 256

void show_login_screen();
char *getUsernameText();
char *getPasswordText();
char *getServerAddressText();
void clearText();
void login(GtkWidget *widget, gpointer arg);
void create_login_ui();

#endif //CS469_PROJECT_LOGIN_WINDOW_H
