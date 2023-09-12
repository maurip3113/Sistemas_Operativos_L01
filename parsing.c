#include <stdlib.h>
#include <stdbool.h>

#include <assert.h>
#include "parsing.h"
#include "parser.h"
#include "command.h"

static scommand parse_scommand(Parser p) {

    scommand cmd = scommand_new(); 
    arg_kind_t type;                          //para checkear si es command, input o output (argumento de parse_next_argument)
    char *arg;                               //se guarda el argumento/comando
    bool finished = false;
    while (!finished) {
        parser_skip_blanks(p); 
        arg = parser_next_argument(p,&type); //guarda el tipo de argumento en arg
        
        if (arg == NULL){                    
            finished = true;
        } 
        else {  
            if (type == ARG_NORMAL){
                scommand_push_back(cmd,arg);                //agregamos a cmd ese comando / argumento de comando 
            } 
            else if (type== ARG_INPUT){
                if (!scommand_is_empty(cmd)){  
                    scommand_set_redir_in(cmd, arg);       //define la redireccion de entrada
                } else {                                          
                    scommand_destroy(cmd);                 //ERROR DE PARSEO(El primer argumento debe ser un comando)

                }
            } 
            else if (type == ARG_OUTPUT){
                if (!scommand_is_empty(cmd)){ 
                    scommand_set_redir_out(cmd, arg);      //define la redireccion de salida
                } else { 
                    scommand_destroy(cmd);                 
                    
                }
            }
        }
    }
    return cmd;
}


pipeline parse_pipeline(Parser p) {  //Modifique el ciclo principal para eliminar if que sobraban.
    assert(p != NULL && !parser_at_eof(p));

    pipeline result = pipeline_new();
    scommand cmd = NULL;
    bool error = false, another_pipe=true;
    bool wait, garbage;

    while (another_pipe && !error) {
        cmd = parse_scommand(p);
        pipeline_push_back(result, cmd);         //Agregamos el commando al pipe
        parser_op_pipe(p, &another_pipe);         //Vemos si existe algun pipe (|)
        error = (scommand_is_empty(cmd)); 
    }
    parser_op_background(p,&wait);
    pipeline_set_wait(result,!wait);
    
    parser_garbage(p,&garbage);      //Consume todos los espacios y el /n, notifica si encontro basura

    if (error || garbage) {       //Si hubo basura o el parseo falla, se elimina el pipeline.
        pipeline_destroy(result);
        result = NULL;
    } 
    
    return result; 
}