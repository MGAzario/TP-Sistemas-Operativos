#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_cliente.h>
#include <utils/utils_server.h>
#include <utils/hello.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <kernel.h>

t_config *config;
t_log* logger;
t_queue cola_new;
t_queue cola_ready;

static int ultimo_pid = 0;
//Estas variables las cargo como globales porque las uso en varias funciones, no se si a nivel codigo es lo correcto.
char *algoritmo_planificacion;
char *ip_cpu;
int socket_cpu_dispatch;

int main(int argc, char* argv[]) {
    decir_hola("Kernel");
    
    crear_logger();
    crear_config();

    //Creo la cola que voy a usar para guardar mis PCBs
    cola_new = queue_create();
    cola_ready = queue_create();

    

    //Usan la misma IP, se las paso por parametro.
    //Conexiones con CPU: Dispatch e Interrupt
    algoritmo_planificacion = config_get_string_value(config,"ALGORITMO_PLANIFICACION");
    ip_cpu = config_get_string_value(config, "IP_CPU");

    conectar_dispatch_cpu(ip_cpu);
    conectar_interrupt_cpu(ip_cpu);
    
    //Kernel a Memoria
    conectar_memoria();

    //Entrada salida a Kernel
    recibir_entradasalida();

    

    log_info(logger,"Terminó\n");
    
    return 0;
}

void crear_logger(){
    logger = log_create("./kernel.log","LOG_KERNEL",true,LOG_LEVEL_INFO);
    if(logger == NULL){
		perror("Ocurrió un error al leer el archivo de Log de Kernel");
		abort();
	}
}

void crear_config(){
    config = config_create("./kernel.config");
    if (config == NULL)
    {
        log_error(logger,"Ocurrió un error al leer el archivo de Configuración del Kernel\n");
        abort();
    }
}

void conectar_memoria(){
    // Establecer conexión con el módulo Memoria
    char *ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char *puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    int socket_memoria = conectar_modulo(ip_memoria, puerto_memoria);
    if (socket_memoria != -1) {
        enviar_mensaje("Mensaje a la Memoria desde el Kernel", socket_memoria);
        liberar_conexion(socket_memoria);
    }
}

void conectar_interrupt_cpu(char* ip_cpu){
    // Establecer conexión con el módulo CPU (interrupt)
    char *puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
    int socket_cpu_interrupt = conectar_modulo(ip_cpu, puerto_cpu_interrupt);
    if (socket_cpu_interrupt != -1) {
        enviar_mensaje("Mensaje al CPU desde el Kernel por interrupt", socket_cpu_interrupt);
        liberar_conexion(socket_cpu_interrupt);
    }
}

void recibir_entradasalida(){
    char *puerto_kernel= config_get_string_value(config, "PUERTO_ESCUCHA");
    int socket_kernel = iniciar_servidor(puerto_kernel);

    // Espero a un cliente (entradasalida). El mensaje entiendo que se programa despues
    int socket_entradasalida = esperar_cliente(socket_kernel);

    //Si falla, no se pudo aceptar
    if (socket_entradasalida == -1) {
        log_info(logger,"Error al aceptar la conexión del kernel asl socket de dispatch.\n");
        liberar_conexion(socket_kernel);
    }
    //Esto deberia recibir el mensaje que manda la entrada salida
    recibir_mensaje(socket_entradasalida);

    // Cerrar conexión con el cliente
    liberar_conexion(socket_entradasalida);

    // Cerrar socket servidor
    liberar_conexion(socket_kernel);
}

//Requisito Checkpoint: Es capaz de crear un PCB y planificarlo por FIFO y RR.
void crear_pcb(int quantum) {
    PCB* pcb = malloc(sizeof(PCB));
    if (pcb == NULL) {
        printf("Error al crear el PCB\n");
        return NULL;
    }
    // Incrementar el PID y asignarlo al nuevo PCB
    ultimo_pid++;
    pcb->PID = ultimo_pid;
    pcb->program_counter = 0;
    pcb->quantum = quantum;
    pcb->estado = NEW;
    //El PCB se agrega a la cola de los procesos NEW
    queue_push(cola_new, pcb);
    // Crear un buffer para almacenar el mensaje con el último PID
    char mensaje[100];
    // Usar sprintf para formatear el mensaje con el último PID
    sprintf(mensaje, "Se crea el proceso %d en NEW\n", ultimo_pid);
    log_info(logger,mensaje);
}

/*En caso de que el grado de multiprogramación lo permita, los procesos creados podrán pasar de la cola de NEW a la cola de READY, 
caso contrario, se quedarán a la espera de que finalicen procesos que se encuentran en ejecución*/
void mover_procesos_ready() {
    // Obtener el grado de multiprogramación del archivo de configuración
    int grado_multiprogramacion = config_get_string_value(config, "GRADO_MULTIPROGRAMACION");
    
    // Calcular la cantidad de procesos en la cola de NEW y en la cola de READY
    int cantidad_procesos_new = queue_size(cola_new);
    
    int cantidad_procesos_ready = queue_size(cola_ready);

    // Verificar si el grado de multiprogramación lo permite
    if (cantidad_procesos_ready < grado_multiprogramacion) {
        // Mover procesos de la cola de NEW a la cola de READY
        while (cantidad_procesos_new > 0 && cantidad_procesos_ready < grado_multiprogramacion) {
            // Seleccionar el primer proceso de la cola de NEW
            PCB *proceso_nuevo = queue_peek(cola_new);
            //Borro el proceso que acabo de seleccionar
            queue_pop();
            
            // Cambiar el estado del proceso a READY
            proceso_nuevo->estado = READY;
            
            // Agregar el proceso a la cola de READY
            queue_push(cola_ready,proceso_nuevo);
            cantidad_procesos_ready++;
            
            // Reducir la cantidad de procesos en la cola de NEW
            cantidad_procesos_new--;
        }
    } else {
        printf("El grado de multiprogramación máximo ha sido alcanzado. Los procesos permanecerán en la cola de NEW.\n");
    }
}

//Planificador corto plazo
void planificar_procesos(char algoritmo_planificacion) {
    PCB* proceso_ejecucion;
    switch (algoritmo_planificacion) {
        case 'FIFO':
            proceso_ejecucion = planificar_fifo();
            break;
        case 'RR':
            proceso_ejecucion = planificar_rr();
            break;
        case 'VRR':
            proceso_ejecucion = planificar_vrr();
            break;
        default:
            printf("Algoritmo de planificación desconocido\n");
            break;
    }
    //Estado pasa a ejecucion
    proceso_ejecucion->estado = EXEC;
    //Me conecto al CPU a traves del dispatch
    conectar_dispatch_cpu(proceso_ejecucion);
}
PCB* planificar_fifo(){
    if (!queue_is_empty(cola_ready)){
        //FIFO toma el primer proceso de la cola
        PCB* proceso_ejecucion = queue_peek(cola_ready);
        //Borro ese proceso de mi cola de ready
        queue_pop(cola_ready);

        return proceso_ejecucion
    }
}