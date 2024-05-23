#include"cpu.h"

t_config *config;
t_log *logger;

char *ip_memoria;
int socket_cpu_dispatch;
int socket_cpu_interrupt;
int socket_kernel_dispatch;
int socket_kernel_interrupt;
int socket_memoria;

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
    continuar_ciclo = 1;
    pid_de_interrupcion = -1;

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
    }
    else if (strcmp("SET", operacion) == 0)
    {
        char registro[10];
        uint32_t valor;

        sscanf(instruccion, "%s %s %u", operacion, registro, &valor);

        execute_set(pcb, registro, valor);
    }
    else if (strcmp("SUM", operacion) == 0)
    {
        char registro_destino[10];
        char registro_origen[10];

        sscanf(instruccion, "%s %s %s", operacion, registro_destino, registro_origen);

        execute_sum(pcb, registro_destino, registro_origen);
    }
    else if (strcmp("SUB", operacion) == 0)
    {
        char registro_destino[10];
        char registro_origen[10];

        sscanf(instruccion, "%s %s %s", operacion, registro_destino, registro_origen);

        execute_sub(pcb, registro_destino, registro_origen);
    }
    else if (strcmp("JNZ", operacion) == 0)
    {
        char registro[10];
        uint32_t nuevo_program_counter;

        sscanf(instruccion, "%s %s %u", operacion, registro, &nuevo_program_counter);

        execute_jnz(pcb, registro, nuevo_program_counter);
    }
    else if (strcmp("IO_GEN_SLEEP", operacion) == 0)
    {
        char interfaz[50];
        uint32_t unidades_de_trabajo;

        sscanf(instruccion, "%s %s %u", operacion, interfaz, &unidades_de_trabajo);

        //execute_io_gen_sleep(pcb, interfaz, unidades_de_trabajo);
        t_paquete mensaje_sleep = crear_paquete();
        mensaje_sleep->cod_op = IO_GEN_SLEEP;
        enviar_paquete(mensaje_sleep,socket_kernel_dispatch);
        eliminar_paquete(mensaje_sleep);
        liberar_conexion(socket_kernel_dispatch);
    }
    else if (strcmp("EXIT", operacion) == 0)
    {
        execute_exit(pcb);
    }
    else
    {
        log_error(logger, "Instrucción desconocida: %s", instruccion);
        sleep(1);
    }
    
    pcb->cpu_registers->pc++;
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