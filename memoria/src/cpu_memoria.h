#ifndef CPU_MEMORIA_H
#define CPU_MEMORIA_H

#include "memoria.h"


void *recibir_cpu();
char *buscar_instruccion(int pid, uint32_t program_counter);


#endif /*CPU_MEMORIA_H*/