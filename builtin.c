#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "command.h"
#include "builtin.h"
#include <unistd.h>
#include <assert.h>
#include "tests/syscall_mock.h"

static void builtin_cd(scommand cmd){
    //Si el comando es [cd] utilizamos chdir()
    if(scommand_length(cmd) == 0){
        //Si no hay argumentos, vamos al home del usuario.
        chdir(getenv("HOME"));
    } 
    else{
        //Si hay argumentos, vamos al path indicado.
        int result = chdir(scommand_front(cmd));
        if(result != 0){
            fprintf(stderr, "bash: cd: %s: No such file or directory \n", scommand_front(cmd));
        }
    }
}

static void builtin_exit(void){
    //Si el comando es [exit] utilizamos exit()
    exit(EXIT_SUCCESS);
}
static void builtin_help(void){
    //Si el comando es [help] printeamos por stdout el mensaje de ayuda.
    char *text= ""
        "-DTML- own Shell\n\n" //completar
        "AUTORES: \n"
        "Lisandro Allio\n"
        "Tomas Romanutti\n"
        "Dianela Fernandez\n"
        "Mauricio Pintos\n\n"
        "COMANDOS: \n"
        "-cd:   Cambia el directorio actual.\n"
        "-exit: Sale del shell.\n"
        "-help: Muestra este mensaje de ayuda.\n\n";
    printf("%s", text);
}

bool builtin_is_internal(scommand cmd){
    assert(cmd != NULL);
    //Tenemos tres comandos internos: [cd], [exit] y [help] entonces vemos; 
    char* cmd_name = scommand_front(cmd);
    bool result = false;
    result = (strcmp(cmd_name, "cd") == 0 || strcmp(cmd_name, "exit") == 0 || strcmp(cmd_name, "help") == 0);
    return result;
}

bool builtin_alone(pipeline p){
    assert(p != NULL);
    //Si el pipeline tiene solo un elemento y si este se corresponde a un comando interno.
    bool result = false;
    result = (pipeline_length(p) == 1 && builtin_is_internal(pipeline_front(p)));
    assert(result == (pipeline_length(p) == 1 && builtin_is_internal(pipeline_front(p))));
    return result;
}

void builtin_run(scommand cmd){
    assert(builtin_is_internal(cmd));
    //Ejecuta un comando interno, utilizamos las funciones estaticas.
    if(strcmp(scommand_front(cmd), "cd") == 0){ 
        scommand_pop_front(cmd); //ELiminamos el comando "cd" que esta primero de la lista, para tener al frente el path
        builtin_cd(cmd); //Llamamos a cd con el path
    }
    else if(strcmp(scommand_front(cmd), "exit") == 0){
        builtin_exit(); 
    }
    else if(strcmp(scommand_front(cmd), "help") == 0){
        builtin_help(); 
    }
}


