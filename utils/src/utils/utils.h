#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <semaphore.h>

#define AX 0
#define BX 1
#define CX 2
#define DX 3

#define EAX 0
#define EBX 1
#define ECX 2
#define EDX 3

typedef enum {
    NEW,
    READY,
    BLOCKED,
    EXEC,
    EXIT
} estado_proceso;

// Definición de los registros de la CPU
typedef struct {
    uint32_t pc;   // Program Counter, 4 bytes
    uint8_t normales[4];
    uint32_t extendidos[4];
    uint32_t si;   // Contiene la dirección lógica de memoria de origen, 4 bytes
    uint32_t di;   // Contiene la dirección lógica de memoria de destino, 4 bytes
    // Aca son todos los registros que estan en el enunciado
} t_cpu_registers;

// Definicion de un PCB
typedef struct
{
    int pid;
    int quantum;
    t_cpu_registers *cpu_registers;
    estado_proceso estado;
} t_pcb;

// Estructura para que el kernel le solicite a la memoria crear un proceso
typedef struct
{
    t_pcb *pcb;
    char *path;
    uint32_t tamanio_path;
} t_creacion_proceso;

typedef struct
{
    int pid;
    t_list *lista_instrucciones;
} t_instrucciones_de_proceso;

//Declaracion de los semaforos
extern sem_t sem_nuevo_pcb;

#endif /* UTILS_H */