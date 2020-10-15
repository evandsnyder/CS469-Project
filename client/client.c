#include "login_window.h"

/**
 * Entry point for the program. Initializes GTK and starts
 * the login sequence.
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv){

    gtk_init(&argc, &argv);

    create_login_ui();
    gtk_main();

    return 0;
}