#ifndef UTILS_SERVER_H_
#define UTILS_SERVER_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<string.h>
#include<assert.h>
// #include "estados.h"
// #include "registros.h"
#include "utils.h"
#include "utils_cliente.h"

#define PUERTO "4444"

extern t_log *logger;

void *recibir_buffer(int *, int);

int iniciar_servidor(char *puerto);
int esperar_cliente(int);
t_list *recibir_paquete(int);
void recibir_mensaje(int);
op_code recibir_operacion(int);
void recibir_ok(int socket_cliente);
t_pcb *recibir_pcb(int socket_cliente);
t_creacion_proceso *recibir_creacion_proceso(int socket_cliente);
t_instruccion *recibir_instruccion(int socket_cliente);
t_interrupcion *recibir_interrupcion(int socket_cliente);
t_sleep *recibir_sleep(int socket_cliente);
t_nombre_y_tipo_io *recibir_nombre_y_tipo(int socket_cliente);
t_resize *recibir_resize(int socket_cliente);
int recibir_numero(int socket_cliente);
t_solicitud_marco *recibir_solicitud_marco(int socket_cliente);
t_leer_memoria *recibir_leer_memoria(int socket_cliente);
t_lectura *recibir_lectura(int socket_cliente);
t_escribir_memoria *recibir_escribir_memoria(int socket_cliente);
t_io_std *recibir_io_std(int socket_cliente);
t_io_stdin_read *recibir_io_stdin_read(int socket_cliente);

#endif /* UTILS_SERVER_SERVER_H_ */
