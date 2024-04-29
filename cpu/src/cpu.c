#include"cpu.h"

t_config *config;
t_log *logger;

int socket_cpu_dispatch;
int socket_cpu_interrupt;
int socket_kernel_dispatch;
int socket_kernel_interrupt;

int main(int argc, char *argv[])
{
    // Las creaciones se pasan a funciones para limpiar el main.
    crear_logger();
    crear_config();

    decir_hola("CPU");

    iniciar_servidores();
    
    // Establecer conexión con el módulo Memoria
    // conectar_memoria();

    // Recibir mensajes del dispatch del Kernel

    recibir_procesos_kernel(socket_kernel_dispatch);

    liberar_conexion(socket_cpu_dispatch);

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
    char *ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char *puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    int socket_memoria = conectar_modulo(ip_memoria, puerto_memoria);
    if (socket_memoria != -1)
    {
        enviar_mensaje("Mensaje a la Memoria desde el Kernel", socket_memoria);
        liberar_conexion(socket_memoria);
    }
}

// Función para manejar la conexión entrante
void recibir_procesos_kernel(int socket_cliente)
{
    int el_kernel_sigue_conectado = 1;
    while(el_kernel_sigue_conectado)
    {
        op_code cod_op = recibir_operacion(socket_cliente);
        if (cod_op != DESCONEXION)
        {
            t_pcb *pcb_prueba = recibir_pcb(socket_cliente);

            log_info(logger, "código de operación: %i", cod_op);
            log_info(logger, "PID: %i", pcb_prueba->pid);
            log_info(logger, "program counter: %i", pcb_prueba->cpu_registers->pc);
            log_info(logger, "quantum: %i", pcb_prueba->quantum);
            log_info(logger, "estado: %i", pcb_prueba->estado);
            log_info(logger, "SI: %i", pcb_prueba->cpu_registers->si);
            log_info(logger, "DI: %i", pcb_prueba->cpu_registers->di);
            log_info(logger, "AX: %i", pcb_prueba->cpu_registers->normales[AX]);
            log_info(logger, "BX: %i", pcb_prueba->cpu_registers->normales[BX]);
            log_info(logger, "CX: %i", pcb_prueba->cpu_registers->normales[CX]);
            log_info(logger, "DX: %i", pcb_prueba->cpu_registers->normales[DX]);
            log_info(logger, "EAX: %i", pcb_prueba->cpu_registers->extendidos[EAX]);
            log_info(logger, "EBX: %i", pcb_prueba->cpu_registers->extendidos[EBX]);
            log_info(logger, "ECX: %i", pcb_prueba->cpu_registers->extendidos[ECX]);
            log_info(logger, "EDX: %i", pcb_prueba->cpu_registers->extendidos[EDX]);

            pcb_prueba->cpu_registers->normales[AX] = 124;

            enviar_pcb(socket_cliente, pcb_prueba);

            free(pcb_prueba->cpu_registers);
            free(pcb_prueba);
        } else {
            el_kernel_sigue_conectado = 0;
        }
    }

    // while (1)
    // {
    //     op_code cod_op = recibir_operacion(socket_cliente);
    //     switch (cod_op)
    //     {
    //     case DISPATCH:
    //         log_info(logger, "t_pcb recibido desde el Kernel\n");
    //         // La funcion recibir_pcb es una variante de los utils, especifica para poder recibir un PCB del dispatch
    //         // t_pcb *proceso_ejecucion = recibir_pcb(socket_cliente);
    //         break;
    //     case INTERRUPT:
    //         log_info(logger, "Paquete INTERRUPT recibido desde el Kernel\n");
    //         break;
    //     default:
    //         log_info(logger, "Paquete desconocido\n");
    //         break;
    //     } 
    // }
}

void procedimiento_de_prueba()
{
    int el_kernel_sigue_conectado = 1;
    while(el_kernel_sigue_conectado)
    {
        op_code cod_op = recibir_operacion(socket_kernel_dispatch);
        if (cod_op != DESCONEXION)
        {
            t_pcb *pcb_prueba = recibir_pcb(socket_kernel_dispatch);
            log_info(logger, "código de operación: %i", cod_op);
            log_info(logger, "PID: %i", pcb_prueba->pid);
            log_info(logger, "program counter: %i", pcb_prueba->cpu_registers->pc);
            log_info(logger, "quantum: %i", pcb_prueba->quantum);
            log_info(logger, "estado: %i", pcb_prueba->estado);
            log_info(logger, "SI: %i", pcb_prueba->cpu_registers->si);
            log_info(logger, "DI: %i", pcb_prueba->cpu_registers->di);
            log_info(logger, "AX: %i", pcb_prueba->cpu_registers->normales[AX]);
            log_info(logger, "BX: %i", pcb_prueba->cpu_registers->normales[BX]);
            log_info(logger, "CX: %i", pcb_prueba->cpu_registers->normales[CX]);
            log_info(logger, "DX: %i", pcb_prueba->cpu_registers->normales[DX]);
            log_info(logger, "EAX: %i", pcb_prueba->cpu_registers->extendidos[EAX]);
            log_info(logger, "EBX: %i", pcb_prueba->cpu_registers->extendidos[EBX]);
            log_info(logger, "ECX: %i", pcb_prueba->cpu_registers->extendidos[ECX]);
            log_info(logger, "EDX: %i", pcb_prueba->cpu_registers->extendidos[EDX]);

            free(pcb_prueba->cpu_registers);
            free(pcb_prueba);
        } else {
            el_kernel_sigue_conectado = 0;
        }
    }
}