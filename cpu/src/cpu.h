#ifndef CPU_H
#define CPU_H
#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_server.h>
#include <utils/utils_cliente.h>
#include <semaphore.h>
#include <utils/registros.h>
#include <utils/estados.h>
#include <utils/hello.h>

void procedimiento_de_prueba();
void crear_logger();
void crear_config();
void iniciar_servidores();
void conectar_memoria();
void recibir_kernel_dispatch();
void recibir_kernel_interrupt();

#endif /*CPU_H*/