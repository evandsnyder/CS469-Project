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
    //sprintf(buffer, "AUTH %s %s", "esnyder", "password");
    sprintf(buffer, "AUTH %s %s", "esnyder", "abcd123");
    // sprintf(buffer, "SYNC");

    SSL_write(ssl, buffer, strlen(buffer));

    bzero(buffer, BUFFER_SIZE);
    SSL_read(ssl, buffer, BUFFER_SIZE);

    fprintf(stdout, "MSG Received: %s\n", buffer);

    bzero(buffer, BUFFER_SIZE);
    sprintf(buffer, "GET ALL");
    SSL_write(ssl, buffer, strlen(buffer));

    int rcount = 1;
    fprintf(stdout, "Message Received: ");
    while(rcount > 0){
        rcount = SSL_read(ssl, buffer, BUFFER_SIZE);
        fprintf(stdout, "%s", buffer);
    }
    fprintf(stdout, "\nEnd Message\n\n");
    return;

    size_t mem_size = 1024*4;
    size_t cur_size = 1024*4;
    char *large_buf = (char*)malloc(sizeof(char)*mem_size);
    large_buf[0] = '\0';

    fprintf(stdout, "Received from client:\n");
    rcount = 1;
    while(rcount >= 0){
        bzero(buffer, BUFFER_SIZE);
        rcount = SSL_read(ssl, buffer, BUFFER_SIZE);
        //fprintf(stdout, "%s", buffer);
        // Need to stitch all of the files together.
        fprintf(stdout, "Read %d bytes\n", rcount);
        if(strlen(large_buf) + rcount >= cur_size){
            // realloc
            cur_size += mem_size;
            large_buf = realloc(large_buf, cur_size);
            fprintf(stdout, "Memory reallocated\n");
        }
        strncat(large_buf, buffer, strlen(buffer));
        fprintf(stdout, "String concatted\n");
        fprintf(stdout, "\ncur: %s\n", large_buf);
        if(buffer[strlen(buffer)-1] == GROUP_SEPARATOR){
            rcount = -1;
        }
    }

    fprintf(stdout, "%s\n", large_buf);
    fprintf(stdout, "\nEnd of message\n");
}
