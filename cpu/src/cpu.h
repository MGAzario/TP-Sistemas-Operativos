#ifndef CPU_H
#define CPU_H
#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_server.h>
#include <utils/utils_cliente.h>
#include <semaphore.h>
#include <utils/utils.h>
#include <utils/hello.h>


void crear_logger();
void crear_config();
void iniciar_servidores();
void conectar_memoria();
void recibir_procesos_kernel(int socket_cliente);


#endif /*CPU_H*/