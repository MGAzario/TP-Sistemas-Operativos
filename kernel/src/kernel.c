#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_cliente.h>
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

    // Establecer conexión con el módulo CPU
    char *ip_cpu = config_get_string_value(config, "IP_CPU");
    char *puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    int socket_cpu = conectar_modulo(ip_cpu, puerto_cpu_dispatch);
    if (socket_cpu != -1) {
        enviar_mensaje("Mensaje al CPU desde el Kernel", socket_cpu);
        liberar_conexion(socket_cpu);
    }


    //Establecer conexión con el módulo Memoria
    char *ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char *puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    int socket_memoria = conectar_modulo(ip_memoria, puerto_memoria);
    if (socket_memoria != -1) {
         enviar_mensaje("Mensaje a la Memoria desde el Kernel", socket_memoria);
         liberar_conexion(socket_memoria);
     }

    printf("Terminó\n");
    
    return 0;
}

