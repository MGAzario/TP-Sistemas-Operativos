#include "instrucciones.h"

void execute_set(t_pcb *pcb, char *registro, uint32_t valor)
{
    if (strcmp("AX", registro) == 0){
        pcb->cpu_registers->normales[0] = (uint8_t)valor;
    } else if (strcmp("BX", registro) == 0){
        pcb->cpu_registers->normales[1] = (uint8_t)valor;
    } else if (strcmp("CX", registro) == 0){
        pcb->cpu_registers->normales[2] = (uint8_t)valor;
    } else if (strcmp("DX", registro) == 0){
        pcb->cpu_registers->normales[3] = (uint8_t)valor;
    } else if (strcmp("EAX", registro) == 0){
        pcb->cpu_registers->extendidos[0] = valor;
    } else if (strcmp("EBX", registro) == 0){
        pcb->cpu_registers->extendidos[1] = valor;
    } else if (strcmp("ECX", registro) == 0){
        pcb->cpu_registers->extendidos[2] = valor;
    } else if (strcmp("EDX", registro) == 0){
        pcb->cpu_registers->extendidos[3] = valor;
    } else {
        log_error(logger, "Registro %s desconocido en SET", registro);
    }
}


void execute_sum(t_pcb *pcb, char *registro_destino, char *registro_origen)
{
    uint32_t *puntero_destino_extendidos = NULL, *puntero_origen_extendidos = NULL;
    uint8_t *puntero_destino_normales = NULL, *puntero_origen_normales = NULL;

    // Asignar puntero de destino
    if (strcmp(registro_destino, "AX") == 0) {
        puntero_destino_normales = &pcb->cpu_registers->normales[0];
    } else if (strcmp(registro_destino, "BX") == 0) {
        puntero_destino_normales = &pcb->cpu_registers->normales[1];
    } else if (strcmp(registro_destino, "CX") == 0) {
        puntero_destino_normales = &pcb->cpu_registers->normales[2];
    } else if (strcmp(registro_destino, "DX") == 0) {
        puntero_destino_normales = &pcb->cpu_registers->normales[3];
    } else if (strcmp(registro_destino, "EAX") == 0) {
        puntero_destino_extendidos = &pcb->cpu_registers->extendidos[0];
    } else if (strcmp(registro_destino, "EBX") == 0) {
        puntero_destino_extendidos = &pcb->cpu_registers->extendidos[1];
    } else if (strcmp(registro_destino, "ECX") == 0) {
        puntero_destino_extendidos = &pcb->cpu_registers->extendidos[2];
    } else if (strcmp(registro_destino, "EDX") == 0) {
        puntero_destino_extendidos = &pcb->cpu_registers->extendidos[3];
    }

    // Asignar puntero de origen
    if (strcmp(registro_origen, "AX") == 0) {
        puntero_origen_normales = &pcb->cpu_registers->normales[0];
    } else if (strcmp(registro_origen, "BX") == 0) {
        puntero_origen_normales = &pcb->cpu_registers->normales[1];
    } else if (strcmp(registro_origen, "CX") == 0) {
        puntero_origen_normales = &pcb->cpu_registers->normales[2];
    } else if (strcmp(registro_origen, "DX") == 0) {
        puntero_origen_normales = &pcb->cpu_registers->normales[3];
    } else if (strcmp(registro_origen, "EAX") == 0) {
        puntero_origen_extendidos = &pcb->cpu_registers->extendidos[0];
    } else if (strcmp(registro_origen, "EBX") == 0) {
        puntero_origen_extendidos = &pcb->cpu_registers->extendidos[1];
    } else if (strcmp(registro_origen, "ECX") == 0) {
        puntero_origen_extendidos = &pcb->cpu_registers->extendidos[2];
    } else if (strcmp(registro_origen, "EDX") == 0) {
        puntero_origen_extendidos = &pcb->cpu_registers->extendidos[3];
    }

    // Realizar la suma
    if (puntero_destino_normales != NULL && puntero_origen_normales != NULL) {
        *puntero_destino_normales += *puntero_origen_normales;
    } else if (puntero_destino_extendidos != NULL && puntero_origen_extendidos != NULL) {
        *puntero_destino_extendidos += *puntero_origen_extendidos;
    } else {
        log_error(logger, "Registros desconocidos en SUM: %s, %s", registro_destino, registro_origen);
    }
}



void execute_sub(t_pcb *pcb, char *registro_destino, char *registro_origen)
{
    uint32_t *puntero_destino_extendidos = NULL, *puntero_origen_extendidos = NULL;
    uint8_t *puntero_destino_normales = NULL, *puntero_origen_normales = NULL;

    // Asignar puntero de destino
    if (strcmp(registro_destino, "AX") == 0) {
        puntero_destino_normales = &pcb->cpu_registers->normales[0];
    } else if (strcmp(registro_destino, "BX") == 0) {
        puntero_destino_normales = &pcb->cpu_registers->normales[1];
    } else if (strcmp(registro_destino, "CX") == 0) {
        puntero_destino_normales = &pcb->cpu_registers->normales[2];
    } else if (strcmp(registro_destino, "DX") == 0) {
        puntero_destino_normales = &pcb->cpu_registers->normales[3];
    } else if (strcmp(registro_destino, "EAX") == 0) {
        puntero_destino_extendidos = &pcb->cpu_registers->extendidos[0];
    } else if (strcmp(registro_destino, "EBX") == 0) {
        puntero_destino_extendidos = &pcb->cpu_registers->extendidos[1];
    } else if (strcmp(registro_destino, "ECX") == 0) {
        puntero_destino_extendidos = &pcb->cpu_registers->extendidos[2];
    } else if (strcmp(registro_destino, "EDX") == 0) {
        puntero_destino_extendidos = &pcb->cpu_registers->extendidos[3];
    }

    // Asignar puntero de origen
    if (strcmp(registro_origen, "AX") == 0) {
        puntero_origen_normales = &pcb->cpu_registers->normales[0];
    } else if (strcmp(registro_origen, "BX") == 0) {
        puntero_origen_normales = &pcb->cpu_registers->normales[1];
    } else if (strcmp(registro_origen, "CX") == 0) {
        puntero_origen_normales = &pcb->cpu_registers->normales[2];
    } else if (strcmp(registro_origen, "DX") == 0) {
        puntero_origen_normales = &pcb->cpu_registers->normales[3];
    } else if (strcmp(registro_origen, "EAX") == 0) {
        puntero_origen_extendidos = &pcb->cpu_registers->extendidos[0];
    } else if (strcmp(registro_origen, "EBX") == 0) {
        puntero_origen_extendidos = &pcb->cpu_registers->extendidos[1];
    } else if (strcmp(registro_origen, "ECX") == 0) {
        puntero_origen_extendidos = &pcb->cpu_registers->extendidos[2];
    } else if (strcmp(registro_origen, "EDX") == 0) {
        puntero_origen_extendidos = &pcb->cpu_registers->extendidos[3];
    }

    // Realizar la resta
    if (puntero_destino_normales != NULL && puntero_origen_normales != NULL) {
        *puntero_destino_normales -= *puntero_origen_normales;
    } else if (puntero_destino_extendidos != NULL && puntero_origen_extendidos != NULL) {
        *puntero_destino_extendidos -= *puntero_origen_extendidos;
    } else {
        log_error(logger, "Registros desconocidos en SUB: %s, %s", registro_destino, registro_origen);
    }
}


void execute_jnz(t_pcb *pcb, char *registro, uint32_t nuevo_program_counter)
{
    uint32_t *puntero_registro_extendidos = NULL;
    uint8_t *puntero_registro_normales = NULL;

    // Asignar puntero del registro
    if (strcmp(registro, "AX") == 0) {
        puntero_registro_normales = &pcb->cpu_registers->normales[0];
    } else if (strcmp(registro, "BX") == 0) {
        puntero_registro_normales = &pcb->cpu_registers->normales[1];
    } else if (strcmp(registro, "CX") == 0) {
        puntero_registro_normales = &pcb->cpu_registers->normales[2];
    } else if (strcmp(registro, "DX") == 0) {
        puntero_registro_normales = &pcb->cpu_registers->normales[3];
    } else if (strcmp(registro, "EAX") == 0) {
        puntero_registro_extendidos = &pcb->cpu_registers->extendidos[0];
    } else if (strcmp(registro, "EBX") == 0) {
        puntero_registro_extendidos = &pcb->cpu_registers->extendidos[1];
    } else if (strcmp(registro, "ECX") == 0) {
        puntero_registro_extendidos = &pcb->cpu_registers->extendidos[2];
    } else if (strcmp(registro, "EDX") == 0) {
        puntero_registro_extendidos = &pcb->cpu_registers->extendidos[3];
    }

    // Realizar la comparación y el salto
    if ((puntero_registro_normales != NULL && *puntero_registro_normales != 0) ||
        (puntero_registro_extendidos != NULL && *puntero_registro_extendidos != 0)) {
        pcb->cpu_registers->pc = nuevo_program_counter;
    }
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