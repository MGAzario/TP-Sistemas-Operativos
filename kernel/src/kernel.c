#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_cliente.h>
#include <utils/utils_server.h>
#include <utils/hello.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <dispatch.h>
#include <pthread.h>
#include <utils/utils.h>
#include <kernel.h>

static int ultimo_pid = 0;
pthread_t hilo_planificador_largo_plazo;

int main(int argc, char *argv[])
{
    decir_hola("Kernel");
    sem_init(&sem_nuevo_pcb, 0, 0);
    sem_init(&sem_proceso_liberado, 0, 0);
    crear_logger();
    crear_config();
    // El grado de multiprogramacion va a ser una variable global, que van a manejar entre CPU y Kernel
    grado_multiprogramacion_max = atoi(config_get_string_value(config, "GRADO_MULTIPROGRAMACION"));
    // El grado activo empieza en 0 y se ira incrementando
    grado_multiprogramacion_activo = 0;

    // Creo la cola que voy a usar para guardar mis PCBs
    cola_new = queue_create();
    cola_ready = queue_create();

    // Creo un hilo para el planificador de largo plazo
    if (pthread_create(&hilo_planificador_largo_plazo, NULL, planificador_largo_plazo,NULL) != 0)
    {
        log_error(logger, "Error al inicializar el Hilo Planificador de Largo Plazo");
        exit(EXIT_FAILURE);
    }
    // Este hilo debe ser independiente dado que el planificador nunca se debe apagar.
    pthread_detach(hilo_planificador_largo_plazo);
    // Creo un PCB de Quantum 2
    crear_pcb(2);
    crear_pcb(1);
    mover_procesos_ready();

    planificador_corto_plazo();
    // Usan la misma IP, se las paso por parametro.
    // Conexiones con CPU: Dispatch e Interrupt
    ip_cpu = config_get_string_value(config, "IP_CPU");

    conectar_interrupt_cpu(ip_cpu);

    // Kernel a Memoria
    conectar_memoria();

    // Entrada salida a Kernel
    recibir_entradasalida();

    log_info(logger, "Terminó\n");

    return 0;
}

void crear_logger()
{
    logger = log_create("./kernel.log", "LOG_KERNEL", true, LOG_LEVEL_INFO);
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

void conectar_interrupt_cpu(char *ip_cpu)
{
    // Establecer conexión con el módulo CPU (interrupt)
    char *puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
    int socket_cpu_interrupt = conectar_modulo(ip_cpu, puerto_cpu_interrupt);
    if (socket_cpu_interrupt != -1)
    {
        enviar_mensaje("Mensaje al CPU desde el Kernel por interrupt", socket_cpu_interrupt);
        liberar_conexion(socket_cpu_interrupt);
    }
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
void crear_pcb(int quantum)
{
    PCB *pcb = malloc(sizeof(PCB));
    // Incrementar el PID y asignarlo al nuevo PCB
    ultimo_pid++;
    pcb->PID = ultimo_pid;
    pcb->program_counter = 0;
    pcb->quantum = quantum;
    pcb->estado = NEW;
    // El PCB se agrega a la cola de los procesos NEW
    queue_push(cola_new, pcb);
    // Crear un buffer para almacenar el mensaje con el último PID
    log_info(logger, "Se crea el proceso %d en NEW\n", ultimo_pid);
    // Despertar el mover procesos a ready
    sem_post(&sem_nuevo_pcb);
}

/*En caso de que el grado de multiprogramación lo permita, los procesos creados podrán pasar de la cola de NEW a la cola de READY,
caso contrario, se quedarán a la espera de que finalicen procesos que se encuentran en ejecución*/
void planificador_largo_plazo()
{
    while (1)
    {
        // Esperar hasta que se cree un nuevo PCB
        sem_wait(&sem_nuevo_pcb);
        // Calcular la cantidad de procesos en la cola de NEW y en la cola de READY
        int cantidad_procesos_new = queue_size(cola_new);

        // Verificar si el grado de multiprogramación lo permite
        // Mover procesos de la cola de NEW a la cola de READY
        while (cantidad_procesos_new > 0 && grado_multiprogramacion_activo < grado_multiprogramacion_max)
        {
            // Seleccionar el primer proceso de la cola de NEW
            PCB *proceso_nuevo = queue_peek(cola_new);
            // Borro el proceso que acabo de seleccionar
            queue_pop(cola_new);

            // Cambiar el estado del proceso a READY
            proceso_nuevo->estado = READY;

            // Agregar el proceso a la cola de READY
            queue_push(cola_ready, proceso_nuevo);
            // Aumentamos el grado de multiprogramacion activo
            grado_multiprogramacion_activo++;

            // Reducir la cantidad de procesos en la cola de NEW
            cantidad_procesos_new--;
        }
        // Este semaforo se usaria para que la CPU no reste al grado de multiprogramacion al mismo tiempo que se modifica aca.
        sem_post(&sem_proceso_liberado);
    }
}

// Planificador corto plazo
void planificador_corto_plazo()
{
    char *algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    if (strcmp(algoritmo_planificacion, "FIFO") == 0)
    {
        // Código para el caso FIFO
        planificar_fifo();
    }
    else if (strcmp(algoritmo_planificacion, "RR") == 0)
    {
        // Código para el caso RR
        planificar_round_robin();
    }
    else if (strcmp(algoritmo_planificacion, "VRR") == 0)
    {
        // Código para el caso VRR
        planificar_vrr();
    }
    else
    {
        // Manejo para un valor no reconocido
    }
}
void planificar_fifo()
{
    while (1)
    {
        // Esperar a que se cree un nuevo PCB
        sem_wait(&sem_nuevo_pcb);
        if (!queue_is_empty(cola_ready))
        {
            // FIFO toma el primer proceso de la cola
            PCB *proceso_ejecucion = queue_peek(cola_ready);
            // Borro ese proceso de mi cola de ready
            queue_pop(cola_ready);
            // Estado pasa a ejecucion
            proceso_ejecucion->estado = EXEC;
            // Me conecto al CPU a traves del dispatch
            conectar_dispatch_cpu(proceso_ejecucion);
        }
    }
}

// Función para planificar los procesos usando Round Robin
void planificar_round_robin()
{
    int quantum_kernel = atoi(config_get_string_value(config, "QUANTUM"));
    // Ejecutar hasta que la cola de procesos en READY esté vacía
    while (1)
    {
        if (!queue_is_empty(cola_ready))
        {
            int tiempo_usado = 0;
            // Obtener el proceso listo para ejecutarse de la cola
            PCB *proceso_ejecucion = queue_peek(cola_ready);
            queue_pop(cola_ready);

            // Cambiar el estado del proceso a EXEC
            proceso_ejecucion->estado = EXEC;

            // TODO Enviar el proceso a la CPU
            conectar_dispatch_cpu(proceso_ejecucion);
            // Si el proceso que envie tiene quantum, voy a chequear cuando tengo que decirle al CPU que corte
            // Tambien lo podriamos hacer del lado de CPU, pero funcionalmente no estaria OK.
            while (proceso_ejecucion->quantum > 0)
            {
                if (tiempo_usado < quantum_kernel)
                {
                    proceso_ejecucion->quantum--;
                    tiempo_usado++;
                }
                // Aca el proceso cortaria porque se le agoto su quantum
                else if (tiempo_usado == quantum_kernel)
                {
                    // TODO, desalojar de CPU con interrupcion.
                    proceso_ejecucion->estado = READY;
                    queue_push(cola_ready, proceso_ejecucion);
                    break;
                }
            }
        }
    }
}