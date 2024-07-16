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

int recibir_numero(int socket_cliente)
{
	int size;
	void *buffer;
	buffer = recibir_buffer(&size, socket_cliente);
	t_numero *mensaje_numero = malloc(sizeof(t_numero));
	
	memcpy(&(mensaje_numero->numero), buffer, sizeof(int));

	int numero = mensaje_numero->numero;
	
	free(buffer);
	free(mensaje_numero);
	return numero;
}

t_solicitud_marco *recibir_solicitud_marco(int socket_cliente)
{
	int size;
	void *buffer;
	buffer = recibir_buffer(&size, socket_cliente);
	t_solicitud_marco *solicitud_marco = malloc(sizeof(t_solicitud_marco));
	
	memcpy(&(solicitud_marco->pid), buffer, sizeof(int));
	buffer += sizeof(int);
	memcpy(&(solicitud_marco->pagina), buffer, sizeof(int));

	buffer = buffer - sizeof(int);
	free(buffer);
	return solicitud_marco;
}

t_leer_memoria *recibir_leer_memoria(int socket_cliente)
{
	int size;
	void *buffer;
	buffer = recibir_buffer(&size, socket_cliente);
	t_leer_memoria *leer_memoria = malloc(sizeof(t_leer_memoria));
	
	memcpy(&(leer_memoria->pid), buffer, sizeof(int));
	buffer += sizeof(int);
	memcpy(&(leer_memoria->direccion), buffer, sizeof(int));
	buffer += sizeof(int);
	memcpy(&(leer_memoria->tamanio), buffer, sizeof(int));

	buffer = buffer - 2 * sizeof(int);
	free(buffer);
	return leer_memoria;
}

t_lectura *recibir_lectura(int socket_cliente)
{
	int size;
	void *buffer;
	buffer = recibir_buffer(&size, socket_cliente);
	t_lectura *leido = malloc(sizeof(t_lectura));
	
	memcpy(&(leido->tamanio_lectura), buffer, sizeof(int));
	buffer += sizeof(int);
	leido->lectura = malloc(leido->tamanio_lectura);
	memcpy(leido->lectura, buffer, leido->tamanio_lectura);

	buffer = buffer - sizeof(int);
	free(buffer);
	return leido;
}

t_escribir_memoria *recibir_escribir_memoria(int socket_cliente)
{
	int size;
	void *buffer;
	buffer = recibir_buffer(&size, socket_cliente);
	t_escribir_memoria *escribir_memoria = malloc(sizeof(t_escribir_memoria));
	
	memcpy(&(escribir_memoria->pid), buffer, sizeof(int));
	buffer += sizeof(int);
	memcpy(&(escribir_memoria->direccion), buffer, sizeof(int));
	buffer += sizeof(int);
	memcpy(&(escribir_memoria->tamanio), buffer, sizeof(int));
	buffer += sizeof(int);
	escribir_memoria->valor = malloc(escribir_memoria->tamanio);
	memcpy(escribir_memoria->valor, buffer, escribir_memoria->tamanio);

	buffer = buffer - 3 * sizeof(int);
	free(buffer);
	return escribir_memoria;
}

t_recurso *recibir_recurso(int socket_cliente)
{
	int size;
	void *buffer;
	buffer = recibir_buffer(&size, socket_cliente);
	t_recurso *recurso = malloc(sizeof(t_recurso));
	t_pcb *pcb = malloc(sizeof(t_pcb));
	t_cpu_registers *registros = malloc(sizeof(t_cpu_registers));
	pcb->cpu_registers = registros;
	recurso->pcb = pcb;

	memcpy(&(recurso->pcb->pid), buffer, sizeof(int));
	buffer += sizeof(int);
	memcpy(&(recurso->pcb->quantum), buffer, sizeof(int));
	buffer += sizeof(int);

	memcpy(&(recurso->pcb->cpu_registers->pc), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	memcpy(&(recurso->pcb->cpu_registers->normales[AX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);
	memcpy(&(recurso->pcb->cpu_registers->normales[BX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);
	memcpy(&(recurso->pcb->cpu_registers->normales[CX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);
	memcpy(&(recurso->pcb->cpu_registers->normales[DX]), buffer, sizeof(uint8_t));
	buffer += sizeof(uint8_t);

	memcpy(&(recurso->pcb->cpu_registers->extendidos[EAX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(recurso->pcb->cpu_registers->extendidos[EBX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(recurso->pcb->cpu_registers->extendidos[ECX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(recurso->pcb->cpu_registers->extendidos[EDX]), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	memcpy(&(recurso->pcb->cpu_registers->si), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	memcpy(&(recurso->pcb->cpu_registers->di), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);

	memcpy(&(recurso->pcb->estado), buffer, sizeof(estado_proceso));
	buffer += sizeof(estado_proceso);
	
	memcpy(&(recurso->tamanio_nombre), buffer, sizeof(uint32_t));
	buffer += sizeof(uint32_t);
	recurso->nombre = malloc(recurso->tamanio_nombre);
	memcpy(recurso->nombre, buffer, recurso->tamanio_nombre);

	buffer = buffer - 2 * sizeof(int) - 8 * sizeof(uint32_t) - 4 * sizeof(uint8_t) - sizeof(estado_proceso);
	free(buffer);
	return recurso;
}

t_io_stdin_read *recibir_io_stdin_read(int socket_cliente)
{
    int size;
    void *buffer = recibir_buffer(&size, socket_cliente);
    
    t_io_stdin_read *io_stdin_read = malloc(sizeof(t_io_stdin_read));
    t_pcb *pcb = malloc(sizeof(t_pcb));
    t_cpu_registers *registros = malloc(sizeof(t_cpu_registers));
    pcb->cpu_registers = registros;
    io_stdin_read->pcb = pcb;

    // Copiar datos del buffer a la estructura t_io_stdin_read
    memcpy(&(io_stdin_read->pcb->pid), buffer, sizeof(int));
    buffer += sizeof(int);
    memcpy(&(io_stdin_read->pcb->quantum), buffer, sizeof(int));
    buffer += sizeof(int);

    memcpy(&(io_stdin_read->pcb->cpu_registers->pc), buffer, sizeof(uint32_t));
    buffer += sizeof(uint32_t);

    memcpy(&(io_stdin_read->pcb->cpu_registers->normales[AX]), buffer, sizeof(uint8_t));
    buffer += sizeof(uint8_t);
    memcpy(&(io_stdin_read->pcb->cpu_registers->normales[BX]), buffer, sizeof(uint8_t));
    buffer += sizeof(uint8_t);
    memcpy(&(io_stdin_read->pcb->cpu_registers->normales[CX]), buffer, sizeof(uint8_t));
    buffer += sizeof(uint8_t);
    memcpy(&(io_stdin_read->pcb->cpu_registers->normales[DX]), buffer, sizeof(uint8_t));
    buffer += sizeof(uint8_t);

    memcpy(&(io_stdin_read->pcb->cpu_registers->extendidos[EAX]), buffer, sizeof(uint32_t));
    buffer += sizeof(uint32_t);
    memcpy(&(io_stdin_read->pcb->cpu_registers->extendidos[EBX]), buffer, sizeof(uint32_t));
    buffer += sizeof(uint32_t);
    memcpy(&(io_stdin_read->pcb->cpu_registers->extendidos[ECX]), buffer, sizeof(uint32_t));
    buffer += sizeof(uint32_t);
    memcpy(&(io_stdin_read->pcb->cpu_registers->extendidos[EDX]), buffer, sizeof(uint32_t));
    buffer += sizeof(uint32_t);

    memcpy(&(io_stdin_read->pcb->cpu_registers->si), buffer, sizeof(uint32_t));
    buffer += sizeof(uint32_t);
    memcpy(&(io_stdin_read->pcb->cpu_registers->di), buffer, sizeof(uint32_t));
    buffer += sizeof(uint32_t);

    memcpy(&(io_stdin_read->pcb->estado), buffer, sizeof(estado_proceso));
    buffer += sizeof(estado_proceso);

    memcpy(&(io_stdin_read->tamanio_nombre_interfaz), buffer, sizeof(uint32_t));
    buffer += sizeof(uint32_t);

    io_stdin_read->nombre_interfaz = malloc(io_stdin_read->tamanio_nombre_interfaz);
    memcpy(io_stdin_read->nombre_interfaz, buffer, io_stdin_read->tamanio_nombre_interfaz);
    buffer += io_stdin_read->tamanio_nombre_interfaz;

    memcpy(&(io_stdin_read->tamanio_contenido), buffer, sizeof(uint32_t));
    buffer += sizeof(uint32_t);

    // Implementación para copiar las direcciones físicas de `buffer` a `t_list`
    io_stdin_read->direcciones_fisicas = list_create();
    int num_direcciones = size / sizeof(uint32_t); // Suponiendo que las direcciones son de tamaño uint32_t
    for (int i = 0; i < num_direcciones; i++) {
        uint32_t *direccion = malloc(sizeof(uint32_t));
        memcpy(direccion, buffer, sizeof(uint32_t));
        list_add(io_stdin_read->direcciones_fisicas, direccion);
        buffer += sizeof(uint32_t);
    }

    free(buffer - size); // Liberar buffer original
    return io_stdin_read;
}

t_io_stdout_write* recibir_io_stdout_write(int socket_cliente) {
    int size;
    void *buffer = recibir_buffer(&size, socket_cliente);
    t_io_stdout_write *io_stdout_write = malloc(sizeof(t_io_stdout_write));
    int desplazamiento = 0;

    // Recibir datos del PCB
    io_stdout_write->pcb = malloc(sizeof(t_pcb));
    io_stdout_write->pcb->cpu_registers = malloc(sizeof(t_cpu_registers));
    memcpy(&(io_stdout_write->pcb->pid), buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&(io_stdout_write->pcb->quantum), buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&(io_stdout_write->pcb->cpu_registers->pc), buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&(io_stdout_write->pcb->cpu_registers->normales[AX]), buffer + desplazamiento, sizeof(uint8_t));
    desplazamiento += sizeof(uint8_t);
    memcpy(&(io_stdout_write->pcb->cpu_registers->normales[BX]), buffer + desplazamiento, sizeof(uint8_t));
    desplazamiento += sizeof(uint8_t);
    memcpy(&(io_stdout_write->pcb->cpu_registers->normales[CX]), buffer + desplazamiento, sizeof(uint8_t));
    desplazamiento += sizeof(uint8_t);
    memcpy(&(io_stdout_write->pcb->cpu_registers->normales[DX]), buffer + desplazamiento, sizeof(uint8_t));
    desplazamiento += sizeof(uint8_t);
    memcpy(&(io_stdout_write->pcb->cpu_registers->extendidos[EAX]), buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&(io_stdout_write->pcb->cpu_registers->extendidos[EBX]), buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&(io_stdout_write->pcb->cpu_registers->extendidos[ECX]), buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&(io_stdout_write->pcb->cpu_registers->extendidos[EDX]), buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&(io_stdout_write->pcb->cpu_registers->si), buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&(io_stdout_write->pcb->cpu_registers->di), buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(&(io_stdout_write->pcb->estado), buffer + desplazamiento, sizeof(estado_proceso));
    desplazamiento += sizeof(estado_proceso);

    // Recibir datos de la interfaz y tamaño de nombre de interfaz
    memcpy(&(io_stdout_write->tamanio_nombre_interfaz), buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    io_stdout_write->nombre_interfaz = malloc(io_stdout_write->tamanio_nombre_interfaz);
    memcpy(io_stdout_write->nombre_interfaz, buffer + desplazamiento, io_stdout_write->tamanio_nombre_interfaz);
    desplazamiento += io_stdout_write->tamanio_nombre_interfaz;

    // Recibir dirección lógica
    memcpy(&(io_stdout_write->direccion_logica), buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    // Recibir tamaño
    memcpy(&(io_stdout_write->tamaño), buffer + desplazamiento, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    free(buffer);
    return io_stdout_write;
}
