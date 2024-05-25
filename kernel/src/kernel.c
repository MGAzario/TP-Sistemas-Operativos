#include "kernel.h"

// Estas variables las cargo como globales porque las uso en varias funciones, no se si a nivel codigo es lo correcto.
t_config *config;
t_log *logger;
t_queue *cola_new;
t_queue *cola_ready;

int ultimo_pid;
char *ip_cpu;
char *ip_memoria;
int socket_cpu_dispatch;
int socket_cpu_interrupt;
int socket_memoria;
int grado_multiprogramacion_activo;
int grado_multiprogramacion_max;

t_pcb *pcb_ejecutandose;

char *algoritmo_planificacion;

sem_t sem_nuevo_pcb;
sem_t sem_proceso_ready;
sem_t sem_round_robin;

pthread_t hilo_planificador_largo_plazo;
pthread_t hilo_planificador_corto_plazo;

int main(int argc, char *argv[])
{
    sem_init(&sem_nuevo_pcb, 0, 0);
    sem_init(&sem_proceso_ready, 0, 0);
    sem_init(&sem_round_robin, 0, 1);

    crear_logger();
    crear_config();

    decir_hola("Kernel");

    // El grado de multiprogramacion va a ser una variable global, que van a manejar entre CPU y Kernel
    grado_multiprogramacion_max = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
    // El grado activo empieza en 0 y se ira incrementando
    grado_multiprogramacion_activo = 0;

    // Creo las colas que voy a usar para guardar mis PCBs
    cola_new = queue_create();
    cola_ready = queue_create();

    // Usan la misma IP, se las paso por parametro.
    // Conexiones con CPU: Dispatch e Interrupt
    ip_cpu = config_get_string_value(config, "IP_CPU");
    conectar_dispatch_cpu(ip_cpu);
    conectar_interrupt_cpu(ip_cpu);

    // Kernel a Memoria
    conectar_memoria();

    // Entrada salida a Kernel
    // recibir_entradasalida();

    algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");

    // Creo un hilo para el planificador de largo plazo
    if (pthread_create(&hilo_planificador_largo_plazo, NULL, planificador_largo_plazo, NULL) != 0){
        log_error(logger, "Error al inicializar el Hilo Planificador de Largo Plazo");
        exit(EXIT_FAILURE);
    }
    // Este hilo debe ser independiente dado que el planificador nunca se debe apagar.
    pthread_detach(hilo_planificador_largo_plazo);
    // Creo un hilo para el planificador de corto plazo
    if (pthread_create(&hilo_planificador_corto_plazo, NULL, planificador_corto_plazo, NULL) != 0){
        log_error(logger, "Error al inicializar el Hilo Planificador de Corto Plazo");
        exit(EXIT_FAILURE);
    }
    // Este hilo debe ser independiente dado que el planificador nunca se debe apagar.
    pthread_detach(hilo_planificador_corto_plazo);

    ultimo_pid = 0;

    consola();

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
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char *puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    socket_memoria = conectar_modulo(ip_memoria, puerto_memoria);
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
void crear_pcb(char *path)
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
    // Asignar el PID e incrementarlo
    pcb->pid = ultimo_pid;
    ultimo_pid++;
    // Termino de completar el PCB
    pcb->cpu_registers = registros;
    pcb->quantum = 0;
    pcb->estado = NEW;

    // Informar a la memoria
    enviar_creacion_proceso(socket_memoria, pcb, path);
    log_trace(logger, "Estoy esperando el OK de la memoria");

    // Esperar recibir OK de la memoria
    op_code cod_op = recibir_operacion(socket_memoria);
    if(cod_op != CREACION_PROCESO_OK)
    {
        log_error(logger, "La memoria mandó otra cosa en lugar de un OK al solicitarle crear el proceso de PID %i", pcb->pid);
        return;
    }
    recibir_ok(socket_memoria);
    log_trace(logger, "Recibí OK de la memoria");

    // El PCB se agrega a la cola de los procesos NEW
    queue_push(cola_new, pcb);
    log_info(logger, "Se crea el proceso %d en NEW", ultimo_pid);
    log_debug(logger, "Hay %i procesos en NEW", queue_size(cola_new));
    // Despertar el mover procesos a ready
    sem_post(&sem_nuevo_pcb);
}

void encontrar_y_eliminar_proceso(int pid_a_eliminar)
{
    // Buscamos en EXEC
    if(pcb_ejecutandose->pid == pid_a_eliminar)
    {
        enviar_interrupcion(socket_cpu_interrupt, pcb_ejecutandose, FINALIZAR_PROCESO);
        return;
    }

    // Buscamos en la cola de NEW
    for(int i = 0; i < list_size(cola_new->elements); i++)
    {
        t_pcb *pcb = (t_pcb *)list_get(cola_new->elements, i);
        if(pcb->pid == pid_a_eliminar)
        {
            list_remove(cola_new->elements, i);
            eliminar_proceso(pcb);
            return;
        }
    }

    // Buscamos en la cola de READY
    for(int i = 0; i < list_size(cola_ready->elements); i++)
    {
        t_pcb *pcb = (t_pcb *)list_get(cola_ready->elements, i);
        if(pcb->pid == pid_a_eliminar)
        {
            list_remove(cola_ready->elements, i);
            eliminar_proceso(pcb);
            return;
        }
    }

    // TODO: Buscar también en la cola (o colas) de bloqueados
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
            log_debug(logger, "Hay %i procesos en READY", queue_size(cola_ready));
            //Aumentamos el grado de multiprogramacion activo
            //TODO poner MUTEX 
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
        } else if(strcmp(algoritmo_planificacion, "RR") == 0)
        {
            planificar_round_robin();
        } else if(strcmp(algoritmo_planificacion, "VRR") == 0)
        {
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
        t_pcb *proceso_a_ejecutar = queue_pop(cola_ready);

        log_trace(logger, "CX: %i", proceso_a_ejecutar->cpu_registers->normales[CX]);

        // Estado pasa a ejecucion
        proceso_a_ejecutar->estado = EXEC;
        
        // Le envío el pcb al CPU a traves del dispatch
        enviar_pcb(socket_cpu_dispatch, proceso_a_ejecutar);

        // Guardamos registro en el Kernel de qué proceso está ejecutándose
        pcb_ejecutandose = proceso_a_ejecutar;

        // Espero a que el CPU me devuelva el proceso
        esperar_cpu();
    }
}

// Función para planificar los procesos usando Round Robin

void planificar_round_robin()
{
	pthread_t quantum_thread;
    int quantum = config_get_int_value(config, "QUANTUM");

	
	sem_wait(&sem_round_robin);
    //desalojo de CPU
    // pensar si sería mejor un semáforo que controle las colas TEORÍA DE SINCRO

    // Ahora mismo, hasta que no se termine el quantum, si un proceso finaliza, el siguiente no se ejecuta.
    if (!queue_is_empty(cola_ready))
    {
        // Obtener el proceso listo para ejecutarse de la cola
        t_pcb *proceso_a_ejecutar = queue_pop(cola_ready);
        
        //wait(sem_mutex_interrupt)
        enviar_interrupcion(socket_cpu_interrupt, pcb_ejecutandose, FIN_DE_QUANTUM);
        esperar_cpu();
        //sem_post(sem_mutex_interrupt)

        // Cambiar el estado del proceso a EXEC
        proceso_a_ejecutar->estado = EXEC;

        enviar_pcb(socket_cpu_dispatch, proceso_a_ejecutar);

        pthread_create(&quantum_thread, NULL, (void*) quantum_count, (void*) quantum);

        pcb_ejecutandose = proceso_a_ejecutar;

        // Si el proceso que envie tiene quantum, voy a chequear cuando tengo que decirle al CPU que corte
        // Tambien lo podriamos hacer del lado de CPU, pero funcionalmente no estaria OK.
        // TODO, desalojar de CPU con interrupcion. ¿Ya está hecho?

    }
    //mutex en la cola de interrupt
}

void quantum_count(int* quantum)
{
	usleep(*quantum * 1000);
	sem_post(&sem_round_robin);
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
        case DISPATCH: // este case es solo una prueba
            log_debug(logger, "Recibí una respuesta del CPU!");
            if (1)
            {
                t_pcb *pcb_prueba = recibir_pcb(socket_cpu_dispatch);

                log_info(logger, "AX: %i", pcb_prueba->cpu_registers->normales[AX]);
                log_info(logger, "BX: %i", pcb_prueba->cpu_registers->normales[BX]);

                free(pcb_prueba->cpu_registers);
                free(pcb_prueba);
            }
            // while(1){}
            // esperar_cpu();
            break;
        case INSTRUCCION_EXIT:
            log_debug(logger, "El CPU informa que le llegó una instrucción EXIT");
            t_pcb *pcb = recibir_pcb(socket_cpu_dispatch);
            eliminar_proceso(pcb);
            break;
        case INTERRUPCION:
            log_debug(logger, "Al CPU el llegó una interrupción, así que envía el pcb junto con el motivo de la interrupción");
            t_interrupcion *interrupcion = recibir_interrupcion(socket_cpu_dispatch);

            // después se puede cambiar este if por un switch cuando haya varios motivos de interrupción
            if(interrupcion->motivo == FINALIZAR_PROCESO)
            {
                log_debug(logger, "Este motivo es FINALIZAR_PROCESO, entonces se elimina el proceso");
                eliminar_proceso(interrupcion->pcb);
            }
            if(interrupcion->motivo == FIN_DE_QUANTUM)
            {
                if(strcmp(algoritmo_planificacion,"RR"))
                {
                    log_debug(logger, "Este motivo es FIN_DE_QUANTUM con RR, entonces se agrega al final de la cola el proceso");
                    interrupcion->pcb->estado = READY;
                    queue_push(cola_ready,interrupcion->pcb);
                    sem_post(&sem_proceso_ready);
                }
                else
                {
                    // TODO: VRR
                }
            }
            break;
        default:
            log_warning(logger, "Mensaje desconocido del CPU");
            break;
    } 


}

void eliminar_proceso(t_pcb *pcb)
{
    // TODO: Informarle a la Memoria la eliminación del proceso, para que pueda liberar sus estructuras

    free(pcb->cpu_registers);
    free(pcb);

    // Se habilita un espacio en el grado de multiprogramación
    //TODO poner MUTEX
    grado_multiprogramacion_activo--;
}

void consola()
{
    char *linea;
    while(1)
    {
        linea = readline("> ");

        char comando[30];
        sscanf(linea, "%s", comando);

        if(strcmp(comando, "COMANDOS") == 0)
        {
            printf("EJECUTAR_SCRIPT [PATH]\n");
            printf("INICIAR_PROCESO [PATH]\n");
            printf("FINALIZAR_PROCESO [PID]\n");
            printf("DETENER_PLANIFICACION\n");
            printf("INICIAR_PLANIFICACION\n");
            printf("MULTIPROGRAMACION [VALOR]\n");
            printf("PROCESO_ESTADO\n");
        }
        else if((strcmp(comando, "EJECUTAR_SCRIPT") == 0) || (strcmp(comando, "1") == 0))
        {
            char path[300];
            sscanf(linea, "%s %s", comando, path);

            printf("Falta implementar\n"); // TODO
        }
        else if((strcmp(comando, "INICIAR_PROCESO") == 0) || (strcmp(comando, "2") == 0))
        {
            char path[300];
            sscanf(linea, "%s %s", comando, path);

            crear_pcb(path);
        }
        else if((strcmp(comando, "FINALIZAR_PROCESO") == 0) || (strcmp(comando, "3") == 0))
        {
            int pid;
            sscanf(linea, "%s %i", comando, &pid);
            
            encontrar_y_eliminar_proceso(pid);
        }
        else if((strcmp(comando, "DETENER_PLANIFICACION") == 0) || (strcmp(comando, "4") == 0))
        {
            printf("Falta implementar\n"); // TODO
        }
        else if((strcmp(comando, "INICIAR_PLANIFICACION") == 0) || (strcmp(comando, "5") == 0))
        {
            printf("Falta implementar\n"); // TODO
        }
        else if((strcmp(comando, "MULTIPROGRAMACION") == 0) || (strcmp(comando, "6") == 0))
        {
            int valor;
            sscanf(linea, "%s %i", comando, &valor);

            printf("Falta implementar\n"); // TODO
        }
        else if((strcmp(comando, "PROCESO_ESTADO") == 0) || (strcmp(comando, "7") == 0))
        {
            printf("Falta implementar\n"); // TODO
        }
        else
        {
            printf("Comando inválido\nEscriba COMANDOS para ver los comandos disponibles\n");
        }

        free(linea);
    }
}