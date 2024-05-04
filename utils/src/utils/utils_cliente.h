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
#include"registros.h"

extern t_log *logger;

typedef enum
{
	MENSAJE,
	PAQUETE,
	DISPATCH,
	INTERRUPT,
	CPU_SOLICITAR_INSTRUCCION,
	KERNEL_CREACION_PROCESO,
	MENSAJE_ENTRADA_SALIDA,
	DESCONEXION
}op_code;

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
void enviar_paquete(t_paquete *paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
int conectar_modulo(char* ip, char* puerto);
t_paquete* crear_paquete_pcb();
void agregar_pcb_a_paquete(t_paquete* paquete, t_pcb* pcb);
void enviar_paquete_pcb(t_paquete* paquete, int socket_cliente);
void enviar_pcb(int socket_cliente, t_pcb *pcb);

#endif /* UTILS_CLIENTE_H_ */