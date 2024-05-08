#include "utils_server.h"

int iniciar_servidor(char *puerto)
{
	int socket_servidor;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, puerto, &hints, &servinfo);

	// Creamos el socket de escucha del servidor
	socket_servidor = socket(servinfo->ai_family,
							 servinfo->ai_socktype,
							 servinfo->ai_protocol);

	/* Esto está para que se pueda  reutilizar el puerto. Si esto no está solo se conecta una vez
	y luego no puede conectarse una segunda vez*/
	const int cosa = 1;
	setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &cosa, sizeof(int));
	// Asociamos el socket a un puerto
	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);
	// Escuchamos las conexiones entrantes
	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);
	log_info(logger, "Listo para escuchar a mi cliente\n");

	return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor, NULL, NULL);
	log_info(logger, "Se conecto un cliente!\n");

	return socket_cliente;
}

op_code recibir_operacion(int socket_cliente)
{
	op_code cod_op;
	if (recv(socket_cliente, &cod_op, sizeof(cod_op), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return DESCONEXION;
	}
}

void *recibir_buffer(int *size, int socket_cliente)
{
	void *buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void recibir_mensaje(int socket_cliente)
{
	int size;
	char *buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s\n", buffer);
	free(buffer);
}

t_list *recibir_paquete(int socket_cliente)
{
	int size;
	int desplazamiento = 0;
	void *buffer;
	t_list *valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while (desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento += sizeof(int);
		char *valor = malloc(tamanio);
		memcpy(valor, buffer + desplazamiento, tamanio);
		desplazamiento += tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}

// Recibir PCB solo se deberia usar cuando se que voy a recibir un paquete del dispatch
t_pcb *recibir_pcb(int socket_cliente)
{
	int size;
	void *buffer;
	buffer = recibir_buffer(&size, socket_cliente);
	t_pcb *pcb = malloc(sizeof(t_pcb));
	t_cpu_registers *registros = malloc(sizeof(t_cpu_registers));
	pcb->cpu_registers = registros;


	memcpy(&(pcb->pid), buffer, sizeof(int));
	buffer += sizeof(int);
	memcpy(&(pcb->quantum), buffer, sizeof(int));
	buffer += sizeof(int);

	memcpy(&(pcb->cpu_registers->pc), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	memcpy(&(pcb->cpu_registers->normales[AX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);
	memcpy(&(pcb->cpu_registers->normales[BX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);
	memcpy(&(pcb->cpu_registers->normales[CX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);
	memcpy(&(pcb->cpu_registers->normales[DX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);

	memcpy(&(pcb->cpu_registers->extendidos[EAX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(pcb->cpu_registers->extendidos[EBX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(pcb->cpu_registers->extendidos[ECX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(pcb->cpu_registers->extendidos[EDX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	memcpy(&(pcb->cpu_registers->si), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(pcb->cpu_registers->di), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	memcpy(&(pcb->estado), buffer, sizeof(estado_proceso));

	buffer = buffer - 2 * sizeof(int) - 7 * sizeof(uint32_t) - 4 * sizeof(uint8_t);
	free(buffer);
	return pcb;
}