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
#include "estados.h"
#include "registros.h"
#include "utils_cliente.h"

#define PUERTO "4444"

extern t_log *logger;

void *recibir_buffer(int *, int);

int iniciar_servidor(char *puerto);
int esperar_cliente(int);
t_list *recibir_paquete(int);
void recibir_mensaje(int);
op_code recibir_operacion(int);
t_pcb *recibir_pcb(int socket_cliente);

#endif /* UTILS_SERVER_SERVER_H_ */
