#ifndef KERNEL_H
#define KERNEL_H
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
// Estas variables las cargo como globales porque las uso en varias funciones, no se si a nivel codigo es lo correcto.
t_config *config;
t_log *logger;
t_queue *cola_new;
t_queue *cola_ready;


char *ip_cpu;
int socket_cpu_dispatch;
int grado_multiprogramacion_activo;
int grado_multiprogramacion_max;

sem_t sem_nuevo_pcb;
sem_t sem_proceso_liberado;

void crear_logger();
void crear_config();
void conectar_memoria();
void conectar_interrupt_cpu(char *ip_cpu);
void recibir_entradasalida();
void crear_pcb(int quantum);
void mover_procesos_ready();
void planificador_corto_plazo();
void planificador_largo_plazo();
void planificar_fifo();
void planificar_round_robin();
void planificar_vrr();


#endif /*KERNEL_H*/