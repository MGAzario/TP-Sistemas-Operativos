#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_server.h>
#include <utils/utils_cliente.h>
#include <utils/hello.h>

t_config *config;

void recibir_kernel();

int main(int argc, char* argv[]) {
    decir_hola("CPU");

    config = config_create("./cpu.config");
    if (config == NULL)
    {
        printf("Ocurrió un error al leer el archivo de configuración\n");
        abort();
    }

    // Establecer conexión con el módulo Memoria
    char *ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char *puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    int socket_memoria = conectar_modulo(ip_memoria, puerto_memoria);
    if (socket_memoria != -1) {
        enviar_mensaje("Mensaje a la Memoria desde el Kernel", socket_memoria);
        liberar_conexion(socket_memoria);
    }

    // Crear socket servidor para aceptar conexion de CPU siguiendo la arquitectura del sistema.
    //TODO mismo que en kernel, de donde sale el puerto cpu. En una charla dijeron que usemos puertos altos, podremos hardcodearlos?
    char *puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    char *puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
    int socket_cpu_dispatch = iniciar_servidor(puerto_cpu_dispatch);
    int socket_cpu_interrupt = iniciar_servidor(puerto_cpu_interrupt);

    // Espero a un cliente (el Kernel). El mensaje entiendo que se programa despues
    int socket_kernel_dispatch = esperar_cliente(socket_cpu_dispatch);
    int socket_kernel_interrupt = esperar_cliente(socket_cpu_interrupt);

    //Si falla, no se pudo aceptar
    if (socket_kernel_dispatch == -1) {
        printf("Error al aceptar la conexión del kernel asl socket de dispatch.\n");
        liberar_conexion(socket_cpu_dispatch);
    }
    if (socket_kernel_interrupt == -1) {
        printf("Error al aceptar la conexión del kernel al socket de interrupt.\n");
        liberar_conexion(socket_cpu_interrupt);
    }
    //Esto deberia recibir el mensaje que manda el kernel
    recibir_mensaje(socket_kernel_dispatch);

    // Cerrar conexión con el cliente
    liberar_conexion(socket_kernel_dispatch);

    // Cerrar socket servidor
    liberar_conexion(socket_cpu_dispatch);

    return 0;
}