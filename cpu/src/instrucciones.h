#ifndef INSTRUCCIONES_H
#define INSTRUCCIONES_H

#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_server.h>
#include <utils/utils_cliente.h>
#include <semaphore.h>
#include <utils/utils.h>
#include <utils/hello.h>
#include "cpu.h"

void execute_set(t_pcb *pcb, char *registro, uint32_t valor);
void execute_sum(t_pcb *pcb, char *registro_destino, char *registro_origen);
void execute_sub(t_pcb *pcb, char *registro_destino, char *registro_origen);
void execute_jnz(t_pcb *pcb, char *registro, uint32_t nuevo_program_counter);
void execute_io_gen_sleep(t_pcb *pcb, char *interfaz, uint32_t unidades_de_trabajo);

#endif /*INSTRUCCIONES_H*/