#include "kernel_memoria.h"

void *recibir_kernel()
{
    while(1)
    {
        log_trace(logger, "Esperando mensaje del Kernel");
        op_code cod_op = recibir_operacion(socket_kernel);

        switch (cod_op)
        {
            case CREACION_PROCESO:
                log_trace(logger, "Recibí una solicitud del kernel para inicializar estructuras de un proceso");
                t_creacion_proceso *creacion_proceso = recibir_creacion_proceso(socket_kernel);
                t_instrucciones_de_proceso *instrucciones;
                instrucciones = leer_archivo_pseudocodigo(creacion_proceso);

                // Prueba inicio
                // char *primera_linea = list_get(instrucciones->lista_instrucciones, 0);
                // log_trace(logger, "Primera linea: %s", primera_linea);
                // char *segunda_linea = list_get(instrucciones->lista_instrucciones, 1);
                // log_trace(logger, "Segunda linea: %s", segunda_linea);
                // char *tercera_linea = list_get(instrucciones->lista_instrucciones, 2);
                // log_trace(logger, "Tercera linea: %s", tercera_linea);
                // char *cuarta_linea = list_get(instrucciones->lista_instrucciones, 3);
                // log_trace(logger, "Cuarta linea: %s", cuarta_linea);
                // char *quinta_linea = list_get(instrucciones->lista_instrucciones, 4);
                // log_trace(logger, "Quinta linea: %s", quinta_linea);
                // Prueba fin

                list_add(lista_instrucciones_por_proceso, instrucciones);
                // list_destroy_and_destroy_elements(instrucciones->lista_instrucciones, destruir_instruccion);
                // free(instrucciones);

                crear_tabla_de_paginas(creacion_proceso->pcb);

                enviar_mensaje_simple(socket_kernel, CREACION_PROCESO_OK);
                break;
            case FINALIZACION_PROCESO:
                log_trace(logger, "Recibí una solicitud del kernel para liberar las estructuras de un proceso");
                t_pcb *pcb = recibir_pcb(socket_kernel);
                liberar_estructuras(pcb->pid);
                enviar_mensaje_simple(socket_kernel, FINALIZACION_PROCESO_OK);
                break;
            case DESCONEXION:
                log_warning(logger, "Se desconectó el Kernel");
                while(1);
                break;
            default:
                log_warning(logger, "Mensaje desconocido del Kernel");
                break;
        }
    }
}

t_instrucciones_de_proceso* leer_archivo_pseudocodigo(t_creacion_proceso* creacion_proceso)
{
    t_instrucciones_de_proceso *instrucciones = malloc(sizeof(t_instrucciones_de_proceso));

    instrucciones->pid = creacion_proceso->pcb->pid;
    instrucciones->lista_instrucciones = list_create();

    char *path = string_new();
    string_append(&path, path_instrucciones);
    string_append(&path, "/");
    string_append(&path, creacion_proceso->path);

    char *linea = NULL;
    size_t size = 0;
    FILE *archivo = fopen(path, "r");

    if (!archivo)
    {
        log_error(logger, "No se pudo abrir el archivo de pseudocódigo: %s", path);
        exit(EXIT_FAILURE);
    }

    free(path);

    while (getline(&linea, &size, archivo) != -1)
    {
        if (strcmp(linea, "EXIT") == 0)
        {
            int tamanio = strlen(linea) + 1;

            char *linea_final = malloc(tamanio);
            strcpy(linea_final, linea);

            list_add(instrucciones->lista_instrucciones, linea_final);
        }
        else
        {
            int tamanio = strlen(linea);
            linea[tamanio - 1] = '\0';

            char *linea_final = malloc(tamanio);
            strcpy(linea_final, linea);

            list_add(instrucciones->lista_instrucciones, linea_final);
        }
    }

    free(linea);
    fclose(archivo);

    return instrucciones;
}

void crear_tabla_de_paginas(t_pcb *pcb)
{
    t_tabla_de_paginas *tabla_de_paginas = malloc(sizeof(t_tabla_de_paginas));
    tabla_de_paginas->pid = pcb->pid;
    tabla_de_paginas->lista_marcos = list_create();
    list_add(lista_tablas_de_paginas, tabla_de_paginas);
    log_info(logger, "PID: %i - Tamaño: %i", pcb->pid, 0);
}

void liberar_estructuras(int pid)
{
    t_tabla_de_paginas *tabla_de_paginas = buscar_tabla_de_paginas(pid);
    int tamanio_tabla = list_size(tabla_de_paginas->lista_marcos);

    // Marcar como libres todos los marcos
    for (int i = 0; i < tamanio_tabla; i++)
    {
        liberar_ultimo_marco(tabla_de_paginas);
    }
    if(!list_is_empty(tabla_de_paginas->lista_marcos))
    {
        log_error(logger, "No se liberaron correctamente todos los marcos");
    }

    // Elminar de lista de tablas de páginas
    if(!list_remove_element(lista_tablas_de_paginas, tabla_de_paginas))
    {
        log_error(logger, "No se eliminó correctamente de la lista de tablas de páginas");
    }
    list_destroy(tabla_de_paginas->lista_marcos);
    free(tabla_de_paginas);

    // Eliminar de lista de instrucciones por proceso
    for (int i = 0; i < list_size(lista_instrucciones_por_proceso); i++)
    {
        t_instrucciones_de_proceso *instrucciones = list_get(lista_instrucciones_por_proceso, i);

        if (instrucciones->pid == pid)
        {
            if(!list_remove_element(lista_instrucciones_por_proceso, instrucciones))
            {
                log_error(logger, "No se eliminó correctamente de la lista de instrucciones por proceso");
            }
            list_destroy(instrucciones->lista_instrucciones);
            free(instrucciones);
        }
    }
}

// void destruir_instruccion(void *elem)
// {
//     char *instruccion = (char *)elem;
//     free(instruccion);
// }