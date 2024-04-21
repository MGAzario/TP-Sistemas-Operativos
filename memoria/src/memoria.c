#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_server.h>
#include <utils/utils_cliente.h>
#include <utils/hello.h>

t_config *config;

int main(int argc, char* argv[]) {
    decir_hola("Memoria");
    
    t_log* memoria_logger = log_create("./memoria.log","LOG_MEMORIA",true,LOG_LEVEL_INFO);
    if(memoria_logger == NULL){
		perror("Ocurrió un error al leer el archivo de Log de Memoria");
		abort();
	}

    config = config_create("./memoria.config");
    if (config == NULL)
    {
        perror("Ocurrió un error al leer el archivo de configuración\n");
        abort();
    }

    // Crear socket memoria, tomando el puerto de un archivo. El IP no lo pasamos como parametro al iniciar servidor, entiendo que deberiamos.
    char *puerto_memoria= config_get_string_value(config, "PUERTO_MEMORIA");
    int socket_memoria = iniciar_servidor(puerto_memoria);

    // Espero a los clientes. El mensaje entiendo que se programa despues
    int socket_kernel = esperar_cliente(socket_memoria);
    if (socket_kernel_planificador == -1) {
        log_info(memoria_logger,"Error al aceptar la conexión del kernel.\n");
        liberar_conexion(socket_memoria);
    }

    int socket_cpu = esperar_cliente(socket_memoria);
    if (socket_kernel_planificador == -1) {
        log_info(memoria_logger,"Error al aceptar la conexión del kernel.\n");
        liberar_conexion(socket_memoria);
    }

    int socket_entradasalida = esperar_cliente(socket_memoria);
    if (socket_kernel_planificador == -1) {
        log_info(memoria_logger,"Error al aceptar la conexión del kernel.\n");
        liberar_conexion(socket_memoria);
    }

    //Esto deberia recibir el mensaje que manda el kernel
    recibir_mensaje(socket_kernel_planificador);

    // Cerrar conexión con los clientes
    liberar_conexion(socket_kernel);
    liberar_conexion(socket_cpu);
    liberar_conexion(socket_memoria);

    // Cerrar socket servidor
    liberar_conexion(socket_memoria);

    return 0;
}