#include <stdio.h>

#include "../globals.h"
#include "login_window.h"
#include "network.h"

void DEBUG_network();

int main(int argc, char** argv){
    printf("Hello from client!\n");

    DEBUG_network();

//    gtk_init(&argc, &argv);
//
//    create_login_ui();
//    gtk_main();

    return 0;
}

void DEBUG_network(){
    char buffer[BUFFER_SIZE];
    if(database_connect(DEFAULT_SERVER, DEFAULT_SERVER_PORT) < 0){
        fprintf(stderr, "Could not connect to server\n");
    }

    bzero(buffer, BUFFER_SIZE);
    // sprintf(buffer, "AUTH %s %s", "esnyder", "password");
    sprintf(buffer, "AUTH %s %s", "esnyder", "notarealpassword");

    SSL_write(ssl, buffer, strlen(buffer));
}