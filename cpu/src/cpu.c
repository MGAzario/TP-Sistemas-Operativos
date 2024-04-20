#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_server.h>
#include <utils/utils_cliente.h>
#include <utils/hello.h>

t_config *config;

void recibir_kernel();

int main(int argc, char* argv[]) {
    decir_hola("CPU");

    recibir_kernel();

    return 0;
}
void recibir_kernel(){

    config = config_create("./cpu.config");
    if (config == NULL)
    {
        printf("Ocurrió un error al leer el archivo de configuración\n");
        abort();
    }

    // Crear socket servidor para aceptar conexion de CPU siguiendo la arquitectura del sistema.
    //TODO mismo que en kernel, de donde sale el puerto cpu. En una charla dijeron que usemos puertos altos, podremos hardcodearlos?
    char *puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    int socket_cpu_dispatch = iniciar_servidor(puerto_cpu_dispatch);

    // Espero a un cliente (el Kernel). El mensaje entiendo que se programa despues
    int socket_kernel_dispatch = esperar_cliente(socket_cpu_dispatch);

    //Si falla, no se pudo aceptar
    if (socket_kernel_dispatch == -1) {
        printf("Error al aceptar la conexión del kernel.\n");
        liberar_conexion(socket_cpu_dispatch);
    }
    //Esto deberia recibir el mensaje que manda el kernel
    recibir_mensaje(socket_kernel_dispatch);

    // Cerrar conexión con el cliente
    liberar_conexion(socket_kernel_dispatch);

    // Cerrar socket servidor
    liberar_conexion(socket_cpu_dispatch);

}
// void enviar_memoria(){
//     // Establecer conexión con el módulo Memoria
//     int socket_memoria = conectar_modulo(IP_MEMORIA, PUERTO_MEMORIA);
//     if (socket_memoria != -1) {
//         enviar_mensaje("Mensaje a la Memoria desde el Kernel", socket_memoria);
//         liberar_conexion(socket_memoria);
//     }
// }