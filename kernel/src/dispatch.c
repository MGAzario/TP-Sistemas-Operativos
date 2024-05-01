#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_cliente.h>
#include <utils/utils_server.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <kernel.h>

// Requisito Checkpoint: Es capaz de enviar un proceso a la CPU
/*Una vez seleccionado el siguiente proceso a ejecutar, se lo transicionará al estado EXEC y se enviará su Contexto de Ejecución al CPU
a través del puerto de dispatch, quedando a la espera de recibir dicho contexto actualizado después de la ejecución,
junto con un motivo de desalojo por el cual fue desplazado a manejar.*/
extern t_config *config;
extern t_log *logger;

int iniciar_dispatch_cpu()
{
    // Establecer conexión con el módulo CPU (dispatch)
    char *puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    char *ip_cpu = config_get_string_value(config, "IP_CPU");
    int socket_cpu_dispatch = conectar_modulo(ip_cpu, puerto_cpu_dispatch);
    return socket_cpu_dispatch;
}

void enviar_pcb(int socket_cliente, PCB *pcb)
{
    // Crear un paquete para enviar los PCBs
    t_paquete* paquete = crear_paquete_pcb();
    //Agrega el PCB a enviar
    agregar_pcb_a_paquete(paquete,pcb);
    //Lo envia a traves de la conexion
    enviar_paquete_pcb(paquete, socket_cliente);

}

void conectar_dispatch_cpu(PCB *proceso_enviar)
{
    // Prendo la conexion al dispatch, creando un socket
    int socket_cpu_dispatch = iniciar_dispatch_cpu();
    // Envio el PCB del proceso que pase a ejecucion en la planificacion
    enviar_pcb(socket_cpu_dispatch,proceso_enviar);
    // Libero la conexion para que se pueda volver a usar.
    liberar_conexion(socket_cpu_dispatch);
}