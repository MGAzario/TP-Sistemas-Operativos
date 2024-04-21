#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_cliente.h>
#include <utils/utils_server.h>
#include <utils/registros.h>
#include <utils/hello.h>

t_config *config;

// Definición del PCB
typedef struct {
    int PID;
    uint32_t program_counter;
    int quantum;
    CPU_Registers cpu_registers;
    // Aca los registros no se si lleva todos, el program counter por ejemplo estaria repetido.
} PCB;

int main(int argc, char* argv[]) {
    decir_hola("Kernel");

    config = config_create("./kernel.config");
    if (config == NULL)
    {
        printf("Ocurrió un error al leer el archivo de configuración\n");
        abort();
    }

    char *ip_cpu = config_get_string_value(config, "IP_CPU");

    // Establecer conexión con el módulo CPU (dispatch)
    char *puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    int socket_cpu_dispatch = conectar_modulo(ip_cpu, puerto_cpu_dispatch);
    if (socket_cpu_dispatch != -1) {
        enviar_mensaje("Mensaje al CPU desde el Kernel por dispatch", socket_cpu_dispatch);
        liberar_conexion(socket_cpu_dispatch);
    }

    // Establecer conexión con el módulo CPU (interrupt)
    char *puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
    int socket_cpu_interrupt = conectar_modulo(ip_cpu, puerto_cpu_interrupt);
    if (socket_cpu_interrupt != -1) {
        enviar_mensaje("Mensaje al CPU desde el Kernel por interrupt", socket_cpu_interrupt);
        liberar_conexion(socket_cpu_interrupt);
    }

    // Establecer conexión con el módulo Memoria
    char *ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char *puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    int socket_memoria = conectar_modulo(ip_memoria, puerto_memoria);
    if (socket_memoria != -1) {
        enviar_mensaje("Mensaje a la Memoria desde el Kernel", socket_memoria);
        liberar_conexion(socket_memoria);
    }

    char *puerto_kernel= config_get_string_value(config, "PUERTO_ESCUCHA");
    int socket_kernel = iniciar_servidor(puerto_kernel);

    // Espero a un cliente (entradasalida). El mensaje entiendo que se programa despues
    int socket_entradasalida = esperar_cliente(socket_kernel);

    //Si falla, no se pudo aceptar
    if (socket_entradasalida == -1) {
        printf("Error al aceptar la conexión del kernel asl socket de dispatch.\n");
        liberar_conexion(socket_kernel);
    }
    //Esto deberia recibir el mensaje que manda el kernel
    recibir_mensaje(socket_entradasalida);

    // Cerrar conexión con el cliente
    liberar_conexion(socket_entradasalida);

    // Cerrar socket servidor
    liberar_conexion(socket_kernel);

    liberar_conexion(socket_cpu_dispatch);
    liberar_conexion(socket_cpu_interrupt);

    printf("Terminó\n");
    
    return 0;
}
