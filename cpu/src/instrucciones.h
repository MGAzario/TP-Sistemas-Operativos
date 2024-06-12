#ifndef INSTRUCCIONES_H
#define INSTRUCCIONES_H

#include "cpu.h"


void execute_set(t_pcb *pcb, char *registro, uint32_t valor);
void execute_mov_in(t_pcb *pcb, char *registro_datos, t_list *direcciones);
void execute_mov_out(t_pcb *pcb, t_list *direcciones, char *registro_datos);
void execute_sum(t_pcb *pcb, char *registro_destino, char *registro_origen);
void execute_sub(t_pcb *pcb, char *registro_destino, char *registro_origen);
void execute_jnz(t_pcb *pcb, char *registro, uint32_t nuevo_program_counter);
void execute_resize(t_pcb *pcb, int nuevo_tamanio_del_proceso);
void execute_io_gen_sleep(t_pcb *pcb, char *nombre_interfaz, uint32_t unidades_de_trabajo);
void execute_exit(t_pcb *pcb);
void destruir_direccion(void *elem);


#endif /*INSTRUCCIONES_H*/