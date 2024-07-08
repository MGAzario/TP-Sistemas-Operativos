#include"cpu.h"

t_config *config;
t_log *logger;

char *ip_memoria;
int socket_cpu_dispatch;
int socket_cpu_interrupt;
int socket_kernel_dispatch;
int socket_kernel_interrupt;
int socket_memoria;

int tamanio_pagina;

bool lru;
int cantidad_entradas_tlb;
t_list *lista_entradas_tlb;
t_queue *cola_reemplazo_tlb;

int continuar_ciclo;
int pid_de_interrupcion;
motivo_interrupcion motivo_de_interrupcion;

pthread_t hilo_interrupciones;

int main(int argc, char *argv[])
{
    // Las creaciones se pasan a funciones para limpiar el main.
    crear_logger();
    crear_config();

    decir_hola("CPU");

    iniciar_servidores();
    
    // Establecer conexión con el módulo Memoria
    conectar_memoria();

    // Se inicializan variables globales
    tamanio_pagina = -1;
    continuar_ciclo = 1;
    pid_de_interrupcion = -1;
    cantidad_entradas_tlb = config_get_int_value(config, "CANTIDAD_ENTRADAS_TLB");
    if(cantidad_entradas_tlb < 0)
    {
        log_error(logger, "Cantidad negativa de entradas de la TLB (debería ser 0 o más)");
    }
    lista_entradas_tlb = list_create();
    cola_reemplazo_tlb = queue_create();
    if(strcmp("FIFO", config_get_string_value(config, "ALGORITMO_TLB")) == 0)
    {
        lru = false;
    }
    else if(strcmp("LRU", config_get_string_value(config, "ALGORITMO_TLB")) == 0)
    {
        lru = true;
    }
    else
    {
        log_error(logger, "Algoritmo de reemplazo de la TLB desconocido");
    }

    // Creo un hilo para recibir interrupciones
    if (pthread_create(&hilo_interrupciones, NULL, recibir_interrupciones, NULL) != 0){
        log_error(logger, "Error al inicializar el Hilo de Interrupciones");
        exit(EXIT_FAILURE);
    }
    pthread_detach(hilo_interrupciones);

    // Recibir mensajes del dispatch del Kernel
    recibir_procesos_kernel(socket_kernel_dispatch);

    liberar_conexion(socket_cpu_dispatch);
    liberar_conexion(socket_kernel_dispatch);

    log_info(logger, "Terminó");

    return 0;
}

void crear_logger()
{
    logger = log_create("./cpu.log", "LOG_CPU", true, LOG_LEVEL_TRACE);
    // Tira error si no se pudo crear
    if (logger == NULL)
    {
        perror("Ocurrió un error al leer el archivo de Log de CPU");
        abort();
    }
}

void crear_config()
{
    config = config_create("./cpu.config");
    if (config == NULL)
    {
        log_error(logger, "Ocurrió un error al leer el archivo de configuración\n");
        abort();
    }
}

void iniciar_servidores() {
    // Iniciar servidor de dispatch
    char *puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    socket_cpu_dispatch = iniciar_servidor(puerto_cpu_dispatch);
    socket_kernel_dispatch = esperar_cliente(socket_cpu_dispatch);
    // Iniciar servidor de interrupt
    char *puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
    socket_cpu_interrupt = iniciar_servidor(puerto_cpu_interrupt);
    socket_kernel_interrupt = esperar_cliente(socket_cpu_interrupt);
}

void conectar_memoria()
{
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char *puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    socket_memoria = conectar_modulo(ip_memoria, puerto_memoria);
}

// Función para manejar la conexión entrante
void recibir_procesos_kernel(int socket_cliente)
{
    while(1)
    {
        // Recibe un proceso del kernel para ser ejecutado
        log_debug(logger, "Esperando proceso del kernel");
        op_code cod_op = recibir_operacion(socket_cliente);
        if(cod_op != DISPATCH)
        {
            log_error(logger, "El CPU esperaba recibir una operación DISPATCH del Kernel pero recibió otra operación");
        }
        t_pcb *pcb = recibir_pcb(socket_cliente);

        // Ejecuta el proceso (se pone en marcha el ciclo de instrucción)
        while(continuar_ciclo)
        {
            ciclo_de_instruccion(pcb);
        }
        log_trace(logger, "Saliendo del ciclo de intrucción");
        continuar_ciclo = 1;

        free(pcb->cpu_registers);
        free(pcb);
    }
}

void *recibir_interrupciones()
{
    while(1)
    {
        op_code cod_op = recibir_operacion(socket_kernel_interrupt);
        if(cod_op == DESCONEXION)
        {
            log_warning(logger, "Se desconectó el Kernel del socket INTERRUPT");
            while(1);
        }
        if(cod_op != INTERRUPCION)
        {
            log_error(logger, "El CPU recibió en el socket de interrupciones algo que no es una interrupción");
        }
        t_interrupcion *interrupcion = recibir_interrupcion(socket_kernel_interrupt);
        log_debug(logger, "El Kernel envió una interrupción");

        pid_de_interrupcion = interrupcion->pcb->pid;
        motivo_de_interrupcion = interrupcion->motivo;
        log_trace(logger, "El motivo de interrupcion es %d",motivo_de_interrupcion);
        free(interrupcion->pcb->cpu_registers);
        free(interrupcion->pcb);
        free(interrupcion);
    }
}

void ciclo_de_instruccion(t_pcb *pcb)
{
    char *instruccion = fetch(pcb);
    decode(pcb, instruccion);
    check_interrupt(pcb);
    free(instruccion);
}

char *fetch(t_pcb *pcb)
{
    log_info(logger, "PID: %i - FETCH - Program Counter: %i", pcb->pid, pcb->cpu_registers->pc);

    enviar_solicitud_instruccion(socket_memoria, pcb);

    op_code cod_op = recibir_operacion(socket_memoria);
    if(cod_op != INSTRUCCION)
    {
        log_error(logger, "El CPU esperaba recibir una operación INSTRUCCION de la Memoria pero recibió otra operación");
    }
    t_instruccion *instruccion_recibida = recibir_instruccion(socket_memoria);
    char *instruccion = instruccion_recibida->instruccion;

    free(instruccion_recibida);

    return instruccion;
}

void decode(t_pcb *pcb, char *instruccion)
{
    char operacion[20];
    sscanf(instruccion, "%s", operacion);

    if(strcmp("PRUEBA", operacion) == 0)
    {
        log_debug(logger, "PID: %i", pcb->pid);
        log_debug(logger, "program counter: %i", pcb->cpu_registers->pc);
        log_debug(logger, "quantum: %i", pcb->quantum);
        log_debug(logger, "estado: %i", pcb->estado);
        log_debug(logger, "SI: %i", pcb->cpu_registers->si);
        log_debug(logger, "DI: %i", pcb->cpu_registers->di);
        log_debug(logger, "AX: %i", pcb->cpu_registers->normales[AX]);
        log_debug(logger, "BX: %i", pcb->cpu_registers->normales[BX]);
        log_debug(logger, "CX: %i", pcb->cpu_registers->normales[CX]);
        log_debug(logger, "DX: %i", pcb->cpu_registers->normales[DX]);
        log_debug(logger, "EAX: %i", pcb->cpu_registers->extendidos[EAX]);
        log_debug(logger, "EBX: %i", pcb->cpu_registers->extendidos[EBX]);
        log_debug(logger, "ECX: %i", pcb->cpu_registers->extendidos[ECX]);
        log_debug(logger, "EDX: %i", pcb->cpu_registers->extendidos[EDX]);

        pcb->cpu_registers->normales[AX] = 124;

        // free(pcb->cpu_registers);
        // free(pcb);

        log_debug(logger, "Terminó la prueba");
        pcb->cpu_registers->pc++;
    }
    else if (strcmp("SET", operacion) == 0)
    {
        char registro[10];
        uint32_t valor;

        sscanf(instruccion, "%s %s %u", operacion, registro, &valor);

        pcb->cpu_registers->pc++;
        execute_set(pcb, registro, valor);
    }
    else if (strcmp("MOV_IN", operacion) == 0)
    {
        char registro_datos[10];
        char registro_direccion[10];

        sscanf(instruccion, "%s %s %s", operacion, registro_datos, registro_direccion);

        t_list *direcciones_fisicas = mmu(leer_registro(pcb, registro_direccion), tamanio_registro(registro_datos), pcb->pid);

        pcb->cpu_registers->pc++;
        log_info(logger, "PID: %i - Ejecutando: MOV_IN - %s %s", pcb->pid, registro_datos, registro_direccion);
        execute_mov_in(pcb, registro_datos, direcciones_fisicas);
    }
    else if (strcmp("MOV_OUT", operacion) == 0)
    {
        char registro_direccion[10];
        char registro_datos[10];

        sscanf(instruccion, "%s %s %s", operacion, registro_direccion, registro_datos);

        t_list *direcciones_fisicas = mmu(leer_registro(pcb, registro_direccion), tamanio_registro(registro_datos), pcb->pid);

        pcb->cpu_registers->pc++;
        log_info(logger, "PID: %i - Ejecutando: MOV_OUT - %s %s", pcb->pid, registro_direccion, registro_datos);
        execute_mov_out(pcb, direcciones_fisicas, registro_datos);
    }
    else if (strcmp("SUM", operacion) == 0)
    {
        char registro_destino[10];
        char registro_origen[10];

        sscanf(instruccion, "%s %s %s", operacion, registro_destino, registro_origen);

        pcb->cpu_registers->pc++;
        execute_sum(pcb, registro_destino, registro_origen);
    }
    else if (strcmp("SUB", operacion) == 0)
    {
        char registro_destino[10];
        char registro_origen[10];

        sscanf(instruccion, "%s %s %s", operacion, registro_destino, registro_origen);

        pcb->cpu_registers->pc++;
        execute_sub(pcb, registro_destino, registro_origen);
    }
    else if (strcmp("JNZ", operacion) == 0)
    {
        char registro[10];
        uint32_t nuevo_program_counter;

        sscanf(instruccion, "%s %s %u", operacion, registro, &nuevo_program_counter);

        pcb->cpu_registers->pc++;
        execute_jnz(pcb, registro, nuevo_program_counter);
    }
    else if (strcmp("RESIZE", operacion) == 0)
    {
        int nuevo_tamanio_del_proceso;

        sscanf(instruccion, "%s %i", operacion, &nuevo_tamanio_del_proceso);

        pcb->cpu_registers->pc++;
        execute_resize(pcb, nuevo_tamanio_del_proceso);
    }
    else if (strcmp("WAIT", operacion) == 0)
    {
        char nombre_recurso[50];

        sscanf(instruccion, "%s %s", operacion, nombre_recurso);

        pcb->cpu_registers->pc++;
        execute_wait(pcb, nombre_recurso);
    }
    else if (strcmp("SIGNAL", operacion) == 0)
    {
        char nombre_recurso[50];

        sscanf(instruccion, "%s %s", operacion, nombre_recurso);

        pcb->cpu_registers->pc++;
        execute_signal(pcb, nombre_recurso);
    }
    else if (strcmp("IO_GEN_SLEEP", operacion) == 0)
    {
        char nombre_interfaz[50];
        uint32_t unidades_de_trabajo;

        sscanf(instruccion, "%s %s %u", operacion, nombre_interfaz, &unidades_de_trabajo);

        pcb->cpu_registers->pc++;
        execute_io_gen_sleep(pcb, nombre_interfaz, unidades_de_trabajo);
    }
    else if (strcmp("IO_STDIN_READ", operacion) == 0)
    {
    char nombre_interfaz[50];
    char registro_direccion[10];
    char registro_tamaño[10];

    sscanf(instruccion, "%s %s %s %s", operacion, nombre_interfaz, registro_direccion, registro_tamaño);

    uint32_t direccion_logica = leer_registro(pcb, registro_direccion);
    uint32_t tamaño = leer_registro(pcb, registro_tamaño);

    // Convertir la dirección lógica a direcciones físicas
    t_list *direcciones_fisicas = mmu(direccion_logica, tamaño, pcb->pid);

    pcb->cpu_registers->pc++;
    log_info(logger, "PID: %i - Ejecutando: IO_STDIN_READ - %s %s %s", pcb->pid, nombre_interfaz, registro_direccion, registro_tamaño);

    execute_io_stdin_read(pcb, nombre_interfaz, direcciones_fisicas, tamaño);
    }
    else if (strcmp("IO_STDOUT_WRITE", operacion) == 0)
    {
    char nombre_interfaz[50];
    char registro_direccion[10];
    char registro_tamaño[10];

    sscanf(instruccion, "%s %s %s %s", operacion, nombre_interfaz, registro_direccion, registro_tamaño);

    uint32_t direccion_logica = leer_registro(pcb, registro_direccion);
    uint32_t tamaño = leer_registro(pcb, registro_tamaño);

    // Convertir la dirección lógica a direcciones físicas (en este caso no necesitamos mmu)
    t_list *direcciones_fisicas = mmu(direccion_logica, tamaño, pcb->pid);

    pcb->cpu_registers->pc++;
    log_info(logger, "PID: %i - Ejecutando: IO_STDOUT_WRITE - %s %s %s", pcb->pid, nombre_interfaz, registro_direccion, registro_tamaño);

    execute_io_stdout_write(pcb, nombre_interfaz, direcciones_fisicas, tamaño);
    }
    else if (strcmp("IO_FS_CREATE", operacion) == 0)
    {
        char nombre_interfaz[50];
        char nombre_archivo[256]; // Asumimos que el nombre del archivo no superará los 256 caracteres

        sscanf(instruccion, "%s %s %s", operacion, nombre_interfaz, nombre_archivo);

        // Aquí no necesitas convertir direcciones lógicas a físicas ni leer registros específicos
        pcb->cpu_registers->pc++;
        log_info(logger, "PID: %i - Ejecutando: IO_FS_CREATE - %s %s", pcb->pid, nombre_interfaz, nombre_archivo);

        execute_io_fs_create(pcb, nombre_interfaz, nombre_archivo);
    }
    else if (strcmp("IO_FS_DELETE", operacion) == 0)
    {
        char nombre_interfaz[50];
        char nombre_archivo[256]; // Asumimos que el nombre del archivo no superará los 256 caracteres

        sscanf(instruccion, "%s %s %s", operacion, nombre_interfaz, nombre_archivo);

        // Aquí no necesitas convertir direcciones lógicas a físicas ni leer registros específicos
        pcb->cpu_registers->pc++;
        log_info(logger, "PID: %i - Ejecutando: IO_FS_DELETE - %s %s", pcb->pid, nombre_interfaz, nombre_archivo);

        execute_io_fs_delete(pcb, nombre_interfaz, nombre_archivo);
    }
    else if (strcmp("EXIT", operacion) == 0)
    {
        pcb->cpu_registers->pc++;
        execute_exit(pcb);
    }
    else
    {
        log_error(logger, "Instrucción desconocida: %s", instruccion);
        sleep(1);
    }
}

void check_interrupt(t_pcb *pcb)
{
    if(pid_de_interrupcion == pcb->pid)
    {
        log_trace(logger, "El check interrupt detectó una interrupción para el pid %i. Se desalojará y enviará al Kernel", pcb->pid);

        enviar_interrupcion(socket_kernel_dispatch, pcb, motivo_de_interrupcion);

        log_trace(logger, "Se envío la interrupción");

        pid_de_interrupcion = -1;
        continuar_ciclo = 0;
    }
    else if(pid_de_interrupcion != -1)
    {
        log_trace(logger, "El check interrupt detectó una interrupción, pero para otro pid (%i), así que la ignora", pcb->pid);
    }
}

void preguntar_tamanio_pagina()
{
    if(tamanio_pagina == -1)
    {
        log_trace(logger, "El CPU no sabe cuál es el tamaño de página, así que le pregunta a la Memoria");
        enviar_mensaje_simple(socket_memoria, PREGUNTA_TAMANIO_PAGINA);
        op_code cod_op = recibir_operacion(socket_memoria);
        if(cod_op != RESPUESTA_TAMANIO_PAGINA)
        {
        log_error(logger, "El CPU esperaba recibir una operación RESPUESTA_TAMANIO_PAGINA de la Memoria pero recibió otra operación");
        }
        tamanio_pagina = recibir_numero(socket_memoria);
        log_trace(logger, "Ahora el CPU sabe que el tamaño de página es %i", tamanio_pagina);
    }
}

uint32_t leer_registro(t_pcb *pcb, char *registro)
{
    if (strcmp(registro, "AX") == 0)
    {
        return pcb->cpu_registers->normales[AX];
    }
    else if (strcmp(registro, "BX") == 0)
    {
        return pcb->cpu_registers->normales[BX];
    }
    else if (strcmp(registro, "CX") == 0)
    {
        return pcb->cpu_registers->normales[CX];
    }
    else if (strcmp(registro, "DX") == 0)
    {
        return pcb->cpu_registers->normales[DX];
    }
    else if (strcmp(registro, "EAX") == 0)
    {
        return pcb->cpu_registers->extendidos[EAX];
    }
    else if (strcmp(registro, "EBX") == 0)
    {
        return pcb->cpu_registers->extendidos[EBX];
    }
    else if (strcmp(registro, "ECX") == 0)
    {
        return pcb->cpu_registers->extendidos[ECX];
    }
    else if (strcmp(registro, "EDX") == 0)
    {
        return pcb->cpu_registers->extendidos[EDX];
    }
    else
    {
        log_error(logger, "El registro %s no existe", registro);
        return -1;
    }
}

void escribir_registro(t_pcb *pcb, char *registro, uint32_t valor)
{
    if (strcmp(registro, "AX") == 0)
    {
        pcb->cpu_registers->normales[AX] = valor;
    }
    else if (strcmp(registro, "BX") == 0)
    {
        pcb->cpu_registers->normales[BX] = valor;
    }
    else if (strcmp(registro, "CX") == 0)
    {
        pcb->cpu_registers->normales[CX] = valor;
    }
    else if (strcmp(registro, "DX") == 0)
    {
        pcb->cpu_registers->normales[DX] = valor;
    }
    else if (strcmp(registro, "EAX") == 0)
    {
        pcb->cpu_registers->extendidos[EAX] = valor;
    }
    else if (strcmp(registro, "EBX") == 0)
    {
        pcb->cpu_registers->extendidos[EAX] = valor;
    }
    else if (strcmp(registro, "ECX") == 0)
    {
        pcb->cpu_registers->extendidos[EAX] = valor;
    }
    else if (strcmp(registro, "EDX") == 0)
    {
        pcb->cpu_registers->extendidos[EAX] = valor;
    }
    else
    {
        log_error(logger, "El registro %s no existe", registro);
    }
}

int tamanio_registro(char *registro)
{
    if (strcmp(registro, "AX") == 0)
    {
        return 1;
    }
    else if (strcmp(registro, "BX") == 0)
    {
        return 1;
    }
    else if (strcmp(registro, "CX") == 0)
    {
        return 1;
    }
    else if (strcmp(registro, "DX") == 0)
    {
        return 1;
    }
    else if (strcmp(registro, "EAX") == 0)
    {
        return 4;
    }
    else if (strcmp(registro, "EBX") == 0)
    {
        return 4;
    }
    else if (strcmp(registro, "ECX") == 0)
    {
        return 4;
    }
    else if (strcmp(registro, "EDX") == 0)
    {
        return 4;
    }
    else
    {
        log_error(logger, "El registro %s no existe", registro);
        return -1;
    }
}

t_list *mmu(uint32_t direccion_logica, int tamanio_del_contenido, int pid)
{
    t_list *lista_direcciones_fisicas = list_create();

    preguntar_tamanio_pagina();
    int numero_pagina = floor(direccion_logica / tamanio_pagina);
    int desplazamiento = direccion_logica - numero_pagina * tamanio_pagina;
    log_trace(logger, "El MMU traducirá la dirección lógica %u (página %i, desplazamiento %i)", direccion_logica, numero_pagina, desplazamiento);

    t_direccion_y_tamanio *direccion_y_tamanio = malloc(sizeof(t_direccion_y_tamanio));
    direccion_y_tamanio->direccion = averiguar_marco(pid, numero_pagina) * tamanio_pagina + desplazamiento;

    // Si alcanza con una sola dirección física:
    if(tamanio_del_contenido <= tamanio_pagina - desplazamiento)
    {
        direccion_y_tamanio->tamanio = tamanio_del_contenido;
        list_add(lista_direcciones_fisicas, direccion_y_tamanio);

        log_trace(logger, "La dirección lógica se tradujo en una única dirección física (%i)", direccion_y_tamanio->direccion);
        return lista_direcciones_fisicas;
    }
    // Si hacen falta al menos dos:
    else
    {
        direccion_y_tamanio->tamanio = tamanio_pagina - desplazamiento;
        list_add(lista_direcciones_fisicas, direccion_y_tamanio);

        tamanio_del_contenido -= tamanio_pagina - desplazamiento;
        while(tamanio_del_contenido >= tamanio_pagina)
        {
            numero_pagina++;
            t_direccion_y_tamanio *direccion_y_tamanio = malloc(sizeof(t_direccion_y_tamanio));
            direccion_y_tamanio->direccion = averiguar_marco(pid, numero_pagina) * tamanio_pagina;
            direccion_y_tamanio->tamanio = tamanio_pagina;
            list_add(lista_direcciones_fisicas, direccion_y_tamanio);
            tamanio_del_contenido -= tamanio_pagina;
        }    

        if(tamanio_del_contenido > 0)
        {
            numero_pagina++;
            t_direccion_y_tamanio *direccion_y_tamanio = malloc(sizeof(t_direccion_y_tamanio));
            direccion_y_tamanio->direccion = averiguar_marco(pid, numero_pagina) * tamanio_pagina;
            direccion_y_tamanio->tamanio = tamanio_del_contenido;
            list_add(lista_direcciones_fisicas, direccion_y_tamanio);
        }

        log_trace(logger, "La dirección lógica debió traducirse en %i direcciones físicas", list_size(lista_direcciones_fisicas));
        return lista_direcciones_fisicas;
    }
}

int averiguar_marco(int pid, int pagina)
{
    // Si la TLB está deshabilitada, le preguntamos directamente a la memoria
    if(cantidad_entradas_tlb == 0)
    {
        return preguntar_marco_a_memoria(pid, pagina);
    }

    // Buscamos la página en la TLB
    for (int i = 0; i < list_size(lista_entradas_tlb); i++)
    {
        t_entrada_tlb *entrada = list_get(lista_entradas_tlb, i);

        if (entrada->pid == pid)
        {
            if(entrada->pagina == pagina)
            {
                log_info(logger, "PID: %i - TLB HIT - Pagina: %i", pid, pagina);
                log_info(logger, "PID: %i - OBTENER MARCO - Página: %i - Marco: %i", pid, pagina, entrada->marco);
                if(lru)
                {
                    t_numero *recientemente_usado;
                    for (int j = 0; j < list_size(cola_reemplazo_tlb->elements); j++)
                    {
                        t_numero *numero_cola = list_get(cola_reemplazo_tlb->elements, j);
                        if(numero_cola->numero == i)
                        {
                            recientemente_usado = list_remove(cola_reemplazo_tlb->elements, j);
                        }
                    }
                    queue_push(cola_reemplazo_tlb, recientemente_usado);
                }
                return entrada->marco;
            }
        }
    }
    log_info(logger, "PID: %i - TLB MISS - Pagina: %i", pid, pagina);

    // La página no estaba en la TLB, así que se la consultamos a la memoria
    int marco = preguntar_marco_a_memoria(pid, pagina);
    t_entrada_tlb *nueva_entrada = malloc(sizeof(t_entrada_tlb));
    nueva_entrada->pid = pid;
    nueva_entrada->pagina = pagina;
    nueva_entrada->marco = marco;

    // Si la TLB aún tiene espacio disponible, agregamos una entrada
    if(list_size(lista_entradas_tlb) < cantidad_entradas_tlb)
    {
        t_numero *numero_de_entrada = malloc(sizeof(t_numero));
        numero_de_entrada->numero = list_add(lista_entradas_tlb, nueva_entrada);
        queue_push(cola_reemplazo_tlb, numero_de_entrada);
        tlb();
    }
    // Si la TLB no tiene espacio disponible, debemos elegir una víctima y reemplazarla
    else
    {
        t_numero *numero_victima = queue_pop(cola_reemplazo_tlb);
        t_entrada_tlb *victima = list_replace(lista_entradas_tlb, numero_victima->numero, nueva_entrada);
        queue_push(cola_reemplazo_tlb, numero_victima);
        free(victima);
        tlb();
    }

    return marco;
}

int preguntar_marco_a_memoria(int pid, int pagina)
{
    enviar_solicitud_marco(socket_memoria, pid, pagina);
    op_code cod_op = recibir_operacion(socket_memoria);
    if(cod_op != MARCO)
    {
    log_error(logger, "El CPU esperaba recibir una operación MARCO de la Memoria pero recibió otra operación");
    }
    int marco = recibir_numero(socket_memoria);
    log_info(logger, "PID: %i - OBTENER MARCO - Página: %i - Marco: %i", pid, pagina, marco);
    return marco;
}

// Esta función es solo para testear
void tlb()
{
    log_trace(logger, "TLB (capacidad máxima de %i entradas):", cantidad_entradas_tlb);
    log_trace(logger, "PID | Página | Marco");
    for (int i = 0; i < list_size(lista_entradas_tlb); i++)
    {
        t_entrada_tlb *entrada = list_get(lista_entradas_tlb, i);
        if(entrada->pagina < 10)
        {
            log_trace(logger, "%i   | %i      | %i    ", entrada->pid, entrada->pagina, entrada->marco);
        }
        else
        {
            log_trace(logger, "%i   | %i     | %i    ", entrada->pid, entrada->pagina, entrada->marco);
        }
    }
}