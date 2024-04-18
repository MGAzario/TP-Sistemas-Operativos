#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_cliente.h>
#include <cpu/registros.h>

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

    // Establecer conexión con el módulo CPU
    int socket_cpu = conectar_modulo(IP_CPU, PUERTO_CPU);
    if (socket_cpu != -1) {
        enviar_mensaje"Mensaje al CPU desde el Kernel", socket_cpu);
        liberar_conexion(socket_cpu);
    }


    // Establecer conexión con el módulo Memoria
    int socket_memoria = conectar_modulo(IP_MEMORIA, PUERTO_MEMORIA);
    if (socket_memoria != -1) {
        enviar_mensaje("Mensaje a la Memoria desde el Kernel", socket_memoria);
        liberar_conexion(socket_memoria);
    }
    
    return 0;
}

