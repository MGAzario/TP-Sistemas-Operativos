#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_server.h>
#include <utils/utils_cliente.h>

int main(int argc, char* argv[]) {
    decir_hola("CPU");
    return 0;
}
void recibir_kernel(){
    // Crear socket servidor para aceptar conexion de CPU siguiendo la arquitectura del sistema.
    //TODO mismo que en kernel, de donde sale el puerto cpu. En una charla dijeron que usemos puertos altos, podremos hardcodearlos?
    int socket_cpu = crear_servidor(PUERTO_CPU);

    // Espero a un cliente (el Kernel). El mensaje entiendo que se programa despues
    int socket_kernel = esperar_cliente(socket_cpu);

    //Si falla, no se pudo aceptar
    if (socket_kernel == -1) {
        printf("Error al aceptar la conexión del kernel.\n");
        liberar_conexion(socket_cpu);
        return 1;
    }
    //Esto deberia recibir el mensaje que manda el kernel
    recibir_mensaje(socket_kernel);

    // Cerrar conexión con el cliente
    liberar_conexion(socket_kernel);

    // Cerrar socket servidor
    liberar_conexion(socket_cpu);

}
void enviar_memoria(){
    // Establecer conexión con el módulo Memoria
    int socket_memoria = conectar_modulo(IP_MEMORIA, PUERTO_MEMORIA);
    if (socket_memoria != -1) {
        enviar_mensaje("Mensaje a la Memoria desde el Kernel", socket_memoria);
        liberar_conexion(socket_memoria);
    }
}