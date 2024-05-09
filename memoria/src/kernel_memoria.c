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
                char *primera_linea = list_get(instrucciones->lista_instrucciones, 0);
                log_trace(logger, "Primera linea: %s", primera_linea);
                char *segunda_linea = list_get(instrucciones->lista_instrucciones, 1);
                log_trace(logger, "Primera linea: %s", segunda_linea);
                char *tercera_linea = list_get(instrucciones->lista_instrucciones, 2);
                log_trace(logger, "Primera linea: %s", tercera_linea);
                char *cuarta_linea = list_get(instrucciones->lista_instrucciones, 3);
                log_trace(logger, "Primera linea: %s", cuarta_linea);
                // Prueba fin

                list_add(lista_instrucciones_por_proceso, instrucciones);
                list_destroy_and_destroy_elements(instrucciones->lista_instrucciones, destruir_instruccion);
                free(instrucciones);

                enviar_ok(socket_kernel, CREACION_PROCESO_OK);
                // TODO: Crear tabla de páginas (vacía)
                break;
            case DESCONEXION:
                log_warning(logger, "Se desconectó el Kernel");
                while(1);
                break;
            default:
                log_warning(logger, "Mensaje desconocido del Kernel\n");
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

void destruir_instruccion(void *elem)
{
    char *instruccion = (char *)elem;
    free(instruccion);
}