#ifndef UTILS_HELLO_H_
#define UTILS_HELLO_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<string.h>


extern t_log* logger;

/**
* @fn    decir_hola
* @brief Imprime un saludo al nombre que se pase por parámetro por consola.
*/
void decir_hola(char* quien);

char *string_itoa_hasta_tres_cifras(int numero);
void string_n_append_con_strnlen(char** original, char* string_to_add, int n);

#endif
