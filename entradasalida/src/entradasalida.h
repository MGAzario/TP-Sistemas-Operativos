#ifndef ENTRADASALIDA_H
#define ENTRADASALIDA_H

#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_cliente.h>
#include <utils/utils_server.h>
#include <utils/hello.h>
#include <utils/utils.h>
#include <commons/bitarray.h>

//Estructrua para bloques.dat
typedef struct {
    FILE *archivo;
    uint32_t block_size;
    uint32_t block_count;
} t_fs_bloques;

typedef struct {
    int bloque_inicial;
    int tamanio_archivo;
} t_metadata_archivo;

void crear_logger();
void create_config();
void conectar_kernel();
void conectar_memoria();
void crear_interfaz();
void crear_interfaz_generica();
void crear_interfaz_stdin();
void crear_interfaz_stdout();
void crear_interfaz_dialfs();
void crear_metadata_archivo(char *nombre_archivo, t_metadata_archivo metadata);
void manejar_io_fs_create();
void manejar_io_fs_delete();
void manejar_io_fs_read();
void manejar_io_fs_truncate();
void manejar_io_fs_write();
void actualizar_estructura_bloques(int bloque_inicial, int tamanio_actual, int nuevo_tamanio, int pid);
void verificar_y_compactar_fs(int bloque_inicial, int bloques_actuales, int nuevos_bloques, int pid);
void compactar_fs();
void mover_bloque(FILE *archivo, int bloque_origen, int bloque_destino);




#endif /*ENTRADASALIDA_H*/