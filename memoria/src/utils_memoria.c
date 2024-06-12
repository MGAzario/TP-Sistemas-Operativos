#include "utils_memoria.h"

t_tabla_de_paginas *buscar_tabla_de_paginas(int pid)
{
    for (int i = 0; i < list_size(lista_tablas_de_paginas); i++)
    {
        t_tabla_de_paginas *tabla_de_paginas = list_get(lista_tablas_de_paginas, i);
        log_debug(logger, "Buscando la tabla de páginas del proceso %i", pid);

        if (tabla_de_paginas->pid == pid)
        {
            return tabla_de_paginas;
        }
    }
    log_error(logger, "El proceso no existe");
    return NULL;
}

void liberar_ultimo_marco(t_tabla_de_paginas *tabla_de_paginas)
{
    int *marco_liberado = (int *) list_remove(tabla_de_paginas->lista_marcos, list_size(tabla_de_paginas->lista_marcos) - 1);
    bitarray_clean_bit(marcos_libres, *marco_liberado);
    free(marco_liberado);
}


/*-----------------------------FUNCIONES PARA TESTEAR-------------------------------------------------------*/

void estado_del_bitarray()
{
    if((long) bitarray_get_max_bit(marcos_libres) == 8)
    {
    log_trace(logger, "Bitarray de marcos libres: %i%i%i%i%i%i%i%i",
        (int) bitarray_test_bit(marcos_libres, 0),
        (int) bitarray_test_bit(marcos_libres, 1),
        (int) bitarray_test_bit(marcos_libres, 2),
        (int) bitarray_test_bit(marcos_libres, 3),
        (int) bitarray_test_bit(marcos_libres, 4),
        (int) bitarray_test_bit(marcos_libres, 5),
        (int) bitarray_test_bit(marcos_libres, 6),
        (int) bitarray_test_bit(marcos_libres, 7));
    }
    else if((long) bitarray_get_max_bit(marcos_libres) == 16)
    {
    log_trace(logger, "Bitarray de marcos libres: %i%i%i%i%i%i%i%i%i%i%i%i%i%i%i%i",
        (int) bitarray_test_bit(marcos_libres, 0),
        (int) bitarray_test_bit(marcos_libres, 1),
        (int) bitarray_test_bit(marcos_libres, 2),
        (int) bitarray_test_bit(marcos_libres, 3),
        (int) bitarray_test_bit(marcos_libres, 4),
        (int) bitarray_test_bit(marcos_libres, 5),
        (int) bitarray_test_bit(marcos_libres, 6),
        (int) bitarray_test_bit(marcos_libres, 7),
        (int) bitarray_test_bit(marcos_libres, 8),
        (int) bitarray_test_bit(marcos_libres, 9),
        (int) bitarray_test_bit(marcos_libres, 10),
        (int) bitarray_test_bit(marcos_libres, 11),
        (int) bitarray_test_bit(marcos_libres, 12),
        (int) bitarray_test_bit(marcos_libres, 13),
        (int) bitarray_test_bit(marcos_libres, 14),
        (int) bitarray_test_bit(marcos_libres, 15));
    }
    else
    {
    log_trace(logger, "Primeros dos bytes del bitarray de marcos libres de tamaño %i: %i%i%i%i%i%i%i%i%i%i%i%i%i%i%i%i",
        (int) bitarray_get_max_bit(marcos_libres),
        (int) bitarray_test_bit(marcos_libres, 0),
        (int) bitarray_test_bit(marcos_libres, 1),
        (int) bitarray_test_bit(marcos_libres, 2),
        (int) bitarray_test_bit(marcos_libres, 3),
        (int) bitarray_test_bit(marcos_libres, 4),
        (int) bitarray_test_bit(marcos_libres, 5),
        (int) bitarray_test_bit(marcos_libres, 6),
        (int) bitarray_test_bit(marcos_libres, 7),
        (int) bitarray_test_bit(marcos_libres, 8),
        (int) bitarray_test_bit(marcos_libres, 9),
        (int) bitarray_test_bit(marcos_libres, 10),
        (int) bitarray_test_bit(marcos_libres, 11),
        (int) bitarray_test_bit(marcos_libres, 12),
        (int) bitarray_test_bit(marcos_libres, 13),
        (int) bitarray_test_bit(marcos_libres, 14),
        (int) bitarray_test_bit(marcos_libres, 15));
    }
}

void estado_tabla_de_paginas(t_tabla_de_paginas *tabla_de_paginas)
{
    if(list_size(tabla_de_paginas->lista_marcos) <= 100)
    {
        log_trace(logger, "Las %i entradas de la tabla de páginas del proceso %i:", list_size(tabla_de_paginas->lista_marcos), tabla_de_paginas->pid);
        log_trace(logger, "Página | Marco");
        for (int i = 0; i < list_size(tabla_de_paginas->lista_marcos); i++)
        {
            if(i < 10)
            {
                log_trace(logger, "%i      | %i    ", i, *(int*) list_get(tabla_de_paginas->lista_marcos, i));
            }
            else
            {
                log_trace(logger, "%i     | %i    ", i, *(int*) list_get(tabla_de_paginas->lista_marcos, i));
            }
        }
    }
    else
    {
        log_trace(logger, "Primeras 100 entradas de %i de la tabla de páginas del proceso %i:", list_size(tabla_de_paginas->lista_marcos), tabla_de_paginas->pid);
        log_trace(logger, "Página | Marco");
        for (int i = 0; i < 99; i++)
        {
            if(i < 10)
            {
                log_trace(logger, "%i      | %i    ", i, *(int*) list_get(tabla_de_paginas->lista_marcos, i));
            }
            else
            {
                log_trace(logger, "%i     | %i    ", i, *(int*) list_get(tabla_de_paginas->lista_marcos, i));
            }
        }
    }
}