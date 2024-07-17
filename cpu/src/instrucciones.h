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
void execute_copy_string(t_pcb *pcb, int tamanio_texto);
void execute_wait(t_pcb *pcb, char *nombre_recurso);
void execute_signal(t_pcb *pcb, char *nombre_recurso);
void execute_io_gen_sleep(t_pcb *pcb, char *nombre_interfaz, uint32_t unidades_de_trabajo);
void execute_exit(t_pcb *pcb);
void execute_io_stdout_write(t_pcb *pcb, char *interfaz, t_list *direcciones_fisicas, uint32_t tamanio);
void execute_io_stdin_read(t_pcb *pcb, char *interfaz, t_list *direcciones_fisicas, uint32_t tamanio);
void destruir_direccion(void *elem);
void execute_io_fs_create(t_pcb *pcb, char *interfaz, char *nombre_archivo);
void execute_io_fs_delete(t_pcb *pcb, char *interfaz, char *nombre_archivo);
void execute_io_fs_truncate(t_pcb *pcb, char *interfaz, char *nombre_archivo, uint32_t nuevo_tamano);
void execute_io_fs_write(t_pcb *pcb, char *interfaz, char *nombre_archivo, t_list *direcciones_fisicas, uint32_t tamanio, uint32_t puntero_archivo);
void execute_io_fs_read(t_pcb *pcb, char *interfaz, char *nombre_archivo, t_list *direcciones_fisicas, uint32_t tamanio, uint32_t puntero_archivo);

#endif /*INSTRUCCIONES_H*/