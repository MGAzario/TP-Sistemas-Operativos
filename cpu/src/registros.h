// registros.h

#ifndef CPU_H
#define CPU_H

#include <stdint.h>

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

#endif /* CPU_H */
