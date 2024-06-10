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
void eliminar_proceso(t_pcb *pcb);
void consola();
void planificar_round_robin();
void planificar_vrr();
void quantum_count(int* quantum);

#endif /*KERNEL_H*/