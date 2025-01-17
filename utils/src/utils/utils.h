#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <semaphore.h>
#include <commons/collections/queue.h>

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

typedef enum {
    FINALIZAR_PROCESO,
    FIN_DE_QUANTUM
} motivo_interrupcion;

typedef enum {
    GENERICA,
    STDIN,
    STDOUT,
    DialFS
  } tipo_interfaz;

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
    t_list *lista_marcos;
} t_tabla_de_paginas;

typedef struct
{
    int pid;
    t_list *lista_instrucciones;
} t_instrucciones_de_proceso;

typedef struct
{
    char *instruccion;
    uint32_t tamanio_instruccion;
} t_instruccion;

typedef struct
{
    t_pcb *pcb;
    motivo_interrupcion motivo;
} t_interrupcion;

//Estructuras para interfaces
typedef struct
{
    t_pcb *pcb;
    char *nombre_interfaz;
    uint32_t tamanio_nombre_interfaz;
    uint32_t unidades_de_trabajo;
} t_sleep;

typedef struct {
    t_pcb *pcb;
    char *nombre_interfaz;
    uint32_t tamanio_nombre_interfaz;
    uint32_t tamanio_contenido;
    t_list *direcciones_fisicas;
} t_io_std;

typedef struct {
    t_pcb *pcb;
    char *nombre_interfaz;
    uint32_t tamanio_nombre_interfaz;
    char *nombre_archivo;
    uint32_t tamanio_nombre_archivo;
} t_io_fs_archivo;

// typedef struct {
//     t_pcb *pcb;
//     char *nombre_interfaz;
//     uint32_t tamanio_nombre_interfaz;
//     char *nombre_archivo;
//     uint32_t tamanio_nombre_archivo;
// } t_io_fs_delete;

typedef struct {
    t_pcb *pcb;                      
    char *nombre_interfaz;           
    uint32_t tamanio_nombre_interfaz;
    char *nombre_archivo;            
    uint32_t tamanio_nombre_archivo; 
    uint32_t nuevo_tamanio;           
} t_io_fs_truncate;

typedef struct {
    t_pcb *pcb;
    char *nombre_interfaz;
    uint32_t tamanio_nombre_interfaz;
    char *nombre_archivo;
    uint32_t tamanio_nombre_archivo;
    t_list *direcciones_fisicas; // Lista de direcciones físicas
    uint32_t tamanio;            // Tamaño de los datos a escribir
    int puntero_archivo;    // Puntero dentro del archivo donde empezar a escribir
} t_io_fs_rw;

typedef enum {
    CREATE,
    DELETE,
    TRUNCATE,
    WRITE,
    READ
} operacion_fs;

typedef struct {
    operacion_fs operacion;
    t_pcb *pcb;
    char *nombre_interfaz;
    uint32_t tamanio_nombre_interfaz;
    char *nombre_archivo;
    uint32_t tamanio_nombre_archivo;
    t_list *direcciones_fisicas;
    uint32_t tamanio;
    int puntero_archivo;
} t_io_fs_comodin;

// typedef struct {
//     t_pcb *pcb;
//     char *nombre_interfaz;
//     uint32_t tamanio_nombre_interfaz;
//     char *nombre_archivo;
//     uint32_t tamanio_nombre_archivo;
//     t_list *direcciones_fisicas; // Lista de direcciones físicas
//     uint32_t tamanio;            // Tamaño de los datos a escribir
//     uint32_t puntero_archivo;    // Puntero dentro del archivo donde empezar a escribir
// } t_io_fs_read;

typedef struct
{
    int socket;
    char *nombre;
    tipo_interfaz tipo;
    bool ocupada;
    t_queue *cola_procesos_esperando;
} t_interfaz;

typedef struct
{
    char *nombre;
    uint32_t tamanio_nombre;
    tipo_interfaz tipo;
} t_nombre_y_tipo_io;

typedef struct
{
    t_pcb *pcb;
    int tamanio;
} t_resize;

typedef struct
{
    int numero;
} t_numero;

typedef struct
{
    int pid;
    int pagina;
} t_solicitud_marco;

typedef struct
{
    int direccion;
    int tamanio;
} t_direccion_y_tamanio;

typedef struct
{
    int pid;
    int direccion;
    int tamanio;
} t_leer_memoria;

typedef struct
{
    void *lectura;
    int tamanio_lectura;
} t_lectura;

typedef struct
{
    int pid;
    int direccion;
    int tamanio;
    void *valor;
} t_escribir_memoria;

typedef struct
{
    t_pcb *pcb;
    char *nombre;
    uint32_t tamanio_nombre;
} t_recurso;

typedef struct
{
    char *nombre;
    int instancias;
    t_queue *procesos_esperando;
    t_list *procesos_asignados;
} t_manejo_de_recurso;

typedef struct
{
    int pid;
    int instancias;
} t_instancias_por_procesos;

typedef struct
{
    int pid;
    int pagina;
    int marco;
} t_entrada_tlb;


//Declaracion de los semaforos
extern sem_t sem_multiprogramacion;

#endif /* UTILS_H */