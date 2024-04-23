#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_server.h>
#include <utils/utils_cliente.h>
#include <utils/hello.h>

t_config *config;
t_log* logger;

void crear_logger();
void crear_config();
void conectar_memoria(); 
void recibir_kernel_dispatch();
void recibir_kernel_interrupt();

int main(int argc, char* argv[]) {
    decir_hola("CPU");
    //Las creaciones se pasan a funciones para limpiar el main.
    crear_logger();
    crear_config();
    // Establecer conexión con el módulo Memoria
    conectar_memoria();
    // Recibir mensaje del dispatch del Kernel
    recibir_kernel_dispatch();
    // Recibir mensaje del dispatch del Kernel
    recibir_kernel_interrupt();

    printf("Terminó\n");

    return 0;
}

void crear_logger(){
    logger = log_create("./cpu.log","LOG_CPU",true,LOG_LEVEL_INFO);
    //Tira error si no se pudo crear
    if(logger == NULL){
		perror("Ocurrió un error al leer el archivo de Log de CPU");
		abort();
	}
}

void crear_config(){
    config = config_create("./cpu.config");
    if (config == NULL)
    {
        log_error(logger,"Ocurrió un error al leer el archivo de configuración\n");
        abort();
    }
}

void recibir_kernel_dispatch(){
    // Crear socket servidor para aceptar conexion de CPU siguiendo la arquitectura del sistema.
    char *puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    int socket_cpu_dispatch = iniciar_servidor(puerto_cpu_dispatch);
    int socket_kernel_dispatch = esperar_cliente(socket_cpu_dispatch);
    if (socket_kernel_dispatch == -1) {
        log_info(logger,"Error al aceptar la conexión del kernel asl socket de dispatch.\n");
        liberar_conexion(socket_cpu_dispatch);
    }
    //Esto deberia recibir el mensaje que manda el kernel
    recibir_mensaje(socket_kernel_dispatch);
    // Cerrar conexión con el cliente
    liberar_conexion(socket_kernel_dispatch);
     // Cerrar socket servidor
    liberar_conexion(socket_cpu_dispatch);
}

//Revisar si es necesario tener 2 sockets distintos para las funcionalidades, o 1 solo que tenga una logica dentro. Esto aplica para Kernel y otros modulos.
void recibir_kernel_interrupt(){
    char *puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
    int socket_cpu_interrupt = iniciar_servidor(puerto_cpu_interrupt);
    int socket_kernel_interrupt = esperar_cliente(socket_cpu_interrupt);
    //Si falla, no se pudo aceptar
    if (socket_kernel_interrupt == -1) {
        log_info(logger,"Error al aceptar la conexión del kernel al socket de interrupt.\n");
        liberar_conexion(socket_cpu_interrupt);
    }
    recibir_mensaje(socket_kernel_interrupt);
    // Cerrar conexión con el cliente
    liberar_conexion(socket_kernel_interrupt);
    // Cerrar socket servidor
    liberar_conexion(socket_cpu_interrupt);
}

void conectar_memoria(){
    char *ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char *puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    int socket_memoria = conectar_modulo(ip_memoria, puerto_memoria);
    if (socket_memoria != -1) {
        enviar_mensaje("Mensaje a la Memoria desde el Kernel", socket_memoria);
        liberar_conexion(socket_memoria);
    }
}

// Función para manejar la conexión entrante
void manejar_mensaje_kernel(int socket_cliente) {
    t_paquete* paquete = recibir_paquete(socket_cliente);

    if (paquete == NULL) {
        log_info(logger, "Error al recibir el paquete\n");
        return;
    }

    if (paquete->codigo_operacion == DISPATCH) {
        log_info(logger, "Paquete DISPATCH recibido desde el Kernel\n");
        t_list lista_pcb = recibir_paquete(socket_cliente);
    } else if (paquete->codigo_operacion == INTERRUPT) {
        log_info(logger, "Paquete INTERRUPT recibido desde el Kernel\n");
    }

    eliminar_paquete(paquete);
}