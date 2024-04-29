// registros.h

#ifndef REGISTROS_H
#define REGISTROS_H

#include <stdint.h>
#include <semaphore.h>
#include "estados.h"

#define AX 0
#define BX 1
#define CX 2
#define DX 3

#define EAX 0
#define EBX 1
#define ECX 2
#define EDX 3

// Definición de los registros de la CPU
typedef struct {
    uint32_t pc;   // Program Counter, 4 bytes
    uint8_t normales[4];
    uint32_t extendidos[4];
    uint32_t si;   // Contiene la dirección lógica de memoria de origen, 4 bytes
    uint32_t di;   // Contiene la dirección lógica de memoria de destino, 4 bytes
    // Aca son todos los registros que estan en el enunciado
} t_cpu_registers;

//Definicion de un PCB
typedef struct
{
    int pid;
    int quantum;
    t_cpu_registers *cpu_registers;
    estado_proceso estado;
} t_pcb;

//Declaracion de los semaforos
extern sem_t sem_nuevo_pcb;
extern sem_t sem_proceso_liberado;
#endif /* CPU_H */
