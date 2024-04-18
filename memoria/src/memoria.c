#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include <utils/utils_server.h>

int main(int argc, char* argv[]) {
    decir_hola("Memoria");
    return 0;
}

void recibir_kernel(){
    // Esto es repetido de lo que hice en el CPU, todas las recepciones por ahora son iguales o hay algo que estoy haciendo mal.
    //TODO PUERTO_MEMORIA
    int socket_memoria = crear_servidor(PUERTO_MEMORIA);

    // Espero a un cliente (el Kernel, CPU o las interfaces). 
    //Aca el tema es que hay 3 modulos interactuando. Son 3 sockets? Es 1 socket que encola los pedidos? La memoria es dios?
    int socket_escucha = esperar_cliente(socket_memoria);

    //Si falla, no se pudo aceptar
    if (socket_kernel == -1) {
        printf("Error al aceptar la conexión del kernel.\n");
        liberar_conexion(socket_memoria);
        return 1;
    }
    //Esto deberia recibir el mensaje que manda cualquiera de los modulos
    recibir_mensaje(socket_escucha);

    // Cerrar conexión con el cliente
    liberar_conexion(socket_escucha);

    // Cerrar socket servidor
    liberar_conexion(socket_memoria);

}
