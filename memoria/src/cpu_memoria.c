#include "cpu_memoria.h"

void *recibir_cpu()
{
    while(1)
    {
        log_trace(logger, "Esperando mensaje del CPU");
        op_code cod_op = recibir_operacion(socket_cpu);

        switch (cod_op)
        {
            case SOLICITUD_INSTRUCCION:
                log_trace(logger, "Recibí una solicitud del CPU para entregarle la siguiente instrucción");
                t_pcb *pcb = recibir_pcb(socket_cpu);
                char *instruccion = buscar_instruccion(pcb->pid, pcb->cpu_registers->pc);
                sleep(1); // TODO: Reemplazar con configuración
                enviar_instruccion(socket_cpu, instruccion);
                break;
            case RESIZE:
                log_trace(logger, "Recibí una solicitud del CPU para ajustar el tamaño de un proceso");
                t_resize *resize = recibir_resize(socket_cpu);
                if(ajustar_tamanio(resize->pcb->pid, resize->tamanio))
                {
                    enviar_mensaje_simple(socket_cpu, RESIZE_EXITOSO);
                }
                else
                {
                    enviar_mensaje_simple(socket_cpu, OUT_OF_MEMORY);
                }
                break;
            case DESCONEXION:
                log_warning(logger, "Se desconectó el CPU");
                while(1);
                break;
            default:
                log_warning(logger, "Mensaje desconocido del CPU");
                while(1);
                break;
        }
    }
}

char *buscar_instruccion(int pid, uint32_t program_counter)
{
    for (int i = 0; i < list_size(lista_instrucciones_por_proceso); i++)
    {
        t_instrucciones_de_proceso *instrucciones = list_get(lista_instrucciones_por_proceso, i);
        log_debug(logger, "Buscando la instrucción del proceso %i", pid);

        if (instrucciones->pid == pid)
        {
            return list_get(instrucciones->lista_instrucciones, program_counter);
        }
    }

    log_error(logger, "No se encontró el proceso en la lista de instrucciones por proceso de la memoria");
    return NULL;
}

bool ajustar_tamanio(int pid, int tamanio_proceso)
{
    t_tabla_de_paginas *tabla_de_paginas_del_proceso = buscar_tabla_de_paginas(pid);

    int tamanio_tabla = list_size(tabla_de_paginas_del_proceso->lista_marcos);
    int cantidad_de_marcos_necesaria = tamanio_proceso / tamanio_pagina;

    // Si queremos ampliar el tamaño (o la tabla esta vacía):
    if(tamanio_tabla < cantidad_de_marcos_necesaria)
    {
        log_info(logger, "PID: %i - Tamaño Actual: %i - Tamaño a Ampliar: %i", pid, tamanio_tabla, tamanio_proceso);
        int marcos_adicionales = cantidad_de_marcos_necesaria - tamanio_tabla;
        log_trace(logger, "Se van a agregar %i entradas a la tabla", marcos_adicionales);
        for(int i = 0; i < marcos_adicionales; i++)
        {
            int marco_libre = buscar_marco_libre();
            if(marco_libre == -1)
            {
                return MEMORIA_LLENA;
            }
            bitarray_set_bit(marcos_libres, marco_libre);
            int *marco = malloc(sizeof(int));
            *marco = marco_libre;
            list_add(tabla_de_paginas_del_proceso->lista_marcos, marco);
            estado_del_bitarray();
        }
        estado_tabla_de_paginas(tabla_de_paginas_del_proceso);
        return TAMANIO_AJUSTADO_EXITOSAMENTE;
    }

    // Si queremos reducir el tamaño:
    if(tamanio_tabla > cantidad_de_marcos_necesaria)
    {
        log_info(logger, "PID: %i - Tamaño Actual: %i - Tamaño a Reducir: %i", pid, tamanio_tabla, tamanio_proceso);
        int marcos_a_remover = tamanio_tabla - cantidad_de_marcos_necesaria;
        log_trace(logger, "Se van a eliminar %i entradas de la tabla", marcos_a_remover);
        for(int i = 0; i < marcos_a_remover; i++)
        {
            liberar_ultimo_marco(tabla_de_paginas_del_proceso);
            estado_del_bitarray();
        }
        estado_tabla_de_paginas(tabla_de_paginas_del_proceso);
        return TAMANIO_AJUSTADO_EXITOSAMENTE;
    }

    // Si es el mismo tamaño:
    if(tamanio_tabla == cantidad_de_marcos_necesaria)
    {
        log_trace(logger, "El tamaño pedido para el RESIZE es el mismo tamaño que ya tiene el proceso");
        return TAMANIO_AJUSTADO_EXITOSAMENTE;
    }

    log_error(logger, "La memoria no supo que hacer ante el pedido de RESIZE");
    return true;
}

int buscar_marco_libre()
{
    for(int i = 0; i < marcos_memoria; i++)
    {
        if(bitarray_test_bit(marcos_libres, i) == 0)
        {
            return i;
        }
    }
    return -1;
}