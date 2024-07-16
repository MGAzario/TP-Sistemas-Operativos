#ifndef UTILS_CLIENTE_H_
#define UTILS_CLIENTE_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/string.h>
#include"utils.h"

extern t_log *logger;

typedef enum
{
	MENSAJE,
	PAQUETE,
	DISPATCH,
	INTERRUPT,
	CPU_SOLICITAR_INSTRUCCION,
	CREACION_PROCESO,
	CREACION_PROCESO_OK,
	FINALIZACION_PROCESO,
	FINALIZACION_PROCESO_OK,
	SOLICITUD_INSTRUCCION,
	INSTRUCCION,
	INSTRUCCION_EXIT,
	INTERRUPCION,
	DESCONEXION,
	NOMBRE_Y_TIPO_IO,
	IO_GEN_SLEEP,
	IO_STDIN_READ,
	IO_STDOUT_WRITE,
	IO_FS_CREATE,
	IO_FS_DELETE,
	IO_FS_TRUNCATE,
	IO_FS_WRITE,
	IO_FS_READ,
	FIN_SLEEP,
	FIN_IO_READ,
	FIN_IO_FS_CREATE,
	FIN_IO_FS_DELETE,
	FIN_IO_FS_TRUNCATE,
	FIN_IO_FS_WRITE,
	FIN_IO_FS_READ,
	FIN_IO_FS,
	ERROR_IO_FS,
	RESIZE,
	RESIZE_EXITOSO,
	OUT_OF_MEMORY,
	PREGUNTA_TAMANIO_PAGINA,
	RESPUESTA_TAMANIO_PAGINA,
	SOLICITUD_MARCO,
	MARCO,
	LEER_MEMORIA,
	MEMORIA_LEIDA,
	ESCRIBIR_MEMORIA,
	MEMORIA_ESCRITA,
	WAIT,
	SIGNAL
} op_code;

typedef struct
{
	int size;
	void *stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer *buffer;
} t_paquete;

int crear_conexion(char *ip, char *puerto);
void enviar_mensaje(char *mensaje, int socket_cliente);
t_paquete *crear_paquete(void);
void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio);
void enviar_paquete_alternativo(t_paquete *paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
int conectar_modulo(char* ip, char* puerto);
t_paquete *crear_paquete_pcb(op_code cod_op);
t_paquete *crear_paquete_pcb_con_cadena(uint32_t tamanio_path, op_code cod_op);
t_paquete* crear_paquete_instruccion(uint32_t tamanio_instruccion);
t_paquete* crear_paquete_interrupcion();
t_paquete* crear_paquete_sleep(uint32_t tamanio_nombre_interfaz);
t_paquete* crear_paquete_nombre_y_tipo(uint32_t tamanio_nombre);
t_paquete* crear_paquete_resize();
t_paquete* crear_paquete_numero(op_code cod_op);
t_paquete* crear_paquete_solicitud_marco();
t_paquete* crear_paquete_leer_memoria();
t_paquete* crear_paquete_lectura(int tamanio_lectura);
t_paquete* crear_paquete_escribir_memoria(int tamanio_valor);
void agregar_pcb_a_paquete(t_paquete* paquete, t_pcb* pcb);
void agregar_pcb_con_cadena_a_paquete(t_paquete* paquete, t_pcb* pcb, char* cadena, uint32_t tamanio_cadena);
void agregar_instruccion_a_paquete(t_paquete* paquete, char* instruccion, uint32_t tamanio_instruccion);
void agregar_interrupcion_a_paquete(t_paquete* paquete, t_pcb* pcb, motivo_interrupcion motivo);
void agregar_sleep_a_paquete(t_paquete* paquete, t_pcb* pcb, char* nombre_interfaz, uint32_t tamanio_nombre_interfaz, uint32_t unidades_de_trabajo);
void agregar_nombre_y_tipo_a_paquete(t_paquete* paquete, char *nombre, uint32_t tamanio_nombre, tipo_interfaz tipo);
void agregar_resize_a_paquete(t_paquete* paquete, t_pcb* pcb, int tamanio);
void agregar_numero_a_paquete(t_paquete* paquete, int numero);
void agregar_solicitud_marco_a_paquete(t_paquete* paquete, int pid, int pagina);
void agregar_leer_memoria_a_paquete(t_paquete* paquete, int pid, int direccion, int tamanio);
void agregar_lectura_a_paquete(t_paquete* paquete, void *lectura, int tamanio_lectura);
void agregar_escribir_memoria_a_paquete(t_paquete* paquete, int pid, int direccion, int tamanio, void *valor);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void enviar_mensaje_simple(int socket_cliente, op_code cod_op);
void enviar_pcb(int socket_cliente, t_pcb *pcb);
void enviar_creacion_proceso(int socket_cliente, t_pcb *pcb, char *path);
void enviar_finalizacion_proceso(int socket_cliente, t_pcb *pcb);
void enviar_solicitud_instruccion(int socket_cliente, t_pcb *pcb);
void enviar_instruccion(int socket_cliente, char *instruccion);
void enviar_exit(int socket_cliente, t_pcb *pcb);
void enviar_interrupcion(int socket_cliente, t_pcb *pcb, motivo_interrupcion motivo);
void enviar_sleep(int socket_cliente, t_pcb *pcb, char *nombre_interfaz, uint32_t unidades_de_trabajo);
void enviar_nombre_y_tipo(int socket_cliente, char *nombre, tipo_interfaz tipo);
void enviar_fin_sleep(int socket_cliente, t_pcb *pcb);
void enviar_resize(int socket_cliente, t_pcb *pcb, int tamanio);
void enviar_out_of_memory(int socket_cliente, t_pcb *pcb);
void enviar_numero(int socket_cliente, int numero, op_code cod_op);
void enviar_solicitud_marco(int socket_cliente, int pid, int pagina);
void enviar_leer_memoria(int socket_cliente, int pid, int direccion, int tamanio);
void enviar_lectura(int socket_cliente, void *lectura, int tamanio_lectura);
void enviar_escribir_memoria(int socket_cliente, int pid, int direccion, int tamanio, void *valor);
void enviar_fin_io_read(int socket_cliente, t_pcb *pcb);
void enviar_stdout_write(int socket_cliente, t_pcb *pcb, char *nombre_interfaz, t_list *direcciones);
void enviar_wait(int socket_cliente, t_pcb *pcb, char *recurso);
void enviar_signal(int socket_cliente, t_pcb *pcb, char *recurso);
t_paquete* crear_paquete_io_stdin_read(uint32_t tamanio_interfaz, uint32_t cantidad_direcciones);
void agregar_io_stdin_read_a_paquete(t_paquete* paquete, t_io_stdin_read* io_stdin_read);
void enviar_io_stdin_read(int socket_cliente, t_io_stdin_read* io_stdin_read);
t_paquete* crear_paquete_io_stdout_write(t_io_stdout_write* io_stdout_write);
void agregar_io_stdout_write_a_paquete(t_paquete* paquete, t_io_stdout_write* io_stdout_write);
void enviar_io_stdout_write(int socket_cliente, t_io_stdout_write* io_stdout_write);
t_io_stdin_read* crear_io_stdin_read(t_pcb* pcb, char* nombre_interfaz, uint32_t tamanio_nombre_interfaz, uint32_t tamanio_contenido, t_list* direcciones_fisicas);
t_io_stdout_write* crear_io_stdout_write(t_pcb* pcb, char* nombre_interfaz, uint32_t tamanio_nombre_interfaz, uint32_t direccion_logica, uint32_t tamaño);
t_paquete* crear_paquete_io_fs_create(uint32_t tamanio_nombre_interfaz, uint32_t tamanio_nombre_archivo);
void agregar_io_fs_create_a_paquete(t_paquete* paquete, t_io_fs_create* io_fs_create);
t_io_fs_create* crear_io_fs_create(t_pcb* pcb, char* nombre_interfaz, char* nombre_archivo);
void enviar_io_fs_create(int socket_cliente, t_io_fs_create* io_fs_create);
t_io_fs_delete* crear_io_fs_delete(t_pcb* pcb, char* nombre_interfaz, char* nombre_archivo);
t_paquete* crear_paquete_io_fs_delete(uint32_t tamanio_nombre_interfaz, uint32_t tamanio_nombre_archivo);
void agregar_io_fs_delete_a_paquete(t_paquete* paquete, t_io_fs_delete* io_fs_delete);
void enviar_io_fs_delete(int socket_cliente, t_io_fs_delete* io_fs_delete);
t_io_fs_write* crear_io_fs_write(t_pcb* pcb, char* nombre_interfaz, char* nombre_archivo, t_list *direcciones_fisicas, uint32_t tamanio, uint32_t puntero_archivo);
t_paquete* crear_paquete_io_fs_write(t_io_fs_write* io_fs_write);
void agregar_io_fs_write_a_paquete(t_paquete* paquete, t_io_fs_write* io_fs_write);
void enviar_io_fs_write(int socket_cliente, t_io_fs_write* io_fs_write);
t_paquete* crear_paquete_io_fs_read(t_io_fs_read* io_fs_read);
void agregar_io_fs_read_a_paquete(t_paquete* paquete, t_io_fs_read* io_fs_read);
void enviar_io_fs_read(int socket_cliente, t_io_fs_read* io_fs_read);
void enviar_fin_io_fs(int socket_cliente, t_pcb *pcb);
void enviar_io_fs_truncate(int socket_cliente, t_io_fs_truncate* io_fs_truncate);


#endif /* UTILS_CLIENTE_H_ */