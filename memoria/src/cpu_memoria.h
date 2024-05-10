#ifndef CPU_MEMORIA_H
#define CPU_MEMORIA_H

#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_server.h>
#include <utils/utils_cliente.h>
#include <utils/hello.h>
#include "memoria.h"


void *recibir_cpu();
char *buscar_instruccion(int pid, uint32_t program_counter);


#endif /*CPU_MEMORIA_H*/