#ifndef KERNEL_MEMORIA_H
#define KERNEL_MEMORIA_H

#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_server.h>
#include <utils/utils_cliente.h>
#include <utils/hello.h>
#include "memoria.h"


void *recibir_kernel();
t_instrucciones_de_proceso *leer_archivo_pseudocodigo(t_creacion_proceso *creacion_proceso);
void destruir_instruccion(void *elem);


#endif /*KERNEL_MEMORIA_H*/