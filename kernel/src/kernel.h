#ifndef KERNEL_H
#define KERNEL_H

#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_cliente.h>
#include <utils/utils_server.h>
#include <utils/hello.h>
#include <readline/readline.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/collections/dictionary.h>
#include <commons/temporal.h>
#include <pthread.h>
#include <utils/utils.h>


void crear_logger();
void crear_config();
void conectar_memoria();
void conectar_dispatch_cpu(char *ip_cpu);
void conectar_interrupt_cpu(char *ip_cpu);
void *recibir_entradasalida();
void cargar_interfaz_recibida(t_interfaz *interfaz, int socket_entradasalida, char *nombre, tipo_interfaz tipo);
void *interfaz_generica(void *interfaz_sleep);
void crear_pcb(char *path);
void encontrar_y_eliminar_proceso(int pid_a_eliminar);
void *planificador_corto_plazo();
void *planificador_largo_plazo();
void planificar_fifo();
void planificar_round_robin();
void planificar_vrr();
void esperar_cpu();
void bloquear_proceso(t_pcb *pcb);
void desbloquear_proceso(t_pcb *pcb);
void eliminar_proceso(t_pcb *pcb);
void consola();
void script(char *path);
void detener_planificacion();
void iniciar_planificacion();
void comprobar_planificacion();
void modificar_grado_multiprogramacion(int nuevo_valor);
void procesos_por_estado();
void listar_procesos(int cantidad_filas);
void planificar_round_robin();
void planificar_vrr();
void quantum_count();
void quantum_block();
void crear_diccionario();
char *recibir_wait(); //a diseñar
char *recibir_signal(); //a diseñar
void desbloquear_proceso_io(t_interfaz *interfaz);
bool fin_sleep(t_interfaz *interfaz);
bool fin_io_read(t_interfaz *interfaz);
bool fin_io_write(t_interfaz *interfaz);
void cargar_interfaz_recibida(t_interfaz *interfaz, int socket_entradasalida, char *nombre, tipo_interfaz tipo);
void *manejo_interfaces(void *interfaz_hilo);
void pedido_io_stdin_read();
void pedido_io_stdout_write();
void pedido_io_stdin_read();
void pedido_io_stdout_write();
void pedido_io_fs_read();
void pedido_io_fs_write();
void pedido_io_fs_truncate();
void pedido_io_fs_delete();
void pedido_io_fs_create();
bool fin_io_fs(t_interfaz *interfaz);

#endif /*KERNEL_H*/