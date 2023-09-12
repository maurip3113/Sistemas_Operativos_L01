#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "command.h"
#include "builtin.h"
#include "execute.h"
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h> // open
#include <sys/stat.h> // O_CREAT
#include "tests/syscall_mock.h"

typedef int fd_t;    //Sinonimo de tipo para los descriptores de archivo.

/* Ejecuta un comando como comando externo sin hacer fork. 
 * Redirige el stdin y el stdout si están seeteados.
 *
 * Requires: cmd != NULL && !scommand_is_empty(cmd)
 */
static void execute_extern(scommand cmd){
    assert(cmd != NULL && !scommand_is_empty(cmd));
    unsigned int length = scommand_length(cmd);
    char **args = calloc(length + 1, sizeof(char*));
    for(unsigned int i = 0; i < length; ++i){
        char *aux = scommand_front(cmd);
        args[i] = strdup(aux); 
        scommand_pop_front(cmd);
    }
    char *in = scommand_get_redir_in(cmd);
    if (in != NULL) {
        fd_t fd = open(in , O_RDONLY, 0666);
        if(fd < 0){
            fprintf(stderr, "bash: %s: No such file or directory.\n", in);
            exit(EXIT_FAILURE);
        }
        dup2(fd , 0);
        close(fd);
    }
    char *out = scommand_get_redir_out(cmd);
    if (out != NULL) {
        fd_t fd = open(out , O_CREAT | O_WRONLY, 0666);
        if(fd < 0){
            fprintf(stderr, "bash: %s: No such file or directory.\n", in);
            exit(EXIT_FAILURE);
        }
        dup2(fd , 1);
        close(fd);
    }
    execvp(args[0], args);
    fprintf(stderr, "%s: Command not found.\n", args[0]);
    exit(EXIT_FAILURE);
}

/* Ejecuta un pipeline de un solo comando externo relizando un fork
 *
 * Requires: cmd != NULL && pipeline_length(apipe) == 1
 */
static void execute_one_command(pipeline apipe){
    assert(apipe != NULL && pipeline_length(apipe) == 1);
    unsigned int child_processes_count = 0;   //Contador de procesos hijos creados
    pid_t cpid = fork();
    if (cpid < 0){
        fprintf(stderr, "fork() has failed.\n");    
        exit(EXIT_FAILURE);
    }
    else if (cpid == 0) {
        execute_extern(pipeline_front(apipe));
    }
    else {
        if (pipeline_get_wait(apipe)){
            wait(NULL);     
        }
        pipeline_pop_front(apipe);
        child_processes_count++;
    }
    assert(child_processes_count == 1);
}
/* Ejecuta un pipeline de dos solo comando externo relizando dos fork y un pipe
 *
 * Requires: cmd != NULL && pipeline_length(apipe) == 2
 */
static void execute_two_commands(pipeline apipe){
    assert(apipe != NULL && pipeline_length(apipe) == 2);
    unsigned int child_processes_count = 0;   //Contador de procesos hijos creados
    fd_t fd[2];
    pipe(fd);
    pid_t cpid = fork();
    if (cpid < 0){
        fprintf(stderr, "fork() has failed.\n");    
        exit(EXIT_FAILURE);
    }
    if(cpid == 0){
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        execute_extern(pipeline_front(apipe));
    }
    pipeline_pop_front(apipe);
    child_processes_count++;
        
    pid_t cpid_bis = fork();
    if(cpid_bis == 0){
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        close(fd[1]);

        execute_extern(pipeline_front(apipe));
    }
    else{ 
        close(fd[0]);
        close(fd[1]);
        pipeline_pop_front(apipe);
        child_processes_count++;
    }

    if(pipeline_get_wait(apipe)){
        waitpid(cpid,NULL,0);
        waitpid(cpid_bis,NULL,0);
    }
    assert(child_processes_count == 2);
}

/* Ejecuta un pipeline de trem o mas comandos externos relizando multiples fork y pipes
 *
 * Requires: cmd != NULL && pipeline_length(apipe) > 2
 */
static void execute_multiple_commands(pipeline apipe){
    assert(apipe != NULL && pipeline_length(apipe) > 2);
    fd_t stdio_fd = dup(STDIN_FILENO);             //Descriptor de archivo del stdio.
    char *buf;                                    //buf almacena lo que vamos a printear en stdout en el caso de 3 o mas comandos
    while(!pipeline_is_empty(apipe)) {    
        fd_t fd[2]; pid_t cpid;
        if (pipe(fd) == -1) {
            fprintf(stderr, "pipe() has failed.\n");
            exit(EXIT_FAILURE);
        }
        cpid = fork();
        if (cpid < 0){
            fprintf(stderr, "fork() has failed.\n");    
            exit(EXIT_FAILURE);
        }
        else if (cpid == 0){   //hijo
            dup2(fd[1], STDOUT_FILENO); //stdout -> pipe 

            execute_extern(pipeline_front(apipe));
        }
        else{      //padre
            if (pipeline_get_wait(apipe)){  
                wait(NULL);       
            }
            dup2(fd[0], STDIN_FILENO); //stdin -> pipe (er1)
            close(fd[1]);   
            pipeline_pop_front(apipe);

            //Printeamos en output
            if (pipeline_is_empty(apipe)) { 
                while(read(fd[0], &buf, 1) > 0){
                    write(1, &buf, 1); // 1 -> stdout
                }
                dup2(stdio_fd, STDIN_FILENO); // restauramos el stdio
            }
            close(fd[0]);
            close(fd[1]);
        }   
    }
} 
/* Ejecuta un pipeline haciendo llamadas a diferentes funciones modularizadas.
 * Las funciones son:
 *      -Ejecutar comando interno
 *      -Ejecutar un comando externo
 *      -Ejecutar dos comandos externos, unidos mediante una pipe.
 *      -Ejecutar tres o mas comando externos, unidos mediante multiples pipe.
 *
 * Requires: cmd != NULL && pipeline_length(apipe) > 2
 */
void execute_pipeline(pipeline apipe){
    assert(apipe != NULL);
    if (pipeline_is_empty(apipe)) { 
        printf("Pipeline is empty\n");
        return;
    }
    else if (builtin_alone(apipe)) {    //Ejecuta un comando interno
        builtin_run(pipeline_front(apipe));
    } 
    else {
        if (pipeline_length(apipe) == 1){      //Ejecuta un comando externo 
            execute_one_command(apipe);  
        } 
        else if (pipeline_length(apipe) == 2){ //Ejecuta dos comandos externos 
            execute_two_commands(apipe);
        }
        else {                                 //Ejecuta mas de dos comandos externos 
            execute_multiple_commands(apipe);
        }
    }
}
