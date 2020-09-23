#include <stdio.h>

#include "../globals.h"
#include "login_window.h"

int main(int argc, char** argv){
    printf("Hello from client!\n");

    gtk_init(&argc, &argv);

    create_login_ui();
    gtk_main();

    return 0;
}