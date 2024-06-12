#ifndef CPU_MEMORIA_H
#define CPU_MEMORIA_H

#include "memoria.h"
#include "utils_memoria.h"

#define TAMANIO_AJUSTADO_EXITOSAMENTE true
#define MEMORIA_LLENA false


void *recibir_cpu();
char *buscar_instruccion(int pid, uint32_t program_counter);
bool ajustar_tamanio(int pid, int tamanio_proceso);
int buscar_marco_libre();
int obtener_marco(int pagina, int pid);


#endif /*CPU_MEMORIA_H*/