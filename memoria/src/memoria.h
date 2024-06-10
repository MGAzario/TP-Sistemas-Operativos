#ifndef MEMORIA_H
#define MEMORIA_H

#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_server.h>
#include <utils/utils_cliente.h>
#include <utils/hello.h>
#include <utils/utils.h>
#include <pthread.h>
#include <commons/bitarray.h>
#include <math.h>
#include "kernel_memoria.h"
#include "cpu_memoria.h"

extern t_config *config;
extern t_log *logger;

extern char *path_instrucciones;

extern int socket_memoria;
extern int socket_kernel;
extern int socket_cpu;
extern int socket_entradasalida;

extern pthread_t hilo_cpu;
extern pthread_t hilo_kernel;
extern pthread_t hilo_entradasalida[100];
extern int numero_de_entradasalida;

extern int tamanio_memoria;
extern int tamanio_pagina;

extern int marcos_memoria;

extern int retardo;

extern void *espacio_de_usuario;
extern sem_t mutex_espacio_de_usuario;

extern t_bitarray *marcos_libres;

extern t_list *lista_tablas_de_paginas;

extern t_list *lista_instrucciones_por_proceso;

void crear_logger();
void crear_config();
void inicializar_variables_globales();
void iniciar_servidor_memoria();
void *interfaz(void *socket);
void *leer(int direccion_fisica, int tamanio);
void escribir(int direccion_fisica, int tamanio, void *valor);


#endif /*MEMORIA_H*/