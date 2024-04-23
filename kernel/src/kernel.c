#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_cliente.h>
#include <utils/utils_server.h>
#include <utils/registros.h>
#include <utils/estados.h>
#include <utils/hello.h>

t_config *config;
t_log* logger;

// Definición del PCB
typedef struct {
    int PID;
    uint32_t program_counter;
    int quantum;
    CPU_Registers cpu_registers;
    estado_proceso estado;
} PCB;

static int ultimo_pid = 0;

int main(int argc, char* argv[]) {
    decir_hola("Kernel");
    
    crear_logger();
    crear_config();

    

    //Usan la misma IP, se las paso por parametro.
    //Conexiones con CPU: Dispatch e Interrupt
    char *ip_cpu = config_get_string_value(config, "IP_CPU");
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

void conectar_dispatch_cpu(char* ip_cpu){
    // Establecer conexión con el módulo CPU (dispatch)
    char *puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    int socket_cpu_dispatch = conectar_modulo(ip_cpu, puerto_cpu_dispatch);
    if (socket_cpu_dispatch != -1) {
        //enviar_paquete_pcb(socket_cpu_dispatch, pcb, codigo_operacion)
        enviar_mensaje("Mensaje al CPU desde el Kernel por dispatch", socket_cpu_dispatch);
        liberar_conexion(socket_cpu_dispatch);
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
PCB* crear_pcb(int quantum) {
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
    return pcb;
}

/*En caso de que el grado de multiprogramación lo permita, los procesos creados podrán pasar de la cola de NEW a la cola de READY, 
caso contrario, se quedarán a la espera de que finalicen procesos que se encuentran en ejecución*/
void mover_procesos_ready(PCB *cola_new[], PCB *cola_ready[]) {
    // Obtener el grado de multiprogramación del archivo de configuración
    int grado_multiprogramacion = config_get_string_value(config, "GRADO_MULTIPROGRAMACION");
    
    // Calcular la cantidad de procesos en la cola de NEW y en la cola de READY
    int cantidad_procesos_new = list_size(cola_new);
    
    int cantidad_procesos_ready = list_size(cola_ready);

    // Verificar si el grado de multiprogramación lo permite
    if (cantidad_procesos_ready < grado_multiprogramacion) {
        // Mover procesos de la cola de NEW a la cola de READY
        while (cantidad_procesos_new > 0 && cantidad_procesos_ready < grado_multiprogramacion) {
            // Seleccionar el primer proceso de la cola de NEW
            PCB *proceso_nuevo = cola_new[0];
            
            // Cambiar el estado del proceso a READY
            proceso_nuevo->estado = READY;
            
            // Agregar el proceso a la cola de READY
            cola_ready[cantidad_procesos_ready] = proceso_nuevo;
            cantidad_procesos_ready++;
            
            // Reducir la cantidad de procesos en la cola de NEW
            cantidad_procesos_new--;
            
            // Eliminar el proceso seleccionado de la cola de NEW (FIFO)
            for (int i = 0; i < cantidad_procesos_new; i++) {
                cola_new[i] = cola_new[i+1];
            }
            //Borra el proximo espacio ocupado dejandolo en NULL
            cola_new[cantidad_procesos_new] = NULL;
        }
    } else {
        printf("El grado de multiprogramación máximo ha sido alcanzado. Los procesos permanecerán en la cola de NEW.\n");
    }
}

//Requisito Checkpoint: Es capaz de enviar un proceso a la CPU
/*Una vez seleccionado el siguiente proceso a ejecutar, se lo transicionará al estado EXEC y se enviará su Contexto de Ejecución al CPU 
a través del puerto de dispatch, quedando a la espera de recibir dicho contexto actualizado después de la ejecución, 
junto con un motivo de desalojo por el cual fue desplazado a manejar.*/
void enviar_lista_pcb(int socket_cliente, t_list* lista_pcb) {
    // Crear un paquete para enviar los PCBs
    t_paquete* paquete = crear_paquete();
    if (paquete == NULL) {
        log_info(logger, "Error al crear el paquete\n");
        return;
    }

    // Establecer el código de operación
    paquete->codigo_operacion = DISPATCH;

    // Recorrer la lista de PCBs y agregar cada uno al paquete
    for (int i = 0; i < list_size(lista_pcb); i++) {
        PCB* pcb = list_get(lista_pcb, i);
        agregar_a_paquete(paquete, pcb, sizeof(PCB));
    }

    // Enviar el paquete a la CPU
    enviar_paquete(paquete, socket_cliente);

    // Eliminar el paquete después de enviarlo
    eliminar_paquete(paquete);
}