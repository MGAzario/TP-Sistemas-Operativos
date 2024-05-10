#ifndef MEMORIA_H
#define MEMORIA_H

#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_server.h>
#include <utils/utils_cliente.h>
#include <utils/hello.h>
#include <pthread.h>
#include "kernel_memoria.h"
#include "cpu_memoria.h"

extern t_config *config;
extern t_log *logger;

extern char *path_instrucciones;

extern int socket_memoria;
extern int socket_kernel;
extern int socket_cpu;
extern int socket_entradasalida;

extern pthread_t hilo_kernel;
extern pthread_t hilo_cpu;

extern t_list *lista_instrucciones_por_proceso;

void crear_logger();
void crear_config();
void iniciar_servidor_memoria();


#endif /*MEMORIA_H*/