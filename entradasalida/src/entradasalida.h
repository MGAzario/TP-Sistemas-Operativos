#ifndef ENTRADASALIDA_H
#define ENTRADASALIDA_H

#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_cliente.h>
#include <utils/utils_server.h>
#include <utils/hello.h>
#include <utils/utils.h>


void crear_logger();
void create_config();
void conectar_kernel();
void conectar_memoria();
void crear_interfaz_generica();
void crear_interfaz_stdin();
void crear_interfaz_stdout();



#endif /*ENTRADASALIDA_H*/