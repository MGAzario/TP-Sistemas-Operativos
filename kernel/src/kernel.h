#ifndef KERNEL_H
#define KERNEL_H

#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_cliente.h>
#include <utils/utils_server.h>
#include <utils/hello.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <pthread.h>
#include <utils/registros.h>
#include <utils/estados.h>


void procedimiento_de_prueba();
void crear_logger();
void crear_config();
void conectar_memoria();
void conectar_dispatch_cpu(char *ip_cpu);
void conectar_interrupt_cpu(char *ip_cpu);
void recibir_entradasalida();
void crear_pcb();
void *planificador_corto_plazo();
void *planificador_largo_plazo();
void planificar_fifo();
void planificar_round_robin();
void planificar_vrr();
void esperar_cpu();
void consola();


#endif /*KERNEL_H*/