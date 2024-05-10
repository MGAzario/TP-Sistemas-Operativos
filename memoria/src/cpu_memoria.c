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
            case DESCONEXION:
                log_warning(logger, "Se desconectó el CPU");
                while(1);
                break;
            default:
                log_warning(logger, "Mensaje desconocido del CPU");
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