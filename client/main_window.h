//
// Created by arch1t3ct on 10/1/20.
//

#ifndef CS469_PROJECT_MAIN_WINDOW_H
#define CS469_PROJECT_MAIN_WINDOW_H

#include <gtk/gtk.h>
#include "../globals.h"

void create_main_ui();
void display_error_dialog(char* msg);
void get_all_items_from_database();

#endif //CS469_PROJECT_MAIN_WINDOW_H
