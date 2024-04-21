#ifndef UTILS_HELLO_H_
#define UTILS_HELLO_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>


extern t_log* logger;

/**
* @fn    decir_hola
* @brief Imprime un saludo al nombre que se pase por parámetro por consola.
*/
void decir_hola(char* quien);

#endif
