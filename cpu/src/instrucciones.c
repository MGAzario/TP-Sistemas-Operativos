#include "instrucciones.h"


void execute_set(t_pcb *pcb, char *registro, uint32_t valor)
{
    log_info(logger, "PID: %i - Ejecutando: SET - %s %u", pcb->pid, registro, valor);
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

void execute_mov_in(t_pcb *pcb, char *registro_datos, t_list *direcciones)
{
    uint32_t valor;
    void *puntero_valor = &valor;
    int desplazamiento_puntero = 0;

    for (int i = 0; i < list_size(direcciones); i++)
    {
        t_direccion_y_tamanio *direccion_y_tamanio = list_get(direcciones, i);
        enviar_leer_memoria(socket_memoria, pcb->pid, direccion_y_tamanio->direccion, direccion_y_tamanio->tamanio);
        op_code cod_op = recibir_operacion(socket_memoria);
        if(cod_op != MEMORIA_LEIDA)
        {
            log_error(logger, "El CPU esperaba recibir una operación MEMORIA_LEIDA de la Memoria pero recibió otra operación");
        }
        t_lectura *leido = recibir_lectura(socket_memoria);

        if(tamanio_registro(registro_datos) == 1)
        {
            uint8_t valor_un_byte = 0;
            void *valor_un_byte_puntero = &valor_un_byte;
            memcpy(valor_un_byte_puntero, leido->lectura, direccion_y_tamanio->tamanio);
            log_info(logger, "PID: %i - Acción: LEER - Dirección Física: %i - Valor: %d", pcb->pid, direccion_y_tamanio->direccion, valor_un_byte);
        }
        else if(tamanio_registro(registro_datos) == 4)
        {
            uint32_t valor_cuatro_bytes = 0;
            void *valor_cuatro_bytes_puntero = &valor_cuatro_bytes;
            memcpy(valor_cuatro_bytes_puntero, leido->lectura, direccion_y_tamanio->tamanio);
            log_info(logger, "PID: %i - Acción: LEER - Dirección Física: %i - Valor: %d", pcb->pid, direccion_y_tamanio->direccion, valor_cuatro_bytes);
        }
        else
        {
            log_error(logger, "No se sabe de qué tamaño es el registro. No pudo mostrarse el log de lectura");
        }

        memcpy(puntero_valor + desplazamiento_puntero, leido->lectura, leido->tamanio_lectura);
        desplazamiento_puntero += leido->tamanio_lectura;
        free(leido->lectura);
        free(leido);
    }

    list_destroy_and_destroy_elements(direcciones, destruir_direccion);

    escribir_registro(pcb, registro_datos, valor);
}

void execute_mov_out(t_pcb *pcb, t_list *direcciones, char *registro_datos)
{
    uint32_t valor = leer_registro(pcb, registro_datos);
    void *puntero_valor = &valor;
    int desplazamiento_puntero = 0;

    for (int i = 0; i < list_size(direcciones); i++)
    {
        t_direccion_y_tamanio *direccion_y_tamanio = list_get(direcciones, i);
        void *valor_a_escribir = malloc(direccion_y_tamanio->tamanio);
        memcpy(valor_a_escribir, puntero_valor + desplazamiento_puntero, direccion_y_tamanio->tamanio);
        desplazamiento_puntero += direccion_y_tamanio->tamanio;

        if(tamanio_registro(registro_datos) == 1)
        {
            uint8_t valor_un_byte = 0;
            void *valor_un_byte_puntero = &valor_un_byte;
            memcpy(valor_un_byte_puntero, valor_a_escribir, direccion_y_tamanio->tamanio);
            log_info(logger, "PID: %i - Acción: ESCRIBIR - Dirección Física: %i - Valor: %d", pcb->pid, direccion_y_tamanio->direccion, valor_un_byte);
        }
        else if(tamanio_registro(registro_datos) == 4)
        {
            uint32_t valor_cuatro_bytes = 0;
            void *valor_cuatro_bytes_puntero = &valor_cuatro_bytes;
            memcpy(valor_cuatro_bytes_puntero, valor_a_escribir, direccion_y_tamanio->tamanio);
            log_info(logger, "PID: %i - Acción: ESCRIBIR - Dirección Física: %i - Valor: %d", pcb->pid, direccion_y_tamanio->direccion, valor_cuatro_bytes);
        }
        else
        {
            log_error(logger, "No se sabe de qué tamaño es el registro. No pudo mostrarse el log de escritura");
        }

        enviar_escribir_memoria(socket_memoria, pcb->pid, direccion_y_tamanio->direccion, direccion_y_tamanio->tamanio, valor_a_escribir);
        free(valor_a_escribir);
        op_code cod_op = recibir_operacion(socket_memoria);
        if(cod_op != MEMORIA_ESCRITA)
        {
            log_error(logger, "El CPU esperaba recibir una operación MEMORIA_ESCRITA de la Memoria pero recibió otra operación");
        }
        recibir_ok(socket_memoria);
    }

    list_destroy_and_destroy_elements(direcciones, destruir_direccion);
}

void destruir_direccion(void *elem)
{
    t_direccion_y_tamanio *direccion_y_tamanio = (t_direccion_y_tamanio *)elem;
    free(direccion_y_tamanio);
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

void execute_resize(t_pcb *pcb, int nuevo_tamanio_del_proceso)
{
    log_info(logger, "PID: %i - Ejecutando: RESIZE - %i", pcb->pid, nuevo_tamanio_del_proceso);
    enviar_resize(socket_memoria, pcb, nuevo_tamanio_del_proceso);
    op_code cod_op = recibir_operacion(socket_memoria);
    if(cod_op == RESIZE_EXITOSO)
    {
        recibir_ok(socket_memoria);
        log_debug(logger, "La memoria informa que se ajustó el tamaño del proceso exitosamente");
    }
    else if (cod_op == OUT_OF_MEMORY)
    {
        recibir_ok(socket_memoria);
        log_debug(logger, "La memoria informa que se quedó sin memoria suficiente para ajustar el tamaño del proceso");
        enviar_out_of_memory(socket_kernel_dispatch, pcb);
        continuar_ciclo = 0;
    }
    else
    {
        recibir_ok(socket_memoria);
        log_error(logger, "Respuesta desconocida de la memoria luego de pedirle un RESIZE");
    }
}

void execute_wait(t_pcb *pcb, char *nombre_recurso)
{
    log_info(logger, "PID: %i - Ejecutando: WAIT - %s", pcb->pid, nombre_recurso);
    enviar_wait(socket_kernel_dispatch, pcb, nombre_recurso);
    continuar_ciclo = 0;

}

void execute_signal(t_pcb *pcb, char *nombre_recurso)
{
    log_info(logger, "PID: %i - Ejecutando: SIGNAL - %s", pcb->pid, nombre_recurso);
    enviar_signal(socket_kernel_dispatch, pcb, nombre_recurso);
    continuar_ciclo = 0;
}

void execute_io_gen_sleep(t_pcb *pcb, char *nombre_interfaz, uint32_t unidades_de_trabajo)
{
    /*IO_GEN_SLEEP (Interfaz, Unidades de trabajo): Esta instrucción solicita al Kernel que se envíe a una interfaz de I/O a que realice un 
    sleep por una cantidad de unidades de trabajo.*/
    log_info(logger, "PID: %i - Ejecutando: IO_GEN_SLEEP - %s %i", pcb->pid, nombre_interfaz, unidades_de_trabajo);
    enviar_sleep(socket_kernel_dispatch, pcb, nombre_interfaz, unidades_de_trabajo);
    continuar_ciclo = 0;
    log_trace(logger, "Terminando instrucción de sleep");
}

void execute_io_stdin_read(t_pcb *pcb, char *interfaz, t_list *direcciones_fisicas, uint32_t tamaño)
{
    log_info(logger, "PID: %i - Ejecutando: IO_STDIN_READ", pcb->pid);
    enviar_io_stdin_read(socket_kernel_dispatch, pcb, interfaz, direcciones_fisicas, tamaño);
    continuar_ciclo = 0;
    log_trace(logger, "Terminando instrucción de IO_STDIN_READ");
}

void execute_io_stdout_write(t_pcb *pcb, char *interfaz, t_list *direcciones_fisicas, uint32_t tamaño) {
    log_info(logger, "PID: %i - Ejecutando: IO_STDOUT_WRITE", pcb->pid);
    // Enviar la solicitud de IO_STDOUT_WRITE al kernel dispatch
    enviar_io_stdout_write(socket_kernel_dispatch, pcb, interfaz, direcciones_fisicas, tamaño);
    continuar_ciclo = 0;
    log_trace(logger, "Terminando instrucción de IO_STDOUT_WRITE");
}

void execute_exit(t_pcb *pcb)
{
    log_info(logger, "PID: %i - Ejecutando: EXIT", pcb->pid);
    enviar_exit(socket_kernel_dispatch, pcb);
    continuar_ciclo = 0;
}