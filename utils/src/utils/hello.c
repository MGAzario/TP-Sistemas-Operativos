#include "hello.h"

void decir_hola(char* quien) {
    log_info(logger, "\nHola desde %s!!\n", quien);
}

// Actualmente esto solo sirve para que quede mejor formateado PROCESO_ESTADO, y no tiene otra utilidad
char *string_itoa_hasta_tres_cifras(int numero)
{
    char *cadena = string_new();

    if (numero < 10)
    {
        string_append(&cadena, "  ");
        string_append(&cadena, string_itoa(numero));
    }
    else if (numero < 100)
    {
        string_append(&cadena, " ");
        string_append(&cadena, string_itoa(numero));
    }
    else
    {
        string_append(&cadena, string_itoa(numero));
    }

    return cadena;
}
