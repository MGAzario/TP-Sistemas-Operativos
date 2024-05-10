#include "instrucciones.h"

void execute_set(t_pcb *pcb, char *registro, uint32_t valor)
{
    log_error(logger, "Falta implementar SET"); // TODO
}

void execute_sum(t_pcb *pcb, char *registro_destino, char *registro_origen)
{
    log_error(logger, "Falta implementar SUM"); // TODO
}

void execute_sub(t_pcb *pcb, char *registro_destino, char *registro_origen)
{
    log_error(logger, "Falta implementar SUB"); // TODO
}

void execute_jnz(t_pcb *pcb, char *registro, uint32_t nuevo_program_counter)
{
    log_error(logger, "Falta implementar JNZ"); // TODO
}

void execute_io_gen_sleep(t_pcb *pcb, char *interfaz, uint32_t unidades_de_trabajo)
{
    log_error(logger, "Falta implementar IO_GEN_SLEEP"); // TODO
}

void execute_exit(t_pcb *pcb)
{
    log_info(logger, "PID: %i - Ejecutando: EXIT", pcb->pid);
    enviar_exit(socket_kernel_dispatch, pcb);
    continuar_ciclo = 0;
}