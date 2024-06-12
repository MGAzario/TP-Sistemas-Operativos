#ifndef CPU_H
#define CPU_H

#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_server.h>
#include <utils/utils_cliente.h>
#include <semaphore.h>
#include <utils/utils.h>
#include <utils/hello.h>
#include <pthread.h>
#include <math.h>
#include "instrucciones.h"

extern t_config *config;
extern t_log *logger;

extern char *ip_memoria;
extern int socket_cpu_dispatch;
extern int socket_cpu_interrupt;
extern int socket_kernel_dispatch;
extern int socket_kernel_interrupt;
extern int socket_memoria;

extern int tamanio_pagina;

extern int continuar_ciclo;
extern int pid_de_interrupcion;
extern motivo_interrupcion motivo_de_interrupcion;

extern pthread_t hilo_interrupciones;


void crear_logger();
void crear_config();
void iniciar_servidores();
void conectar_memoria();
void recibir_procesos_kernel(int socket_cliente);
void *recibir_interrupciones();
void ciclo_de_instruccion(t_pcb *pcb);
char *fetch(t_pcb *pcb);
void decode(t_pcb *pcb, char *instruccion);
void check_interrupt(t_pcb *pcb);
void preguntar_tamanio_pagina();
uint32_t leer_registro(t_pcb *pcb, char *registro);
void escribir_registro(t_pcb *pcb, char *registro, uint32_t valor);
int tamanio_registro(char *registro);
t_list *mmu(uint32_t direccion_logica, int tamanio_del_contenido, int pid);
int averiguar_marco(int pid, int pagina);


#endif /*CPU_H*/