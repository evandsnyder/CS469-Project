#include <stdio.h>

#include "../globals.h"
#include "login_window.h"
#include "network.h"

int main(int argc, char** argv){

    gtk_init(&argc, &argv);

    create_login_ui();
    gtk_main();

    return 0;
}