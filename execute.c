
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

static void execute_extern(scommand cmd){
    unsigned int length = scommand_length(cmd);
    char **args = calloc(length + 1, sizeof(char*));
    for(unsigned int i = 0; i < length; ++i){
        args[i] = scommand_front(cmd); 
        scommand_pop_front(cmd);
    }
    char *in = scommand_get_redir_in(cmd);
    if (in != NULL) {
        int fd = open(in , O_RDONLY, 0666);
        dup2(fd , 0);
        close(fd);
    }
    char *out = scommand_get_redir_out(cmd);
    if (out != NULL) {
        int fd = open(out , O_CREAT | O_WRONLY, 0666);
        dup2(fd , 1);
        close(fd);
    }
    execvp(args[0], args);
    fprintf(stderr, "%s: command not found.\n", args[0]);
    exit(EXIT_FAILURE);
}

void execute_pipeline(pipeline apipe){
    if (pipeline_is_empty(apipe)) {
        printf("No ingreso ningun comando\n");
        return;
    }
    else if (builtin_alone(apipe)) {
        builtin_run(pipeline_front(apipe));
    } 
    else {
        if(pipeline_length(apipe) > 1){
            int stdio_fd = dup(STDIN_FILENO); // Descriptor de archivo del stdio.
            while(!pipeline_is_empty(apipe)) {    
                int fd[2], cpid;
                char *buf; //se almacena los datos para printear en stdout
                if (pipe(fd) == -1) {
                    fprintf(stderr, "pipe() has failed.\n");
                    exit(EXIT_FAILURE);
                }
                cpid = fork();
                if (cpid < 0){
                    fprintf(stderr, "fork() has failed.\n");    
                    exit(EXIT_FAILURE);
                }
                else if (cpid == 0){   //child
                    dup2(fd[1], 1); // redirect stdout -> pipe (er2) -----> ACA ESTA EL PROBLEMA SE TRABA EN dup2(fd[1], 1) para el segundo comando del pipe

                    execute_extern(pipeline_front(apipe));
                }
                else{ //parent
                    if (!pipeline_get_wait(apipe)){  //MODIFIQUE ESTO QUE FUNCIONABA AL REVES
                        wait(NULL); //Parent waits for the child to terminate
                    }
                    dup2(fd[0], 0); // redirect stdin -> pipe (er1)
                    close(fd[1]);   //close unused write end
                    pipeline_pop_front(apipe);
                    
                    //Printeamos en output
                    if (pipeline_is_empty(apipe)) { 
                        while(read(fd[0], &buf, 1) > 0){
                            write(1, &buf, 1); // 1 -> stdout
                        }
                        dup2(stdio_fd, 0); // restauramos el stdio
                    }
                }   
            } 
        }
        else{
            int cpid2 = fork();
            if (cpid2 < 0){
                fprintf(stderr, "fork() has failed.\n");    
                exit(EXIT_FAILURE);
            }
            else if (cpid2 == 0) {
                execute_extern(pipeline_front(apipe));
            }
            else {
                if (pipeline_get_wait(apipe)){
                    wait(NULL); // Padre espera al hijo para que termine
                }
                pipeline_pop_front(apipe);
            }   
        }
    }
}

