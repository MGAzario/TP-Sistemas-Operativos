#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_cliente.h>
#include <utils/utils_server.h>
#include <utils/hello.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <pthread.h>
#include "kernel.h"
#include "dispatch.h"

// Estas variables las cargo como globales porque las uso en varias funciones, no se si a nivel codigo es lo correcto.
t_config *config;
t_log *logger;
t_queue *cola_new;
t_queue *cola_ready;

// static int ultimo_pid = 0;
char *ip_cpu;
int socket_cpu_dispatch;
int socket_cpu_interrupt;
int grado_multiprogramacion_activo;
int grado_multiprogramacion_max;

sem_t sem_nuevo_pcb;
sem_t sem_proceso_liberado;

pthread_t hilo_planificador_largo_plazo;

int main(int argc, char *argv[])
{
    // sem_init(&sem_nuevo_pcb, 0, 0);
    // sem_init(&sem_proceso_liberado, 0, 0);
    crear_logger();
    crear_config();

    decir_hola("Kernel");

    //El grado de multiprogramacion va a ser una variable global, que van a manejar entre CPU y Kernel
    // grado_multiprogramacion_max = config_get_string_value(config, "GRADO_MULTIPROGRAMACION");
    //El grado activo empieza en 0 y se ira incrementando
    grado_multiprogramacion_activo = 0;

    // Creo la cola que voy a usar para guardar mis PCBs
    cola_new = queue_create();
    cola_ready = queue_create();

    // Usan la misma IP, se las paso por parametro.
    // Conexiones con CPU: Dispatch e Interrupt
    ip_cpu = config_get_string_value(config, "IP_CPU");
    conectar_dispatch_cpu(ip_cpu);
    conectar_interrupt_cpu(ip_cpu);

    procedimiento_de_prueba();

    // //Creo un hilo para el planificador de largo plazo
    // if (pthread_create(&hilo_planificador_largo_plazo, NULL, planificador_largo_plazo) != 0){
    //     log_error(logger, "Error al inicializar el Hilo Planificador de Largo Plazo");
    //     exit(EXIT_FAILURE);
    // }
    // //Este hilo debe ser independiente dado que el planificador nunca se debe apagar.
    // pthread_detach(hilo_planificador_largo_plazo);
    // // Creo un PCB de Quantum 2
    // crear_pcb(2);
    // crear_pcb(1);
    // mover_procesos_ready();

    // planificador_corto_plazo();

    // // Kernel a Memoria
    // conectar_memoria();

    // // Entrada salida a Kernel
    // recibir_entradasalida();
    // int grado_multiprogramacion_max = config_get_string_value(config, "GRADO_MULTIPROGRAMACION");

    log_info(logger, "Terminó");

    return 0;
}

void crear_logger()
{
    logger = log_create("./kernel.log", "LOG_KERNEL", true, LOG_LEVEL_TRACE);
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
    // if (socket_cpu_interrupt != -1)
    // {
    //     enviar_mensaje("Mensaje al CPU desde el Kernel por interrupt", socket_cpu_interrupt);
    //     liberar_conexion(socket_cpu_interrupt);
    // }
}

// void recibir_entradasalida()
// {
//     char *puerto_kernel = config_get_string_value(config, "PUERTO_ESCUCHA");
//     int socket_kernel = iniciar_servidor(puerto_kernel);

//     // Espero a un cliente (entradasalida). El mensaje entiendo que se programa despues
//     int socket_entradasalida = esperar_cliente(socket_kernel);

//     // Si falla, no se pudo aceptar
//     if (socket_entradasalida == -1)
//     {
//         log_info(logger, "Error al aceptar la conexión del kernel asl socket de dispatch.\n");
//         liberar_conexion(socket_kernel);
//     }
//     // Esto deberia recibir el mensaje que manda la entrada salida
//     recibir_mensaje(socket_entradasalida);

//     // Cerrar conexión con el cliente
//     liberar_conexion(socket_entradasalida);

//     // Cerrar socket servidor
//     liberar_conexion(socket_kernel);
// }

// // Requisito Checkpoint: Es capaz de crear un PCB y planificarlo por FIFO y RR.
// void crear_pcb(int quantum)
// {
//     t_pcb *pcb = malloc(sizeof(t_pcb));
//     if (pcb == NULL)
//     {
//         printf("Error al crear el PCB\n");
//         return NULL;
//     }
//     // Incrementar el PID y asignarlo al nuevo PCB
//     ultimo_pid++;
//     pcb->pid = ultimo_pid;
//     pcb->program_counter = 0;
//     pcb->quantum = quantum;
//     pcb->estado = NEW;
//     // El PCB se agrega a la cola de los procesos NEW
//     queue_push(cola_new, pcb);
//     // Crear un buffer para almacenar el mensaje con el último PID
//     char mensaje[100];
//     // Usar sprintf para formatear el mensaje con el último PID
//     sprintf(mensaje, "Se crea el proceso %d en NEW\n", ultimo_pid);
//     log_info(logger, mensaje);
//     // Despertar el mover procesos a ready
//     sem_post(&sem_nuevo_pcb);
// }

// /*En caso de que el grado de multiprogramación lo permita, los procesos creados podrán pasar de la cola de NEW a la cola de READY,
// caso contrario, se quedarán a la espera de que finalicen procesos que se encuentran en ejecución*/
// void planificador_largo_plazo()
// {
//     while (1)
//     {
//         // Esperar hasta que se cree un nuevo PCB
//         sem_wait(&sem_nuevo_pcb);
//         // Calcular la cantidad de procesos en la cola de NEW y en la cola de READY
//         int cantidad_procesos_new = queue_size(cola_new);

//         // Verificar si el grado de multiprogramación lo permite
//         // Mover procesos de la cola de NEW a la cola de READY
//         while (cantidad_procesos_new > 0 && grado_multiprogramacion_activo < grado_multiprogramacion_max)
//         {
//             // Seleccionar el primer proceso de la cola de NEW
//             t_pcb *proceso_nuevo = queue_peek(cola_new);
//             // Borro el proceso que acabo de seleccionar
//             queue_pop(cola_new);

//             // Cambiar el estado del proceso a READY
//             proceso_nuevo->estado = READY;

//             // Agregar el proceso a la cola de READY
//             queue_push(cola_ready, proceso_nuevo);
//             //Aumentamos el grado de multiprogramacion activo
//             grado_multiprogramacion_activo++;

//             // Reducir la cantidad de procesos en la cola de NEW
//             cantidad_procesos_new--;

//         }
//         //Este semaforo se usaria para que la CPU no reste al grado de multiprogramacion al mismo tiempo que se modifica aca.
//         sem_post(&sem_proceso_liberado);
//     }
// }

// // Planificador corto plazo
// void planificador_corto_plazo()
// {
//     char *algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
//     switch (algoritmo_planificacion)
//     {
//     case 'FIFO':
//         planificar_fifo();
//         break;
//     case 'RR':
//         planificar_round_robin();
//         break;
//     case 'VRR':
//         planificar_vrr();
//         break;
//     default:
//         printf("Algoritmo de planificación desconocido\n");
//         break;
//     }
// }
// void planificar_fifo()
// {
//     while (1)
//     {
//         // Esperar a que se cree un nuevo PCB
//         sem_wait(&sem_nuevo_pcb);
//         if (!queue_is_empty(cola_ready))
//         {
//             // FIFO toma el primer proceso de la cola
//             t_pcb *proceso_ejecucion = queue_peek(cola_ready);
//             // Borro ese proceso de mi cola de ready
//             queue_pop(cola_ready);
//             // Estado pasa a ejecucion
//             proceso_ejecucion->estado = EXEC;
//             // Me conecto al CPU a traves del dispatch
//             conectar_dispatch_cpu(proceso_ejecucion);
//         }
//     }
// }

// // Función para planificar los procesos usando Round Robin
// void planificar_round_robin()
// {
//     int quantum_kernel = config_get_string_value(config, "QUANTUM");
//     // Ejecutar hasta que la cola de procesos en READY esté vacía
//     while (1)
//     {
//         if (!queue_is_empty(cola_ready))
//         {
//             int tiempo_usado = 0;
//             // Obtener el proceso listo para ejecutarse de la cola
//             t_pcb *proceso_ejecucion = queue_peek(cola_ready);
//             queue_pop(cola_ready);

//             // Cambiar el estado del proceso a EXEC
//             proceso_ejecucion->estado = EXEC;

//             // TODO Enviar el proceso a la CPU
//             conectar_dispatch_cpu(proceso_ejecucion);
//             // Si el proceso que envie tiene quantum, voy a chequear cuando tengo que decirle al CPU que corte
//             // Tambien lo podriamos hacer del lado de CPU, pero funcionalmente no estaria OK.
//             while (proceso_ejecucion->quantum > 0)
//             {
//                 if (tiempo_usado < quantum_kernel)
//                 {
//                     proceso_ejecucion->quantum--;
//                     tiempo_usado++;
//                 }
//                 // Aca el proceso cortaria porque se le agoto su quantum
//                 else if (tiempo_usado == quantum_kernel)
//                 {
//                     // TODO, desalojar de CPU con interrupcion.
//                     proceso_ejecucion->estado = READY;
//                     queue_push(cola_ready, proceso_ejecucion);
//                     break;
//                 }
//             }
//         }
//     }
// }

void procedimiento_de_prueba()
{
    t_cpu_registers *registros = malloc(sizeof(t_cpu_registers));
    registros->pc = 10;
    registros->normales[AX] = 8;
    registros->normales[BX] = 8;
    registros->normales[CX] = 8;
    registros->normales[DX] = 8;
    registros->extendidos[EAX] = 21;
    registros->extendidos[EBX] = 21;
    registros->extendidos[ECX] = 21;
    registros->extendidos[EDX] = 21;
    registros->si = 1;
    registros->di= 1;

    t_pcb *pcb = malloc(sizeof(t_pcb));
    if (pcb == NULL)
    {
        printf("Error al crear el PCB\n");
    }

    pcb->pid = 11;
    pcb->quantum = 4;
    pcb->estado = NEW;
    pcb->cpu_registers = registros;

    enviar_pcb(socket_cpu_dispatch, pcb);

    free(pcb);

    sleep(5);

    t_pcb *pcb_2 = malloc(sizeof(t_pcb));
    if (pcb_2 == NULL)
    {
        printf("Error al crear el PCB\n");
    }

    pcb_2->pid = 5;
    pcb_2->quantum = 24;
    pcb_2->estado = NEW;
    pcb_2->cpu_registers = registros;

    enviar_pcb(socket_cpu_dispatch, pcb_2);

    free(pcb_2);

    sleep(5);

    t_pcb *pcb_3 = malloc(sizeof(t_pcb));
    if (pcb_3 == NULL)
    {
        printf("Error al crear el PCB\n");
    }

    pcb_3->pid = 45;
    pcb_3->quantum = 21;
    pcb_3->estado = NEW;
    pcb_3->cpu_registers = registros;

    enviar_pcb(socket_cpu_dispatch, pcb_3);

    free(pcb_3);

    free(registros);

    liberar_conexion(socket_cpu_dispatch);

    sleep(5);
}