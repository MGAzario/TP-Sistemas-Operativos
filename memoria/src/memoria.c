#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_server.h>
#include <utils/utils_cliente.h>
#include <utils/hello.h>

t_config *config;

void recibir_kernel_planificador();

int main(int argc, char* argv[]) {
    decir_hola("Memoria");
    recibir_kernel();
    return 0;
}

void recibir_kernel_planificador(){

    config = config_create("./memoria.config");
    if (config == NULL)
    {
        printf("Ocurrió un error al leer el archivo de configuración\n");
        abort();
    }

    // Crear socket memoria, tomando el puerto de un archivo. El IP no lo pasamos como parametro al iniciar servidor, entiendo que deberiamos.
    char *puerto_memoria= config_get_string_value(config, "PUERTO_MEMORIA");
    int socket_memoria = iniciar_servidor(puerto_memoria);

    // Espero a un cliente (el Kernel). El mensaje entiendo que se programa despues
    int socket_kernel_planificador = esperar_cliente(socket_memoria);

    //Si falla, no se pudo aceptar
    if (socket_kernel_planificador == -1) {
        printf("Error al aceptar la conexión del kernel.\n");
        liberar_conexion(socket_memoria);
    }
    //Esto deberia recibir el mensaje que manda el kernel
    recibir_mensaje(socket_kernel_planificador);

    // Cerrar conexión con el cliente
    liberar_conexion(socket_kernel_planificador);

    // Cerrar socket servidor
    liberar_conexion(socket_memoria);

}