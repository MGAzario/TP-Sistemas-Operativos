#include "kernel.h"

// Estas variables las cargo como globales porque las uso en varias funciones, no se si a nivel codigo es lo correcto.
t_config *config;
t_log *logger;
t_queue *cola_new;
t_queue *cola_ready;

static int ultimo_pid = 0;
char *ip_cpu;
int socket_cpu_dispatch;
int socket_cpu_interrupt;
int grado_multiprogramacion_activo;
int grado_multiprogramacion_max;

char *algoritmo_planificacion;

sem_t sem_nuevo_pcb;
sem_t sem_proceso_ready;

pthread_t hilo_planificador_largo_plazo;
pthread_t hilo_planificador_corto_plazo;

int main(int argc, char *argv[])
{
    sem_init(&sem_nuevo_pcb, 0, 0);
    sem_init(&sem_proceso_ready, 0, 0);

    crear_logger();
    crear_config();

    decir_hola("Kernel");

    // El grado de multiprogramacion va a ser una variable global, que van a manejar entre CPU y Kernel
    grado_multiprogramacion_max = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
    // El grado activo empieza en 0 y se ira incrementando
    grado_multiprogramacion_activo = 0;

    // Creo la cola que voy a usar para guardar mis PCBs
    cola_new = queue_create();
    cola_ready = queue_create();

    // Usan la misma IP, se las paso por parametro.
    // Conexiones con CPU: Dispatch e Interrupt
    ip_cpu = config_get_string_value(config, "IP_CPU");
    conectar_dispatch_cpu(ip_cpu);
    conectar_interrupt_cpu(ip_cpu);

    algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");

    //Creo un hilo para el planificador de largo plazo
    if (pthread_create(&hilo_planificador_largo_plazo, NULL, planificador_largo_plazo, NULL) != 0){
        log_error(logger, "Error al inicializar el Hilo Planificador de Largo Plazo");
        exit(EXIT_FAILURE);
    }
    //Este hilo debe ser independiente dado que el planificador nunca se debe apagar.
    pthread_detach(hilo_planificador_largo_plazo);
    //Creo un hilo para el planificador de corto plazo
    if (pthread_create(&hilo_planificador_corto_plazo, NULL, planificador_corto_plazo, NULL) != 0){
        log_error(logger, "Error al inicializar el Hilo Planificador de Corto Plazo");
        exit(EXIT_FAILURE);
    }
    //Este hilo debe ser independiente dado que el planificador nunca se debe apagar.
    pthread_detach(hilo_planificador_corto_plazo);

    consola();

    // Kernel a Memoria
    // conectar_memoria();

    // Entrada salida a Kernel
    // recibir_entradasalida();

    liberar_conexion(socket_cpu_dispatch);
    liberar_conexion(socket_cpu_interrupt);

    log_info(logger, "Terminó");

    return 0;
}

void crear_logger()
{
    logger = log_create("./kernel.log", "LOG_KERNEL", false, LOG_LEVEL_TRACE);
    if (logger == NULL)
    {
        perror("Ocurrió un error al leer el archivo de Log de Kernel");
        abort();
    }
}

void crear_config()
{
    config = config_create("./kernel.config");
    if (config == NULL)
    {
        log_error(logger, "Ocurrió un error al leer el archivo de Configuración del Kernel\n");
        abort();
    }
}

void conectar_memoria()
{
    // Establecer conexión con el módulo Memoria
    char *ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char *puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    int socket_memoria = conectar_modulo(ip_memoria, puerto_memoria);
    if (socket_memoria != -1)
    {
        enviar_mensaje("Mensaje a la Memoria desde el Kernel", socket_memoria);
        liberar_conexion(socket_memoria);
    }
}

void conectar_dispatch_cpu(char *ip_cpu)
{
    // Establecer conexión con el módulo CPU (dispatch)
    char *puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    socket_cpu_dispatch = conectar_modulo(ip_cpu, puerto_cpu_dispatch);
}

void conectar_interrupt_cpu(char *ip_cpu)
{
    // Establecer conexión con el módulo CPU (interrupt)
    char *puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
    socket_cpu_interrupt = conectar_modulo(ip_cpu, puerto_cpu_interrupt);
}

void recibir_entradasalida()
{
    char *puerto_kernel = config_get_string_value(config, "PUERTO_ESCUCHA");
    int socket_kernel = iniciar_servidor(puerto_kernel);

    // Espero a un cliente (entradasalida). El mensaje entiendo que se programa despues
    int socket_entradasalida = esperar_cliente(socket_kernel);

    // Si falla, no se pudo aceptar
    if (socket_entradasalida == -1)
    {
        log_info(logger, "Error al aceptar la conexión del kernel asl socket de dispatch.\n");
        liberar_conexion(socket_kernel);
    }
    // Esto deberia recibir el mensaje que manda la entrada salida
    recibir_mensaje(socket_entradasalida);

    // Cerrar conexión con el cliente
    liberar_conexion(socket_entradasalida);

    // Cerrar socket servidor
    liberar_conexion(socket_kernel);
}

// Requisito Checkpoint: Es capaz de crear un PCB y planificarlo por FIFO y RR.
void crear_pcb()
{
    t_pcb *pcb = malloc(sizeof(t_pcb));
    // Registros. Asumo que se inicializan todos en cero
    t_cpu_registers *registros = malloc(sizeof(t_cpu_registers));
    registros->pc = 0;
    registros->normales[AX] = 0;
    registros->normales[BX] = 0;
    registros->normales[CX] = 0;
    registros->normales[DX] = 0;
    registros->extendidos[EAX] = 0;
    registros->extendidos[EBX] = 0;
    registros->extendidos[ECX] = 0;
    registros->extendidos[EDX] = 0;
    registros->si = 0;
    registros->di= 0;
    // Incrementar el PID y asignarlo al nuevo PCB
    ultimo_pid++;
    pcb->pid = ultimo_pid;
    // Termino de completar el PCB
    pcb->cpu_registers = registros;
    pcb->quantum = 0;
    pcb->estado = NEW;
    // El PCB se agrega a la cola de los procesos NEW
    queue_push(cola_new, pcb);
    // // Crear un buffer para almacenar el mensaje con el último PID
    // char mensaje[100];
    // // Usar sprintf para formatear el mensaje con el último PID
    // sprintf(mensaje, "Se crea el proceso %d en NEW\n", ultimo_pid);
    // log_info(logger, mensaje);
    // Despertar el mover procesos a ready
    sem_post(&sem_nuevo_pcb);
}

/*En caso de que el grado de multiprogramación lo permita, los procesos creados podrán pasar de la cola de NEW a la cola de READY,
caso contrario, se quedarán a la espera de que finalicen procesos que se encuentran en ejecución*/
void *planificador_largo_plazo()
{
    while (1)
    {
        // Esperar hasta que se cree un nuevo PCB
        log_trace(logger, "PLP: Esperando nuevo proceso");
        sem_wait(&sem_nuevo_pcb);
        log_trace(logger, "PLP: Llegó un nuevo proceso");

        // Verificar si el grado de multiprogramación lo permite
        // Mover procesos de la cola de NEW a la cola de READY
        if(grado_multiprogramacion_activo < grado_multiprogramacion_max)
        {
            log_trace(logger, "El grado de multiprogramación permite que un proceso entre a ready");
            // Tomar el primer proceso de la cola de NEW
            t_pcb *proceso_nuevo = queue_pop(cola_new);

            // Cambiar el estado del proceso a READY
            proceso_nuevo->estado = READY;

            // Agregar el proceso a la cola de READY
            queue_push(cola_ready, proceso_nuevo);
            sem_post(&sem_proceso_ready);
            //Aumentamos el grado de multiprogramacion activo
            grado_multiprogramacion_activo++;

            // Reducir la cantidad de procesos en la cola de NEW
            log_trace(logger, "PLP: El proceso fue enviado a ready");
        }
    }
}

// Planificador corto plazo
void *planificador_corto_plazo()
{
    while(1)
    {
        // Esperar a que se cree un nuevo PCB
        log_trace(logger, "PCP: Esperando que llegue un proceso a ready");
        sem_wait(&sem_proceso_ready);
        log_trace(logger, "PCP: Un proceso entró a ready");

        if(strcmp(algoritmo_planificacion, "FIFO") == 0)
        {
            planificar_fifo();
        } else if(strcmp(algoritmo_planificacion, "RR") == 0){
            planificar_round_robin();
        } else if(strcmp(algoritmo_planificacion, "VRR") == 0){
            planificar_vrr();
        } else {
            log_error(logger, "Algoritmo de planificación desconocido");
            exit(EXIT_FAILURE);
        }
    }
}

// Función para planificar los procesos usando FIFO
void planificar_fifo()
{
    if (!queue_is_empty(cola_ready))
    {
        // FIFO toma el primer proceso de la cola
        t_pcb *proceso_ejecucion = queue_pop(cola_ready);

        log_debug(logger, "CX: %i", proceso_ejecucion->cpu_registers->normales[CX]);

        // Estado pasa a ejecucion
        // proceso_ejecucion->estado = EXEC;
        // Le envío el pcb al CPU a traves del dispatch
        enviar_pcb(socket_cpu_dispatch, proceso_ejecucion);
        // Espero a que el CPU me devuelva el proceso
        esperar_cpu();
    }
}

// Función para planificar los procesos usando Round Robin
void planificar_round_robin()
{
    // int quantum_kernel = config_get_string_value(config, "QUANTUM");
    // // Ejecutar hasta que la cola de procesos en READY esté vacía
    // while (1)
    // {
    //     if (!queue_is_empty(cola_ready))
    //     {
    //         int tiempo_usado = 0;
    //         // Obtener el proceso listo para ejecutarse de la cola
    //         t_pcb *proceso_ejecucion = queue_peek(cola_ready);
    //         queue_pop(cola_ready);

    //         // Cambiar el estado del proceso a EXEC
    //         proceso_ejecucion->estado = EXEC;

    //         // TODO Enviar el proceso a la CPU
    //         conectar_dispatch_cpu(proceso_ejecucion);
    //         // Si el proceso que envie tiene quantum, voy a chequear cuando tengo que decirle al CPU que corte
    //         // Tambien lo podriamos hacer del lado de CPU, pero funcionalmente no estaria OK.
    //         while (proceso_ejecucion->quantum > 0)
    //         {
    //             if (tiempo_usado < quantum_kernel)
    //             {
    //                 proceso_ejecucion->quantum--;
    //                 tiempo_usado++;
    //             }
    //             // Aca el proceso cortaria porque se le agoto su quantum
    //             else if (tiempo_usado == quantum_kernel)
    //             {
    //                 // TODO, desalojar de CPU con interrupcion.
    //                 proceso_ejecucion->estado = READY;
    //                 queue_push(cola_ready, proceso_ejecucion);
    //                 break;
    //             }
    //         }
    //     }
    // }
}

void planificar_vrr()
{
    // TODO
}

void esperar_cpu()
{
    log_trace(logger, "PCP: Esperando respuesta del CPU");
    op_code cod_op = recibir_operacion(socket_cpu_dispatch);
    log_trace(logger, "PCP: Llegó una respuesta del CPU");

    switch (cod_op)
    {
        case DISPATCH: // op_code genérico solo para probar, después hay que ver todos los op_code que hagan falta
            log_debug(logger, "Recibí una respuesta del CPU!");
            if (1)
            {
                t_pcb *pcb_prueba = recibir_pcb(socket_cpu_dispatch);

                log_info(logger, "AX: %i", pcb_prueba->cpu_registers->normales[AX]);
                log_info(logger, "BX: %i", pcb_prueba->cpu_registers->normales[BX]);

                free(pcb_prueba->cpu_registers);
                free(pcb_prueba);
            }
            while(1){}
            break;
        default:
            log_debug(logger, "Paquete desconocido\n");
            break;
    } 


}

void consola()
{
    char *linea;
    while(1)
    {
        linea = readline("> ");

        char comando[30];
        sscanf(linea, "%s", comando);

        if(strcmp(comando, "COMANDOS") == 0){
            printf("EJECUTAR_SCRIPT [PATH]\n");
            printf("INICIAR_PROCESO [PATH]\n");
            printf("FINALIZAR_PROCESO [PID]\n");
            printf("DETENER_PLANIFICACION\n");
            printf("INICIAR_PLANIFICACION\n");
            printf("MULTIPROGRAMACION [VALOR]\n");
            printf("PROCESO_ESTADO\n");
        }
        if(strcmp(comando, "EJECUTAR_SCRIPT") == 0){
            char path[300];
            sscanf(linea, "%s %s", comando, path);

            printf("Falta implementar\n"); // TODO
        }
        if(strcmp(comando, "INICIAR_PROCESO") == 0){
            char path[300];
            sscanf(linea, "%s %s", comando, path);

            printf("Path: %s\n", path); // TODO: Sacar instrucciones del path

            crear_pcb();
        }
        if(strcmp(comando, "FINALIZAR_PROCESO") == 0){
            int pid;
            sscanf(linea, "%s %i", comando, &pid);
            
            printf("Falta implementar\n"); // TODO
        }
        if(strcmp(comando, "DETENER_PLANIFICACION") == 0){
            printf("Falta implementar\n"); // TODO
        }
        if(strcmp(comando, "INICIAR_PLANIFICACION") == 0){
            printf("Falta implementar\n"); // TODO
        }
        if(strcmp(comando, "MULTIPROGRAMACION") == 0){
            int valor;
            sscanf(linea, "%s %i", comando, &valor);

            printf("Falta implementar\n"); // TODO
        }
        if(strcmp(comando, "PROCESO_ESTADO") == 0){
            printf("Falta implementar\n"); // TODO
        }

        free(linea);
    }
}