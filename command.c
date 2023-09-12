#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include "command.h"
#include <glib.h> 
#include <string.h>
#include "strextra.h"

struct scommand_s { // Estructura de la forma [*char](comando), *char(archivo input), *char(archivo output)]
    GSList * comms_array; // Lista de comandos simples 
    char * in; // Archivo de input
    char * out; //Archivo de output 
};

scommand scommand_new(void){
    scommand new = malloc(sizeof(struct scommand_s));
    new -> comms_array = NULL;
    new -> in = NULL;
    new -> out = NULL;
    assert(new != NULL && scommand_is_empty(new) && scommand_get_redir_in(new) == NULL && scommand_get_redir_out(new) == NULL);

    return new; 
}

scommand scommand_destroy(scommand self){
    assert(self != NULL);
    g_slist_free_full(self->comms_array, free);
    free(self->in);
    free(self->out);
    free(self);
    self->in = NULL;
    self->out = NULL;
    self = NULL;
    assert(self == NULL);
    return self;
}

void scommand_push_back(scommand self, char * argument){
    assert(self != NULL && argument != NULL);
    self->comms_array = g_slist_append(self -> comms_array, argument);
    assert(!scommand_is_empty(self));
}

void scommand_pop_front(scommand self){
    assert(self != NULL && !scommand_is_empty(self));
    gpointer aux = g_slist_nth_data(self->comms_array, 0u);
    self->comms_array = g_slist_remove(self -> comms_array, aux);
    free(aux); aux = NULL;
}

void scommand_set_redir_in(scommand self, char * filename){
    assert(self != NULL);
    free(self->in); self->in = NULL;
    self -> in = filename;
}
void scommand_set_redir_out(scommand self, char * filename){
    assert(self != NULL);
    free(self->out); self->out = NULL;
    self -> out = filename;
}


bool scommand_is_empty(const scommand self){
    assert(self != NULL);
    return (self -> comms_array == NULL);
}


unsigned int scommand_length(const scommand self){
    assert(self != NULL);  
    unsigned int size = g_slist_length(self -> comms_array);
    assert ((size == 0 || scommand_is_empty(self)) == scommand_is_empty(self));  //(scommand_length(self)==0) --> scommand_is_empty(self)
    return size;
}


char * scommand_front(const scommand self){
    assert(self!=NULL && !scommand_is_empty(self));
    char * value = NULL;
    value = g_slist_nth_data(self -> comms_array, 0);
    assert(value != NULL);
    return value; 
}


char * scommand_get_redir_in(const scommand self){
    assert(self!=NULL);
    return self -> in; 
}
char * scommand_get_redir_out(const scommand self){
    assert(self!=NULL);
    return self -> out;
}


char * scommand_to_string(const scommand self){
    assert(self != NULL);
    char *result = strdup("");  
    for (unsigned int i = 0; i < scommand_length(self); i++){
        result = strmerge(result, g_slist_nth_data(self->comms_array, i));
        result = strmerge(result, " ");
    }
    if (self -> in != NULL){
        result = strmerge(result, " < ");
        result = strmerge(result, self -> in);
    }
    if (self -> out != NULL){
        result = strmerge(result, " > ");
        result = strmerge(result, self -> out);
    }

    assert(scommand_get_redir_in(self) == NULL || scommand_get_redir_out(self) ==NULL || strlen(result) > 0);
    return result;
}


struct pipeline_s {
    GSList * scomms_array; 
    bool background;
};

pipeline pipeline_new(void) {
    pipeline new = malloc(sizeof (struct pipeline_s));
    new->scomms_array = NULL;
    new->background = true;
    assert(new != NULL && pipeline_is_empty(new) && pipeline_get_wait(new));
    return new;
}

static void scommand_destroy_void(void* self){
    scommand s = self;
    scommand_destroy(s);
}

pipeline pipeline_destroy(pipeline self) {
    assert(self != NULL);
    g_slist_free_full(self->scomms_array, scommand_destroy_void);
    self->scomms_array = NULL;
    free(self);
    self = NULL;
    assert(self == NULL);
    return self;
}


void pipeline_push_back(pipeline self, scommand sc) {
    assert(self != NULL && sc != NULL);
    self->scomms_array = g_slist_append(self -> scomms_array, sc);
    assert(!pipeline_is_empty(self));
}


void pipeline_pop_front(pipeline self){
    assert(self != NULL && !pipeline_is_empty(self));
    gpointer aux = g_slist_nth_data(self->scomms_array, 0);
    self->scomms_array = g_slist_remove(self->scomms_array, aux);
    scommand_destroy(aux); 
}


void pipeline_set_wait(pipeline self, const bool w){
    assert(self != NULL);
    self->background = w;
}


bool pipeline_is_empty(const pipeline self){
    assert(self != NULL);
    bool r_value = (self->scomms_array == NULL); 
    return r_value;
}


unsigned int pipeline_length(const pipeline self){
    assert(self != NULL);
    unsigned int size = g_slist_length(self->scomms_array);
    assert ((size == 0 || pipeline_is_empty(self)) == pipeline_is_empty(self)); //(pipeline_length(self)==0) --> pipeline_is_empty(self)  
    return size;
}


scommand pipeline_front(const pipeline self){
    assert(self!=NULL && !pipeline_is_empty(self));
    scommand value = NULL;
    value = g_slist_nth_data(self->scomms_array, 0);
    assert(value !=NULL);
    return value;
}


bool pipeline_get_wait(const pipeline self){
    assert(self != NULL);
    return self->background;
}


char * pipeline_to_string(const pipeline self){
    assert(self != NULL);
    char *result = strdup("");
    for (unsigned int i = 0; i < pipeline_length(self); i++){
        char *tmp = scommand_to_string(g_slist_nth_data(self->scomms_array, i));
        result = strmerge(result, tmp);
        free(tmp); tmp = NULL;
        if (i < pipeline_length(self) - 1){
            result = strmerge(result, " | ");
        }
    }
    if (!pipeline_get_wait(self)){
        result = strmerge(result, " & ");
    }
    assert((pipeline_is_empty(self) || pipeline_get_wait(self) || strlen(result) > 0));
    return result;
}

