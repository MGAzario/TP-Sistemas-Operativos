#ifndef CPU_H
#define CPU_H

#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_server.h>
#include <utils/utils_cliente.h>
#include <semaphore.h>
#include <utils/utils.h>
#include <utils/hello.h>
#include "instrucciones.h"


void crear_logger();
void crear_config();
void iniciar_servidores();
void conectar_memoria();
void recibir_procesos_kernel(int socket_cliente);
void ciclo_de_instruccion(t_pcb *pcb);
char *fetch(t_pcb *pcb);
void decode(t_pcb *pcb, char *instruccion);
void check_interrupt();


#endif /*CPU_H*/