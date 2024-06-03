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
	log_trace(logger, "Listo para escuchar a mi cliente");

	return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor, NULL, NULL);
	log_trace(logger, "Se conecto un cliente!");

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

void recibir_ok(int socket_cliente)
{
	int size;
	void *buffer;
	buffer = recibir_buffer(&size, socket_cliente);

	free(buffer);
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

t_creacion_proceso *recibir_creacion_proceso(int socket_cliente)
{
	int size;
	void *buffer;
	buffer = recibir_buffer(&size, socket_cliente);
	t_creacion_proceso *creacion_proceso = malloc(sizeof(t_creacion_proceso));
	t_pcb *pcb = malloc(sizeof(t_pcb));
	t_cpu_registers *registros = malloc(sizeof(t_cpu_registers));
	pcb->cpu_registers = registros;
	creacion_proceso->pcb = pcb;

	memcpy(&(creacion_proceso->pcb->pid), buffer, sizeof(int));
	buffer += sizeof(int);
	memcpy(&(creacion_proceso->pcb->quantum), buffer, sizeof(int));
	buffer += sizeof(int);

	memcpy(&(creacion_proceso->pcb->cpu_registers->pc), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	memcpy(&(creacion_proceso->pcb->cpu_registers->normales[AX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);
	memcpy(&(creacion_proceso->pcb->cpu_registers->normales[BX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);
	memcpy(&(creacion_proceso->pcb->cpu_registers->normales[CX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);
	memcpy(&(creacion_proceso->pcb->cpu_registers->normales[DX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);

	memcpy(&(creacion_proceso->pcb->cpu_registers->extendidos[EAX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(creacion_proceso->pcb->cpu_registers->extendidos[EBX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(creacion_proceso->pcb->cpu_registers->extendidos[ECX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(creacion_proceso->pcb->cpu_registers->extendidos[EDX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	memcpy(&(creacion_proceso->pcb->cpu_registers->si), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(creacion_proceso->pcb->cpu_registers->di), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	memcpy(&(creacion_proceso->pcb->estado), buffer, sizeof(estado_proceso));
	buffer += sizeof(estado_proceso);
	
	memcpy(&(creacion_proceso->tamanio_path), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	creacion_proceso->path = malloc(creacion_proceso->tamanio_path);
	memcpy(creacion_proceso->path, buffer, creacion_proceso->tamanio_path);

	buffer = buffer - 2 * sizeof(int) - 8 * sizeof(uint32_t) - 4 * sizeof(uint8_t) - sizeof(estado_proceso);
	free(buffer);
	return creacion_proceso;
}

t_instruccion *recibir_instruccion(int socket_cliente)
{
	int size;
	void *buffer;
	buffer = recibir_buffer(&size, socket_cliente);
	t_instruccion *instruccion = malloc(sizeof(t_instruccion));
	
	memcpy(&(instruccion->tamanio_instruccion), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	instruccion->instruccion = malloc(instruccion->tamanio_instruccion);
	memcpy(instruccion->instruccion, buffer, instruccion->tamanio_instruccion);

	buffer = buffer - sizeof(uint32_t);
	free(buffer);
	return instruccion;
}

t_interrupcion *recibir_interrupcion(int socket_cliente)
{
	int size;
	void *buffer;
	buffer = recibir_buffer(&size, socket_cliente);
	t_interrupcion *interrupcion = malloc(sizeof(t_interrupcion));
	t_pcb *pcb = malloc(sizeof(t_pcb));
	t_cpu_registers *registros = malloc(sizeof(t_cpu_registers));
	pcb->cpu_registers = registros;
	interrupcion->pcb = pcb;

	memcpy(&(interrupcion->pcb->pid), buffer, sizeof(int));
	buffer += sizeof(int);
	memcpy(&(interrupcion->pcb->quantum), buffer, sizeof(int));
	buffer += sizeof(int);

	memcpy(&(interrupcion->pcb->cpu_registers->pc), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	memcpy(&(interrupcion->pcb->cpu_registers->normales[AX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);
	memcpy(&(interrupcion->pcb->cpu_registers->normales[BX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);
	memcpy(&(interrupcion->pcb->cpu_registers->normales[CX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);
	memcpy(&(interrupcion->pcb->cpu_registers->normales[DX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);

	memcpy(&(interrupcion->pcb->cpu_registers->extendidos[EAX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(interrupcion->pcb->cpu_registers->extendidos[EBX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(interrupcion->pcb->cpu_registers->extendidos[ECX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(interrupcion->pcb->cpu_registers->extendidos[EDX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	memcpy(&(interrupcion->pcb->cpu_registers->si), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(interrupcion->pcb->cpu_registers->di), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	memcpy(&(interrupcion->pcb->estado), buffer, sizeof(estado_proceso));
	buffer += sizeof(estado_proceso);
	
	memcpy(&(interrupcion->motivo), buffer, sizeof(motivo_interrupcion));

	buffer = buffer - 2 * sizeof(int) - 7 * sizeof(uint32_t) - 4 * sizeof(uint8_t) - sizeof(estado_proceso);
	free(buffer);
	return interrupcion;
}

t_sleep *recibir_sleep(int socket_cliente)
{
	int size;
	void *buffer;
	buffer = recibir_buffer(&size, socket_cliente);
	t_sleep *sleep = malloc(sizeof(t_sleep));
	t_pcb *pcb = malloc(sizeof(t_pcb));
	t_cpu_registers *registros = malloc(sizeof(t_cpu_registers));
	pcb->cpu_registers = registros;
	sleep->pcb = pcb;

	memcpy(&(sleep->pcb->pid), buffer, sizeof(int));
	buffer += sizeof(int);
	memcpy(&(sleep->pcb->quantum), buffer, sizeof(int));
	buffer += sizeof(int);

	memcpy(&(sleep->pcb->cpu_registers->pc), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	memcpy(&(sleep->pcb->cpu_registers->normales[AX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);
	memcpy(&(sleep->pcb->cpu_registers->normales[BX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);
	memcpy(&(sleep->pcb->cpu_registers->normales[CX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);
	memcpy(&(sleep->pcb->cpu_registers->normales[DX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);

	memcpy(&(sleep->pcb->cpu_registers->extendidos[EAX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(sleep->pcb->cpu_registers->extendidos[EBX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(sleep->pcb->cpu_registers->extendidos[ECX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(sleep->pcb->cpu_registers->extendidos[EDX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	memcpy(&(sleep->pcb->cpu_registers->si), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(sleep->pcb->cpu_registers->di), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	memcpy(&(sleep->pcb->estado), buffer, sizeof(estado_proceso));
	buffer += sizeof(estado_proceso);
	
	memcpy(&(sleep->tamanio_nombre_interfaz), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	sleep->nombre_interfaz = malloc(sleep->tamanio_nombre_interfaz);
	memcpy(sleep->nombre_interfaz, buffer, sleep->tamanio_nombre_interfaz);
	buffer += sleep->tamanio_nombre_interfaz;

	memcpy(&(sleep->unidades_de_trabajo), buffer, sizeof(uint32_t));

	buffer = buffer - 2 * sizeof(int) - 8 * sizeof(uint32_t) - 4 * sizeof(uint8_t) - sizeof(estado_proceso) - sleep->tamanio_nombre_interfaz;
	free(buffer);
	return sleep;
}

t_nombre_y_tipo_io *recibir_nombre_y_tipo(int socket_cliente)
{
	int size;
	void *buffer;
	buffer = recibir_buffer(&size, socket_cliente);
	t_nombre_y_tipo_io *nombre_y_tipo = malloc(sizeof(t_nombre_y_tipo_io));

	memcpy(&(nombre_y_tipo->tamanio_nombre), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	nombre_y_tipo->nombre = malloc(nombre_y_tipo->tamanio_nombre);
	memcpy(nombre_y_tipo->nombre, buffer, nombre_y_tipo->tamanio_nombre);
	buffer += nombre_y_tipo->tamanio_nombre;

	memcpy(&(nombre_y_tipo->tipo), buffer, sizeof(tipo_interfaz));

	buffer = buffer - sizeof(uint32_t) - nombre_y_tipo->tamanio_nombre;
	free(buffer);
	return nombre_y_tipo;
}

t_resize *recibir_resize(int socket_cliente)
{
	int size;
	void *buffer;
	buffer = recibir_buffer(&size, socket_cliente);
	t_resize *resize = malloc(sizeof(t_resize));
	t_pcb *pcb = malloc(sizeof(t_pcb));
	t_cpu_registers *registros = malloc(sizeof(t_cpu_registers));
	pcb->cpu_registers = registros;
	resize->pcb = pcb;

	memcpy(&(resize->pcb->pid), buffer, sizeof(int));
	buffer += sizeof(int);
	memcpy(&(resize->pcb->quantum), buffer, sizeof(int));
	buffer += sizeof(int);

	memcpy(&(resize->pcb->cpu_registers->pc), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	memcpy(&(resize->pcb->cpu_registers->normales[AX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);
	memcpy(&(resize->pcb->cpu_registers->normales[BX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);
	memcpy(&(resize->pcb->cpu_registers->normales[CX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);
	memcpy(&(resize->pcb->cpu_registers->normales[DX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);

	memcpy(&(resize->pcb->cpu_registers->extendidos[EAX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(resize->pcb->cpu_registers->extendidos[EBX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(resize->pcb->cpu_registers->extendidos[ECX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(resize->pcb->cpu_registers->extendidos[EDX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	memcpy(&(resize->pcb->cpu_registers->si), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(resize->pcb->cpu_registers->di), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	memcpy(&(resize->pcb->estado), buffer, sizeof(estado_proceso));
	buffer += sizeof(estado_proceso);
	
	memcpy(&(resize->tamanio), buffer, sizeof(int));

	buffer = buffer - 2 * sizeof(int) - 7 * sizeof(uint32_t) - 4 * sizeof(uint8_t) - sizeof(estado_proceso);
	free(buffer);
	return resize;
}