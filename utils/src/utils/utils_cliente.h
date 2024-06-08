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
	FIN_IO_READ
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
t_paquete *crear_paquete_creacion_proceso(uint32_t tamanio_path);
t_paquete* crear_paquete_instruccion(uint32_t tamanio_instruccion);
t_paquete* crear_paquete_interrupcion();
t_paquete* crear_paquete_sleep(uint32_t tamanio_nombre_interfaz);
t_paquete* crear_paquete_nombre_y_tipo(uint32_t tamanio_nombre);
void agregar_pcb_a_paquete(t_paquete* paquete, t_pcb* pcb);
void agregar_creacion_proceso_a_paquete(t_paquete* paquete, t_pcb* pcb, char* path, uint32_t tamanio_path);
void agregar_instruccion_a_paquete(t_paquete* paquete, char* instruccion, uint32_t tamanio_instruccion);
void agregar_interrupcion_a_paquete(t_paquete* paquete, t_pcb* pcb, motivo_interrupcion motivo);
void agregar_sleep_a_paquete(t_paquete* paquete, t_pcb* pcb, char* nombre_interfaz, uint32_t tamanio_nombre_interfaz, uint32_t unidades_de_trabajo);
void agregar_nombre_y_tipo_a_paquete(t_paquete* paquete, char *nombre, uint32_t tamanio_nombre, tipo_interfaz tipo);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void enviar_ok(int socket_cliente, op_code cod_op);
void enviar_pcb(int socket_cliente, t_pcb *pcb);
void enviar_creacion_proceso(int socket_cliente, t_pcb *pcb, char *path);
void enviar_solicitud_instruccion(int socket_cliente, t_pcb *pcb);
void enviar_instruccion(int socket_cliente, char *instruccion);
void enviar_exit(int socket_cliente, t_pcb *pcb);
void enviar_interrupcion(int socket_cliente, t_pcb *pcb, motivo_interrupcion motivo);
void enviar_sleep(int socket_cliente, t_pcb *pcb, char *nombre_interfaz, uint32_t unidades_de_trabajo);
void enviar_nombre_y_tipo(int socket_cliente, char *nombre, tipo_interfaz tipo);
void enviar_fin_sleep(int socket_cliente, t_pcb *pcb);

#endif /* UTILS_CLIENTE_H_ */