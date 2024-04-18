#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include "cpu/registros.h"

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
    return 0;
}
