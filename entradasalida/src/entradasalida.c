#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_cliente.h>
#include <utils/hello.h>

t_config *config;

int main(int argc, char* argv[]) {
    decir_hola("una Interfaz de Entrada/Salida");

    config = config_create("./entradasalida.config");
    if (config == NULL)
    {
        printf("Ocurrió un error al leer el archivo de configuración\n");
        abort();
    }

    // Establecer conexión con el módulo Kernel
    char *ip_kernel = config_get_string_value(config, "IP_KERNEL");
    char *puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    int socket_kernel = conectar_modulo(ip_kernel, puerto_kernel);
    if (socket_kernel != -1) {
        enviar_mensaje("Mensaje al kernel desde el Kernel", socket_kernel);
        liberar_conexion(socket_kernel);
    }

    // Establecer conexión con el módulo Memoria
    char *ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char *puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    int socket_memoria = conectar_modulo(ip_memoria, puerto_memoria);
    if (socket_memoria != -1) {
        enviar_mensaje("Mensaje a la Memoria desde el Kernel", socket_memoria);
        liberar_conexion(socket_memoria);
    }

    liberar_conexion(socket_kernel);
    liberar_conexion(socket_memoria);

    printf("Terminó\n");
    
    return 0;
}
