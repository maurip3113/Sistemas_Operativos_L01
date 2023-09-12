#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "command.h"
#include "execute.h"
#include "parser.h"
#include "parsing.h"
#include "builtin.h"

//Colores para el bash
#define GRN   "\x1B[32m"
#define BLU   "\x1B[34m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"


static void show_prompt(void) {
    char dir[64];
    char host[64];
    char user[64];
    getlogin_r(user,64);           //Obtenemos el username
    getcwd(dir, 64);               //Obtenemos el directorio
    gethostname(host, 64);         //Obtenemos el host
    printf(GRN "%s@%s" RESET WHT ":" RESET BLU "~%s" RESET WHT "$ " RESET , user, host, dir);

    fflush (stdout);
}

int main(int argc, char *argv[]) {
    pipeline pipe;
    Parser input;
    bool quit = false;

    input = parser_new(stdin);
    while (!quit) {
        show_prompt();
        pipe = parse_pipeline(input);
        
        quit = parser_at_eof(input);
        if(pipe != NULL){
            execute_pipeline(pipe);
            pipeline_destroy(pipe); pipe = NULL;
        }
    }
    printf("\n");
    parser_destroy(input); input = NULL;
    return EXIT_SUCCESS;
}

