// registros.h

#ifndef REGISTROS_H
#define REGISTROS_H

#include <stdint.h>
#include <estados.h>

// Definición de los registros de la CPU
typedef struct {
    uint32_t PC;   // Program Counter, 4 bytes
    uint8_t AX;    // Registro Numérico de propósito general, 1 byte
    uint8_t BX;    // Registro Numérico de propósito general, 1 byte
    uint8_t CX;    // Registro Numérico de propósito general, 1 byte
    uint8_t DX;    // Registro Numérico de propósito general, 1 byte
    uint32_t EAX;  // Registro Numérico de propósito general, 4 bytes
    uint32_t EBX;  // Registro Numérico de propósito general, 4 bytes
    uint32_t ECX;  // Registro Numérico de propósito general, 4 bytes
    uint32_t EDX;  // Registro Numérico de propósito general, 4 bytes
    uint32_t SI;   // Contiene la dirección lógica de memoria de origen, 4 bytes
    uint32_t DI;   // Contiene la dirección lógica de memoria de destino, 4 bytes
    // Aca son todos los registros que estan en el enunciado
} CPU_Registers;

//Definicion de un PCB
typedef struct
{
    int PID;
    uint32_t program_counter;
    int quantum;
    CPU_Registers cpu_registers;
    estado_proceso estado;
} PCB;

//Declaracion de los semaforos
extern sem_t sem_nuevo_pcb;
extern sem_t sem_proceso_liberado;
#endif /* CPU_H */
