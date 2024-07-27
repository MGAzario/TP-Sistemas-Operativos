#ifndef ENTRADASALIDA_H
#define ENTRADASALIDA_H

#include <stdlib.h>
#include <stdio.h>
#include <readline/readline.h>
#include <utils/utils_cliente.h>
#include <utils/utils_server.h>
#include <utils/hello.h>
#include <utils/utils.h>
#include <commons/bitarray.h>
#include <commons/config.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <dirent.h>


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
void crear_metadata_archivo(char *nombre_archivo, t_metadata_archivo *metadata);
void manejar_io_fs_create();
void manejar_io_fs_delete();
void manejar_io_fs_read();
void manejar_io_fs_truncate();
void manejar_io_fs_write();
void actualizar_estructura_bloques(int bloque_inicial, int tamanio_actual, int nuevo_tamanio, int pid, char *archivo_nombre);
void verificar_y_compactar_fs(int bloque_inicial, int bloques_actuales, int nuevos_bloques, int pid, char *archivo_nombre);
void compactar_fs(int bloque_inicial, int bloques_actuales, char *archivo_nombre);
int mover_archivo(int bloque_origen, int bloque_destino);
t_metadata_archivo *archivo(char *nombre);
void escribir_archivo(char *nombre_archivo, int puntero, void *valor, uint32_t tamanio);
void *leer_archivo(char *nombre_archivo, int puntero, uint32_t tamanio);
void escribir_archivo_compactacion(char *nombre_archivo, int bloque_inicial, void *valor);
void *leer_archivo_compactacion(char *nombre_archivo);
void lista_de_bloques_iniciales();


#endif /*ENTRADASALIDA_H*/