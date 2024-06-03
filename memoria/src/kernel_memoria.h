#ifndef KERNEL_MEMORIA_H
#define KERNEL_MEMORIA_H

#include "memoria.h"
#include "utils_memoria.h"


void *recibir_kernel();
t_instrucciones_de_proceso *leer_archivo_pseudocodigo(t_creacion_proceso *creacion_proceso);
void crear_tabla_de_paginas(t_pcb *pcb);
void liberar_estructuras(int pid);
// void destruir_instruccion(void *elem);


#endif /*KERNEL_MEMORIA_H*/