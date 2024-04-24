#ifndef KERNEL_H
#define KERNEL_H
#include <utils/registros.h>
#include <utils/estados.h>
// Definición del PCB
typedef struct
{
    int PID;
    uint32_t program_counter;
    int quantum;
    CPU_Registers cpu_registers;
    estado_proceso estado;
} PCB;

#endif /*KERNEL_H*/