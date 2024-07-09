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

void execute_io_stdin_read(t_pcb *pcb, char *interfaz, t_list *direcciones_fisicas, uint32_t tamaño) {
    log_info(logger, "PID: %i - Ejecutando: IO_STDIN_READ", pcb->pid);

    // Crear la estructura t_io_stdin_read
    uint32_t tamanio_nombre_interfaz = strlen(interfaz) + 1;
    t_io_stdin_read *io_stdin_read = crear_io_stdin_read(pcb, interfaz, tamanio_nombre_interfaz, tamaño, direcciones_fisicas);

    // Enviar la estructura t_io_stdin_read al kernel dispatch
    enviar_io_stdin_read(socket_kernel_dispatch, io_stdin_read);

    // Liberar memoria de la estructura t_io_stdin_read
    free(io_stdin_read->nombre_interfaz);
    free(io_stdin_read);
    
    continuar_ciclo = 0;
    log_trace(logger, "Terminando instrucción de IO_STDIN_READ");
}


void execute_io_stdout_write(t_pcb *pcb, char *interfaz, uint32_t direccion_logica, uint32_t tamaño) {
    log_info(logger, "PID: %i - Ejecutando: IO_STDOUT_WRITE", pcb->pid);

    // Crear la estructura t_io_stdout_write
    uint32_t tamanio_nombre_interfaz = strlen(interfaz) + 1;
    t_io_stdout_write *io_stdout_write = crear_io_stdout_write(pcb, interfaz, tamanio_nombre_interfaz, direccion_logica, tamaño);

    // Enviar la estructura t_io_stdout_write al kernel dispatch
    enviar_io_stdout_write(socket_kernel_dispatch, io_stdout_write);

    // Liberar memoria de la estructura t_io_stdout_write
    free(io_stdout_write->nombre_interfaz);
    free(io_stdout_write);

    continuar_ciclo = 0;
    log_trace(logger, "Terminando instrucción de IO_STDOUT_WRITE");
}

void execute_io_fs_create(t_pcb *pcb, char *interfaz, char *nombre_archivo) {
    log_info(logger, "PID: %i - Ejecutando: IO_FS_CREATE para el archivo %s", pcb->pid, nombre_archivo);

    // Crear la estructura t_io_fs_create
    t_io_fs_create *io_fs_create = crear_io_fs_create(pcb, interfaz, nombre_archivo);
    if (io_fs_create == NULL) {
        log_error(logger, "No se pudo crear la estructura para IO_FS_CREATE");
        return; // Gestión de error en caso de que no se pueda crear la estructura
    }

    // Enviar la estructura t_io_fs_create al kernel dispatch
    enviar_io_fs_create(socket_kernel_dispatch, io_fs_create);

    // Liberar memoria de la estructura t_io_fs_create
    free(io_fs_create->nombre_interfaz);
    free(io_fs_create->nombre_archivo);
    free(io_fs_create);

    continuar_ciclo = 0;
    log_trace(logger, "Terminando instrucción de IO_FS_CREATE para el archivo %s", nombre_archivo);
}

void execute_io_fs_delete(t_pcb *pcb, char *interfaz, char *nombre_archivo) {
    log_info(logger, "PID: %i - Ejecutando: IO_FS_DELETE para el archivo %s", pcb->pid, nombre_archivo);

    // Crear la estructura t_io_fs_delete
    t_io_fs_delete *io_fs_delete = crear_io_fs_delete(pcb, interfaz, nombre_archivo);
    if (io_fs_delete == NULL) {
        log_error(logger, "No se pudo crear la estructura para IO_FS_DELETE");
        return; // Gestión de error en caso de que no se pueda crear la estructura
    }

    // Enviar la estructura t_io_fs_delete al kernel dispatch
    enviar_io_fs_delete(socket_kernel_dispatch, io_fs_delete);

    // Liberar memoria de la estructura t_io_fs_delete
    free(io_fs_delete->nombre_interfaz);
    free(io_fs_delete->nombre_archivo);
    free(io_fs_delete);

    continuar_ciclo = 0;
    log_trace(logger, "Terminando instrucción de IO_FS_DELETE para el archivo %s", nombre_archivo);
}

void execute_io_fs_truncate(t_pcb *pcb, char *interfaz, char *nombre_archivo, uint32_t nuevo_tamano) {
    log_info(logger, "PID: %i - Ejecutando: IO_FS_TRUNCATE para el archivo %s con nuevo tamaño %u", pcb->pid, nombre_archivo, nuevo_tamano);

    // Crear la estructura para la solicitud
    t_io_fs_truncate *io_fs_truncate = crear_io_fs_truncate(pcb, interfaz, nombre_archivo, nuevo_tamano);
    if (io_fs_truncate == NULL) {
        log_error(logger, "No se pudo crear la estructura para IO_FS_TRUNCATE");
        return;  // Manejo de error
    }

    // Enviar la estructura al kernel dispatch
    enviar_io_fs_truncate(socket_kernel_dispatch, io_fs_truncate);

    // Liberar memoria
    free(io_fs_truncate->nombre_interfaz);
    free(io_fs_truncate->nombre_archivo);
    free(io_fs_truncate);

    continuar_ciclo = 0;
    log_trace(logger, "Terminando instrucción de IO_FS_TRUNCATE para el archivo %s", nombre_archivo);
}

void execute_io_fs_write(t_pcb *pcb, char *interfaz, char *nombre_archivo, t_list *direcciones_fisicas, uint32_t tamanio, uint32_t puntero_archivo) {
    log_info(logger, "PID: %i - Ejecutando: IO_FS_WRITE para el archivo %s, tamaño: %u, puntero archivo: %u", pcb->pid, nombre_archivo, tamanio, puntero_archivo);

    // Crear la estructura para la solicitud
    t_io_fs_write *io_fs_write = crear_io_fs_write(pcb, interfaz, nombre_archivo, direcciones_fisicas, tamanio, puntero_archivo);
    if (io_fs_write == NULL) {
        log_error(logger, "No se pudo crear la estructura para IO_FS_WRITE");
        list_destroy(direcciones_fisicas);  // Asegurar limpieza de la lista
        return;  // Manejo de error
    }

    // Enviar la estructura al kernel dispatch
    enviar_io_fs_write(socket_kernel_dispatch, io_fs_write);

    // Liberar memoria
    free(io_fs_write->nombre_interfaz);
    free(io_fs_write->nombre_archivo);
    list_destroy_and_destroy_elements(io_fs_write->direcciones_fisicas, free);
    free(io_fs_write);

    continuar_ciclo = 0;
    log_trace(logger, "Terminando instrucción de IO_FS_WRITE para el archivo %s", nombre_archivo);
}


void execute_exit(t_pcb *pcb)
{
    log_info(logger, "PID: %i - Ejecutando: EXIT", pcb->pid);
    enviar_exit(socket_kernel_dispatch, pcb);
    continuar_ciclo = 0;
}