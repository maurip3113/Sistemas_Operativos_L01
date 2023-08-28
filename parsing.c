#include <stdlib.h>
#include <stdbool.h>

#include "parsing.h"
#include "parser.h"
#include "command.h"

static scommand parse_scommand(Parser p) {

    scommand cmd = scommand_new(); 
    arg_kind_t type;                          //para checkear si es command, input o output (argumento de parse_next_argument)
    char *arg;                               //se guarda el argumento/comando
    bool finished = false;
    while (!finished) 
    {
        parser_skip_blanks(p); 
        arg = parser_next_argument(p,&type); //guarda el tipo de argumento en arg
        
        if (arg == NULL)                    // ESTO SUPONIENDO QUE parser_next_argument DEVUELVE NULL CUANDO SE ENCUENTRA CON UN PIPELINE O UN /N
        {
            finished = true;
        } 
        else {  
            if (type == ARG_NORMAL){
                scommand_push_back(cmd,arg);                       //agregamos a cmd ese comando / argumento de comando 
            } 
            else if (type== ARG_INPUT){
                if (!scommand_is_empty(cmd)){ 
                    scommand_set_redir_in(cmd, arg);       //define la redireccion de entrada
                } else {                                          
                    scommand_destroy(cmd);                  //Hay error de parseo (El primer argumento debe ser un comando)
                    return NULL;
                }
            }
            else if (type == ARG_OUTPUT){
                if (!scommand_is_empty(cmd)){ 
                    scommand_set_redir_out(cmd, arg);      //define la redireccion de salida
                } else { 
                    scommand_destroy(cmd);                  //Hay error de parseo (El primer argumento debe ser un comando)
                    return NULL;
            }}
        }
    }
    return cmd;
}


pipeline parse_pipeline(Parser p) {
    pipeline result = pipeline_new();
    scommand cmd = NULL;
    bool error = false, another_pipe=true;
    bool wait, garbage;

    cmd = parse_scommand(p);
    error = (cmd==NULL); /* Comando inválido al empezar */
    if(!error) {
        pipeline_push_back(result,cmd);
    }
    else {
        pipeline_destroy(result);
    }

    parser_op_pipe(p,&another_pipe);

    while (another_pipe && !error) {
        cmd = parse_scommand(p);
        error = (cmd==NULL);
        if(!error) {
            pipeline_push_back(result,cmd);
        }
        parser_op_pipe(p,&another_pipe);
    }
    parser_op_background(p,&wait);
    pipeline_set_wait(result,!wait);

    parser_garbage(p,&garbage);     // Consume todos los espacios y el /n, notifica si encontro basura

    if(error || garbage){ // Si hubo basura o el parseo falla, se elimina el pipeline.
        pipeline_destroy(result);
    }
    return result;
}