#include "utils_cliente.h"


void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(op_code));
	desplazamiento+= sizeof(op_code);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

int crear_conexion(char *ip, char* puerto)
{
	struct addrinfo hints;
	struct addrinfo *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &servinfo);

	// Ahora vamos a crear el socket.
	int socket_cliente = socket(servinfo->ai_family,
                    servinfo->ai_socktype,
                    servinfo->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo
	while (connect(socket_cliente, servinfo->ai_addr, servinfo->ai_addrlen) == -1)
	{
		log_info(logger,"Intentando conectar con %s:%s ...", ip, puerto);
		sleep(3);
	}

	freeaddrinfo(servinfo);

	log_trace(logger, "Conectado con %s:%s", ip, puerto);

	return socket_cliente;
}

void enviar_mensaje(char* mensaje, int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}


void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

t_paquete* crear_paquete(void)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = PAQUETE;
	crear_buffer(paquete);
	return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void enviar_paquete_alternativo(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}
// Función para establecer la conexión con un módulo. Si falla, tira error.
int conectar_modulo(char* ip, char* puerto)
{
    int socket_modulo = crear_conexion(ip, puerto);
    if (socket_modulo == -1) {
        log_info(logger,"Error al conectar con el módulo en %s:%s.\n", ip, puerto);
    }
    return socket_modulo;
}

//############################################
//Para crear paquetes de distintas operaciones, y poder ir mandandolos con una identificacion.
t_paquete* crear_paquete_pcb(op_code cod_op) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = cod_op;
	crear_buffer(paquete);
    // Crear un buffer para almacenar el PCB
    paquete->buffer->size = 7 * sizeof(uint32_t) 
							+ 2 * sizeof(int) 
							+ sizeof(estado_proceso)
							+ 4 * sizeof(uint8_t); // Tamaño del PCB
	void *magic = malloc(paquete->buffer->size);
    paquete->buffer->stream = magic; // Reservar memoria para el PCB
	free(magic);
    return paquete;
}

t_paquete* crear_paquete_pcb_con_cadena(uint32_t tamanio_cadena, op_code cod_op) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = cod_op;
	crear_buffer(paquete);
    paquete->buffer->size = 8 * sizeof(uint32_t) 
							+ 2 * sizeof(int) 
							+ sizeof(estado_proceso)
							+ 4 * sizeof(uint8_t)
							+ tamanio_cadena;
	void *magic = malloc(paquete->buffer->size);
    paquete->buffer->stream = magic;
	free(magic);
    return paquete;
}

t_paquete* crear_paquete_instruccion(uint32_t tamanio_instruccion) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = INSTRUCCION;
	crear_buffer(paquete);
    paquete->buffer->size = sizeof(uint32_t) 
							+ tamanio_instruccion;
	void *magic = malloc(paquete->buffer->size);
    paquete->buffer->stream = magic;
	free(magic);
    return paquete;
}
t_paquete* crear_paquete_interrupcion() {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = INTERRUPCION;
	crear_buffer(paquete);
    paquete->buffer->size = 7 * sizeof(uint32_t) 
							+ 2 * sizeof(int) 
							+ sizeof(estado_proceso)
							+ 4 * sizeof(uint8_t)
							+ sizeof(motivo_interrupcion);
	void *magic = malloc(paquete->buffer->size);
    paquete->buffer->stream = magic;
	free(magic);
    return paquete;
}

t_paquete* crear_paquete_sleep(uint32_t tamanio_nombre_interfaz) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = IO_GEN_SLEEP;
	crear_buffer(paquete);
    paquete->buffer->size = 9 * sizeof(uint32_t) 
							+ 2 * sizeof(int) 
							+ sizeof(estado_proceso)
							+ 4 * sizeof(uint8_t)
							+ tamanio_nombre_interfaz;
	void *magic = malloc(paquete->buffer->size);
    paquete->buffer->stream = magic;
	free(magic);
    return paquete;
}

t_paquete* crear_paquete_nombre_y_tipo(uint32_t tamanio_nombre) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = NOMBRE_Y_TIPO_IO;
	crear_buffer(paquete);
    paquete->buffer->size =  sizeof(uint32_t) 
							+ tamanio_nombre
							+ sizeof(tipo_interfaz);
	void *magic = malloc(paquete->buffer->size);
    paquete->buffer->stream = magic;
	free(magic);
    return paquete;
}

t_paquete* crear_paquete_resize() {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = RESIZE;
	crear_buffer(paquete);
    paquete->buffer->size = 7 * sizeof(uint32_t) 
							+ 3 * sizeof(int) 
							+ sizeof(estado_proceso)
							+ 4 * sizeof(uint8_t);
	void *magic = malloc(paquete->buffer->size);
    paquete->buffer->stream = magic;
	free(magic);
    return paquete;
}

t_paquete* crear_paquete_numero(op_code cod_op) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = cod_op;
	crear_buffer(paquete);
    paquete->buffer->size = sizeof(int);
	void *magic = malloc(paquete->buffer->size);
    paquete->buffer->stream = magic;
	free(magic);
    return paquete;
}

t_paquete* crear_paquete_solicitud_marco() {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = SOLICITUD_MARCO;
	crear_buffer(paquete);
    paquete->buffer->size = 2 * sizeof(int);
	void *magic = malloc(paquete->buffer->size);
    paquete->buffer->stream = magic;
	free(magic);
    return paquete;
}

t_paquete* crear_paquete_leer_memoria() {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = LEER_MEMORIA;
	crear_buffer(paquete);
    paquete->buffer->size = 3 * sizeof(int);
	void *magic = malloc(paquete->buffer->size);
    paquete->buffer->stream = magic;
	free(magic);
    return paquete;
}

t_paquete* crear_paquete_lectura(int tamanio_lectura) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = MEMORIA_LEIDA;
	crear_buffer(paquete);
    paquete->buffer->size = sizeof(int)
							+ tamanio_lectura;
	void *magic = malloc(paquete->buffer->size);
    paquete->buffer->stream = magic;
	free(magic);
    return paquete;
}

t_paquete* crear_paquete_escribir_memoria(int tamanio_valor) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = ESCRIBIR_MEMORIA;
	crear_buffer(paquete);
    paquete->buffer->size = 3 * sizeof(int)
							+ tamanio_valor;
	void *magic = malloc(paquete->buffer->size);
    paquete->buffer->stream = magic;
	free(magic);
    return paquete;
}

void agregar_pcb_a_paquete(t_paquete* paquete, t_pcb* pcb) {
    // Copiar el PCB al stream del buffer del paquete
	void * magic = malloc(paquete->buffer->size);
	int desplazamiento = 0;

    memcpy(magic + desplazamiento, &(pcb->pid), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(pcb->quantum), sizeof(int));
	desplazamiento+= sizeof(int);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->pc), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[AX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[BX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[CX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[DX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[EAX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[EBX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[ECX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[EDX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->si), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->di), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);

	memcpy(magic + desplazamiento, &(pcb->estado), sizeof(estado_proceso));

	paquete->buffer->stream = magic;
}

void agregar_pcb_con_cadena_a_paquete(t_paquete* paquete, t_pcb* pcb, char* cadena, uint32_t tamanio_cadena) {
	void * magic = malloc(paquete->buffer->size);
	int desplazamiento = 0;

    memcpy(magic + desplazamiento, &(pcb->pid), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(pcb->quantum), sizeof(int));
	desplazamiento+= sizeof(int);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->pc), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[AX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[BX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[CX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[DX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[EAX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[EBX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[ECX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[EDX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->si), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->di), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);

	memcpy(magic + desplazamiento, &(pcb->estado), sizeof(estado_proceso));
	desplazamiento+= sizeof(estado_proceso);

	memcpy(magic + desplazamiento, &tamanio_cadena, sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, cadena, tamanio_cadena);

	paquete->buffer->stream = magic;
}

void agregar_instruccion_a_paquete(t_paquete* paquete, char* instruccion, uint32_t tamanio_instruccion) {
	void * magic = malloc(paquete->buffer->size);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &tamanio_instruccion, sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, instruccion, tamanio_instruccion);

	paquete->buffer->stream = magic;
}

void agregar_interrupcion_a_paquete(t_paquete* paquete, t_pcb* pcb, motivo_interrupcion motivo) {
	void * magic = malloc(paquete->buffer->size);
	int desplazamiento = 0;

    memcpy(magic + desplazamiento, &(pcb->pid), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(pcb->quantum), sizeof(int));
	desplazamiento+= sizeof(int);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->pc), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[AX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[BX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[CX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[DX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[EAX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[EBX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[ECX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[EDX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->si), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->di), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);

	memcpy(magic + desplazamiento, &(pcb->estado), sizeof(estado_proceso));
	desplazamiento+= sizeof(estado_proceso);

	memcpy(magic + desplazamiento, &motivo, sizeof(motivo_interrupcion));

	paquete->buffer->stream = magic;
}

void agregar_sleep_a_paquete(t_paquete* paquete, t_pcb* pcb, char* nombre_interfaz, uint32_t tamanio_nombre_interfaz, uint32_t unidades_de_trabajo) {
	void * magic = malloc(paquete->buffer->size);
	int desplazamiento = 0;

    memcpy(magic + desplazamiento, &(pcb->pid), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(pcb->quantum), sizeof(int));
	desplazamiento+= sizeof(int);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->pc), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[AX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[BX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[CX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[DX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[EAX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[EBX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[ECX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[EDX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->si), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->di), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);

	memcpy(magic + desplazamiento, &(pcb->estado), sizeof(estado_proceso));
	desplazamiento+= sizeof(estado_proceso);

	memcpy(magic + desplazamiento, &tamanio_nombre_interfaz, sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, nombre_interfaz, tamanio_nombre_interfaz);
	desplazamiento+= tamanio_nombre_interfaz;

	memcpy(magic + desplazamiento, &unidades_de_trabajo, sizeof(uint32_t));

	paquete->buffer->stream = magic;
}
void agregar_stidn_read_a_paquete(t_paquete* paquete, t_pcb* pcb, char* nombre_interfaz, uint32_t tamanio_nombre_interfaz, uint32_t unidades_de_trabajo) {
	void * magic = malloc(paquete->buffer->size);
	int desplazamiento = 0;

    memcpy(magic + desplazamiento, &(pcb->pid), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(pcb->quantum), sizeof(int));
	desplazamiento+= sizeof(int);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->pc), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[AX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[BX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[CX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[DX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[EAX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[EBX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[ECX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[EDX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->si), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->di), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);

	memcpy(magic + desplazamiento, &(pcb->estado), sizeof(estado_proceso));
	desplazamiento+= sizeof(estado_proceso);

	memcpy(magic + desplazamiento, &tamanio_nombre_interfaz, sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, nombre_interfaz, tamanio_nombre_interfaz);
	desplazamiento+= tamanio_nombre_interfaz;

	memcpy(magic + desplazamiento, &unidades_de_trabajo, sizeof(uint32_t));

	paquete->buffer->stream = magic;
}

void agregar_nombre_y_tipo_a_paquete(t_paquete* paquete, char *nombre, uint32_t tamanio_nombre, tipo_interfaz tipo) {
	void * magic = malloc(paquete->buffer->size);
	int desplazamiento = 0;

    memcpy(magic + desplazamiento, &tamanio_nombre, sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, nombre, tamanio_nombre);
	desplazamiento+= tamanio_nombre;

	memcpy(magic + desplazamiento, &tipo, sizeof(tipo_interfaz));

	paquete->buffer->stream = magic;
}

void agregar_resize_a_paquete(t_paquete* paquete, t_pcb* pcb, int tamanio) {
	void * magic = malloc(paquete->buffer->size);
	int desplazamiento = 0;

    memcpy(magic + desplazamiento, &(pcb->pid), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(pcb->quantum), sizeof(int));
	desplazamiento+= sizeof(int);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->pc), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[AX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[BX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[CX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->normales[DX]), sizeof(uint8_t));
	desplazamiento+= sizeof(u_int8_t);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[EAX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[EBX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[ECX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->extendidos[EDX]), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);

	memcpy(magic + desplazamiento, &(pcb->cpu_registers->si), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);
	memcpy(magic + desplazamiento, &(pcb->cpu_registers->di), sizeof(uint32_t));
	desplazamiento+= sizeof(u_int32_t);

	memcpy(magic + desplazamiento, &(pcb->estado), sizeof(estado_proceso));
	desplazamiento+= sizeof(estado_proceso);

	memcpy(magic + desplazamiento, &tamanio, sizeof(int));

	paquete->buffer->stream = magic;
}

void agregar_numero_a_paquete(t_paquete* paquete, int numero) {
	void * magic = malloc(paquete->buffer->size);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &numero, sizeof(int));

	paquete->buffer->stream = magic;
}

void agregar_solicitud_marco_a_paquete(t_paquete* paquete, int pid, int pagina) {
	void * magic = malloc(paquete->buffer->size);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &pid, sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &pagina, sizeof(int));
	
	paquete->buffer->stream = magic;
}

void agregar_leer_memoria_a_paquete(t_paquete* paquete, int pid, int direccion, int tamanio) {
	void * magic = malloc(paquete->buffer->size);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &pid, sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &direccion, sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &tamanio, sizeof(int));
	
	paquete->buffer->stream = magic;
}

void agregar_lectura_a_paquete(t_paquete* paquete, void *lectura, int tamanio_lectura) {
	void * magic = malloc(paquete->buffer->size);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &tamanio_lectura, sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, lectura, tamanio_lectura);
	
	paquete->buffer->stream = magic;
}

void agregar_escribir_memoria_a_paquete(t_paquete* paquete, int pid, int direccion, int tamanio, void *valor) {
	void * magic = malloc(paquete->buffer->size);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &pid, sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &direccion, sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &tamanio, sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, valor, tamanio);
	
	paquete->buffer->stream = magic;
}

void enviar_paquete(t_paquete* paquete, int socket_cliente) 
{
    // Calcular el tamaño total del paquete
    int bytes = paquete->buffer->size + sizeof(op_code) + sizeof(int);
    // Serializar el paquete
    void* a_enviar = serializar_paquete(paquete, bytes);
    // Enviar el paquete a través del socket
    send(socket_cliente, a_enviar, bytes, 0);
	free(a_enviar);
    // Liberar la memoria utilizada por el paquete y su buffer
	eliminar_paquete(paquete);
}

void enviar_mensaje_simple(int socket_cliente, op_code cod_op)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = cod_op;
	crear_buffer(paquete);
    paquete->buffer->size = sizeof(uint8_t);

	void * magic = malloc(paquete->buffer->size);
	int desplazamiento = 0;

	memcpy(magic, &desplazamiento, sizeof(uint8_t));

	paquete->buffer->stream = magic;

	enviar_paquete(paquete, socket_cliente);
}

void enviar_pcb(int socket_cliente, t_pcb *pcb)
{
    // Crear un paquete para enviar los PCBs
    t_paquete* paquete = crear_paquete_pcb(DISPATCH);
    //Agrega el PCB a enviar
    agregar_pcb_a_paquete(paquete, pcb);
    //Lo envia a traves de la conexion
    enviar_paquete(paquete, socket_cliente);
}

void enviar_creacion_proceso(int socket_cliente, t_pcb *pcb, char *path)
{
	uint32_t tamanio_path = string_length(path) + 1;
	t_paquete* paquete = crear_paquete_pcb_con_cadena(tamanio_path, CREACION_PROCESO);

	agregar_pcb_con_cadena_a_paquete(paquete, pcb, path, tamanio_path);

	enviar_paquete(paquete, socket_cliente);
}

void enviar_solicitud_instruccion(int socket_cliente, t_pcb *pcb)
{
	t_paquete* paquete = crear_paquete_pcb(SOLICITUD_INSTRUCCION);

	agregar_pcb_a_paquete(paquete, pcb);

	enviar_paquete(paquete, socket_cliente);
}

void enviar_instruccion(int socket_cliente, char *instruccion)
{
	uint32_t tamanio_instruccion = string_length(instruccion) + 1;
	t_paquete* paquete = crear_paquete_instruccion(tamanio_instruccion);

	agregar_instruccion_a_paquete(paquete, instruccion, tamanio_instruccion);

	enviar_paquete(paquete, socket_cliente);
}

void enviar_exit(int socket_cliente, t_pcb *pcb)
{
    t_paquete* paquete = crear_paquete_pcb(INSTRUCCION_EXIT);

    agregar_pcb_a_paquete(paquete, pcb);

    enviar_paquete(paquete, socket_cliente);
}

void enviar_interrupcion(int socket_cliente, t_pcb *pcb, motivo_interrupcion motivo)
{
	t_paquete* paquete = crear_paquete_interrupcion();

	agregar_interrupcion_a_paquete(paquete, pcb, motivo);

	enviar_paquete(paquete, socket_cliente);
}

void enviar_sleep(int socket_cliente, t_pcb *pcb, char *nombre_interfaz, uint32_t unidades_de_trabajo)
{
	uint32_t tamanio_nombre_interfaz = string_length(nombre_interfaz) + 1;
	t_paquete* paquete = crear_paquete_sleep(tamanio_nombre_interfaz);

	agregar_sleep_a_paquete(paquete, pcb, nombre_interfaz, tamanio_nombre_interfaz, unidades_de_trabajo);

	enviar_paquete(paquete, socket_cliente);
}

void enviar_stdin_read(int socket_cliente, t_pcb *pcb, char *nombre_interfaz, t_list *direcciones)
{
	t_paquete *paquete = crear_paquete();
	paquete->codigo_operacion = IO_STDIN_READ;

	agregar_a_paquete(paquete, pcb, sizeof(t_pcb));
	agregar_a_paquete(paquete, nombre_interfaz, sizeof(nombre_interfaz));
	agregar_a_paquete(paquete, direcciones, sizeof(t_list));
	enviar_paquete(paquete, socket_cliente);
	eliminar_paquete(paquete);
}

void enviar_nombre_y_tipo(int socket_cliente, char *nombre, tipo_interfaz tipo)
{
	uint32_t tamanio_nombre = string_length(nombre) + 1;
	t_paquete* paquete = crear_paquete_nombre_y_tipo(tamanio_nombre);

	agregar_nombre_y_tipo_a_paquete(paquete, nombre, tamanio_nombre, tipo);

	enviar_paquete(paquete, socket_cliente);
}

void enviar_fin_sleep(int socket_cliente, t_pcb *pcb)
{
	t_paquete* paquete = crear_paquete_pcb(FIN_SLEEP);

	agregar_pcb_a_paquete(paquete, pcb);

	enviar_paquete(paquete, socket_cliente);
}


void enviar_fin_io_read(int socket_cliente, t_pcb *pcb)
{
	t_paquete* paquete = crear_paquete_pcb(FIN_IO_READ);

	agregar_pcb_a_paquete(paquete, pcb);

	enviar_paquete(paquete, socket_cliente);
}

void enviar_fin_io_write(int socket_cliente, t_pcb *pcb)
{
	t_paquete* paquete = crear_paquete_pcb(FIN_IO_WRITE);

	agregar_pcb_a_paquete(paquete, pcb);

	enviar_paquete(paquete, socket_cliente);
}

void enviar_resize(int socket_cliente, t_pcb *pcb, int tamanio)
{
	t_paquete* paquete = crear_paquete_resize();

	agregar_resize_a_paquete(paquete, pcb, tamanio);

	enviar_paquete(paquete, socket_cliente);
}

void enviar_out_of_memory(int socket_cliente, t_pcb *pcb)
{
    t_paquete* paquete = crear_paquete_pcb(OUT_OF_MEMORY);

    agregar_pcb_a_paquete(paquete, pcb);

    enviar_paquete(paquete, socket_cliente);
}

void enviar_finalizacion_proceso(int socket_cliente, t_pcb *pcb)
{
    t_paquete* paquete = crear_paquete_pcb(FINALIZACION_PROCESO);

    agregar_pcb_a_paquete(paquete, pcb);

    enviar_paquete(paquete, socket_cliente);
}

void enviar_numero(int socket_cliente, int numero, op_code cod_op)
{
    t_paquete* paquete = crear_paquete_numero(cod_op);

    agregar_numero_a_paquete(paquete, numero);

    enviar_paquete(paquete, socket_cliente);
}

void enviar_solicitud_marco(int socket_cliente, int pid, int pagina)
{
	t_paquete* paquete = crear_paquete_solicitud_marco();

	agregar_solicitud_marco_a_paquete(paquete, pid, pagina);

	enviar_paquete(paquete, socket_cliente);
}

void enviar_leer_memoria(int socket_cliente, int pid, int direccion, int tamanio)
{
	t_paquete* paquete = crear_paquete_leer_memoria();

	agregar_leer_memoria_a_paquete(paquete, pid, direccion, tamanio);

	enviar_paquete(paquete, socket_cliente);
}

void enviar_lectura(int socket_cliente, void *lectura, int tamanio_lectura)
{
	t_paquete* paquete = crear_paquete_lectura(tamanio_lectura);

	agregar_lectura_a_paquete(paquete, lectura, tamanio_lectura);

	enviar_paquete(paquete, socket_cliente);
}

void enviar_escribir_memoria(int socket_cliente, int pid, int direccion, int tamanio, void *valor)
{
	t_paquete* paquete = crear_paquete_escribir_memoria(tamanio);

	agregar_escribir_memoria_a_paquete(paquete, pid, direccion, tamanio, valor);

	enviar_paquete(paquete, socket_cliente);
}

void enviar_wait(int socket_cliente, t_pcb *pcb, char *recurso)
{
	uint32_t tamanio_recurso = string_length(recurso) + 1;
	t_paquete* paquete = crear_paquete_pcb_con_cadena(tamanio_recurso, WAIT);

	agregar_pcb_con_cadena_a_paquete(paquete, pcb, recurso, tamanio_recurso);

	enviar_paquete(paquete, socket_cliente);
}

void enviar_signal(int socket_cliente, t_pcb *pcb, char *recurso)
{
	uint32_t tamanio_recurso = string_length(recurso) + 1;
	t_paquete* paquete = crear_paquete_pcb_con_cadena(tamanio_recurso, SIGNAL);

	agregar_pcb_con_cadena_a_paquete(paquete, pcb, recurso, tamanio_recurso);

	enviar_paquete(paquete, socket_cliente);
}

t_io_std* crear_io_std(t_pcb* pcb, char* nombre_interfaz, uint32_t tamanio_nombre_interfaz, uint32_t tamanio_contenido, t_list* direcciones_fisicas) {
    t_io_std* io_stdin_read = malloc(sizeof(t_io_std));
    if (io_stdin_read == NULL) {
        return NULL; // Manejo de error: no se pudo asignar memoria
    }

    io_stdin_read->pcb = pcb;
    io_stdin_read->nombre_interfaz = strdup(nombre_interfaz); // Duplicar la cadena para evitar problemas de gestión de memoria
    io_stdin_read->tamanio_nombre_interfaz = tamanio_nombre_interfaz;
    io_stdin_read->tamanio_contenido = tamanio_contenido;
    io_stdin_read->direcciones_fisicas = direcciones_fisicas; // Asignación directa, asegúrate de gestionar la memoria adecuadamente

    return io_stdin_read;
}

t_paquete* crear_paquete_io_stdin_read(uint32_t tamanio_interfaz, uint32_t cantidad_direcciones) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = IO_STDIN_READ;
    crear_buffer(paquete);

    // Calcular el tamaño total del paquete
    paquete->buffer->size = 9 * sizeof(uint32_t) 
                            + 2 * sizeof(int) 
                            + sizeof(estado_proceso)
                            + 4 * sizeof(uint8_t)
                            + tamanio_interfaz
                            + sizeof(uint32_t)  // Tamaño de la cantidad de direcciones
                            + cantidad_direcciones * 2 * sizeof(int);

    // Asignar memoria para el stream de datos del buffer
    void *magic = malloc(paquete->buffer->size);
    paquete->buffer->stream = magic;

    return paquete;
}

t_paquete* crear_paquete_io_stdout_write(uint32_t tamanio_interfaz, uint32_t cantidad_direcciones) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = IO_STDOUT_WRITE;
    crear_buffer(paquete);

    // Calcular el tamaño total del paquete
    paquete->buffer->size = 9 * sizeof(uint32_t) 
                            + 2 * sizeof(int) 
                            + sizeof(estado_proceso)
                            + 4 * sizeof(uint8_t)
                            + tamanio_interfaz
                            + sizeof(uint32_t)  // Tamaño de la cantidad de direcciones
                            + cantidad_direcciones * 2 * sizeof(int);

    // Asignar memoria para el stream de datos del buffer
    void *magic = malloc(paquete->buffer->size);
    paquete->buffer->stream = magic;

    return paquete;
}

void agregar_io_std_a_paquete(t_paquete* paquete, t_io_std* io_std, uint32_t cantidad_direcciones) {
    int desplazamiento = 0;

    // Copiar datos del PCB
    memcpy(paquete->buffer->stream + desplazamiento, &(io_std->pcb->pid), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_std->pcb->quantum), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_std->pcb->cpu_registers->pc), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_std->pcb->cpu_registers->normales[AX]), sizeof(uint8_t));
    desplazamiento += sizeof(uint8_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_std->pcb->cpu_registers->normales[BX]), sizeof(uint8_t));
    desplazamiento += sizeof(uint8_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_std->pcb->cpu_registers->normales[CX]), sizeof(uint8_t));
    desplazamiento += sizeof(uint8_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_std->pcb->cpu_registers->normales[DX]), sizeof(uint8_t));
    desplazamiento += sizeof(uint8_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_std->pcb->cpu_registers->extendidos[EAX]), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_std->pcb->cpu_registers->extendidos[EBX]), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_std->pcb->cpu_registers->extendidos[ECX]), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_std->pcb->cpu_registers->extendidos[EDX]), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_std->pcb->cpu_registers->si), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_std->pcb->cpu_registers->di), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_std->pcb->estado), sizeof(estado_proceso));
    desplazamiento += sizeof(estado_proceso);

    // Copiar datos de la interfaz y tamaño de nombre de interfaz
    memcpy(paquete->buffer->stream + desplazamiento, &(io_std->tamanio_nombre_interfaz), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, io_std->nombre_interfaz, io_std->tamanio_nombre_interfaz);
    desplazamiento += io_std->tamanio_nombre_interfaz;

	memcpy(paquete->buffer->stream + desplazamiento, &cantidad_direcciones, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    // Copiar direcciones físicas
    for (int i = 0; i < list_size(io_std->direcciones_fisicas); i++) {
        t_direccion_y_tamanio* direccion = list_get(io_std->direcciones_fisicas, i);
        memcpy(paquete->buffer->stream + desplazamiento, &(direccion->direccion), sizeof(int));
		desplazamiento += sizeof(int);
		memcpy(paquete->buffer->stream + desplazamiento, &(direccion->tamanio), sizeof(int));
        desplazamiento += sizeof(int);
    }

    // Copiar tamaño del contenido
    memcpy(paquete->buffer->stream + desplazamiento, &(io_std->tamanio_contenido), sizeof(uint32_t));
}

void enviar_io_stdin_read(int socket_cliente, t_io_std* io_stdin_read) {
    uint32_t tamanio_nombre_interfaz = string_length(io_stdin_read->nombre_interfaz) + 1;
    uint32_t cantidad_direcciones = list_size(io_stdin_read->direcciones_fisicas);
    t_paquete* paquete = crear_paquete_io_stdin_read(tamanio_nombre_interfaz, cantidad_direcciones);

    agregar_io_std_a_paquete(paquete, io_stdin_read, cantidad_direcciones);
	list_destroy_and_destroy_elements(io_stdin_read->direcciones_fisicas, destruir_direccion);

    enviar_paquete(paquete, socket_cliente);
}

void enviar_io_stdout_write(int socket_cliente, t_io_std* io_stdout_write) {
    uint32_t tamanio_nombre_interfaz = string_length(io_stdout_write->nombre_interfaz) + 1;
    uint32_t cantidad_direcciones = list_size(io_stdout_write->direcciones_fisicas);
    t_paquete* paquete = crear_paquete_io_stdout_write(tamanio_nombre_interfaz, cantidad_direcciones);

    agregar_io_std_a_paquete(paquete, io_stdout_write, cantidad_direcciones);
	list_destroy_and_destroy_elements(io_stdout_write->direcciones_fisicas, destruir_direccion);

    enviar_paquete(paquete, socket_cliente);
}

void destruir_direccion(void *elem)
{
    t_direccion_y_tamanio *direccion_y_tamanio = (t_direccion_y_tamanio *)elem;
    free(direccion_y_tamanio);
}

void enviar_io_fs_create(int socket_cliente, t_io_fs_create* io_fs_create) {
    t_paquete* paquete = crear_paquete_io_fs_create(strlen(io_fs_create->nombre_interfaz) + 1, strlen(io_fs_create->nombre_archivo) + 1);

    agregar_io_fs_create_a_paquete(paquete, io_fs_create);

    enviar_paquete(paquete, socket_cliente);
    eliminar_paquete(paquete);
}

t_paquete* crear_paquete_io_fs_create(uint32_t tamanio_nombre_interfaz, uint32_t tamanio_nombre_archivo) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = IO_FS_CREATE;
    crear_buffer(paquete);

    // Calcular el tamaño total del paquete, sumando todos los tamaños de los campos que se enviarán
    paquete->buffer->size = 2 * sizeof(uint32_t) // tamanio_nombre_interfaz y tamanio_nombre_archivo
                            + tamanio_nombre_interfaz
                            + tamanio_nombre_archivo
                            + sizeof(uint32_t)  // PID
                            + sizeof(uint32_t)  // Quantum
                            + sizeof(uint32_t)  // Program Counter
                            + 4 * sizeof(uint8_t)  // Registros normales (AX, BX, CX, DX)
                            + 4 * sizeof(uint32_t) // Registros extendidos (EAX, EBX, ECX, EDX)
                            + 2 * sizeof(uint32_t) // SI, DI
                            + sizeof(estado_proceso); // Estado del proceso

    // Asignar memoria para el stream del buffer
    paquete->buffer->stream = malloc(paquete->buffer->size);

    return paquete;
}

void agregar_io_fs_create_a_paquete(t_paquete* paquete, t_io_fs_create* io_fs_create) {
    int desplazamiento = 0;

    // Copiar datos del PCB
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_create->pcb->pid), sizeof(int));
    desplazamiento += sizeof(int);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_create->pcb->quantum), sizeof(int));
    desplazamiento += sizeof(int);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_create->pcb->cpu_registers->pc), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_create->pcb->cpu_registers->normales[0]), sizeof(uint8_t) * 4);
    desplazamiento += sizeof(uint8_t) * 4;
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_create->pcb->cpu_registers->extendidos[0]), sizeof(uint32_t) * 4);
    desplazamiento += sizeof(uint32_t) * 4;
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_create->pcb->cpu_registers->si), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_create->pcb->cpu_registers->di), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_create->pcb->estado), sizeof(estado_proceso));
    desplazamiento += sizeof(estado_proceso);

    // Copiar datos de la interfaz y tamaño de nombre de interfaz
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_create->tamanio_nombre_interfaz), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, io_fs_create->nombre_interfaz, io_fs_create->tamanio_nombre_interfaz);
    desplazamiento += io_fs_create->tamanio_nombre_interfaz;

    // Copiar datos del nombre del archivo y tamaño de nombre del archivo
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_create->tamanio_nombre_archivo), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, io_fs_create->nombre_archivo, io_fs_create->tamanio_nombre_archivo);
    desplazamiento += io_fs_create->tamanio_nombre_archivo;
	
}

t_io_fs_create* crear_io_fs_create(t_pcb* pcb, char* nombre_interfaz, char* nombre_archivo) {
    // Crear y asignar memoria para la nueva estructura
    t_io_fs_create* io_fs_create = malloc(sizeof(t_io_fs_create));
    if (io_fs_create == NULL) {
        return NULL; // Manejo de error: no se pudo asignar memoria
    }
    // Asignar el PCB
    io_fs_create->pcb = pcb;
    // Asignar y duplicar el nombre de la interfaz
    io_fs_create->nombre_interfaz = strdup(nombre_interfaz);
    io_fs_create->tamanio_nombre_interfaz = strlen(io_fs_create->nombre_interfaz) + 1;
    // Asignar y duplicar el nombre del archivo
    io_fs_create->nombre_archivo = strdup(nombre_archivo);
    io_fs_create->tamanio_nombre_archivo = strlen(io_fs_create->nombre_archivo) + 1;

    return io_fs_create;
}

t_io_fs_delete* crear_io_fs_delete(t_pcb* pcb, char* nombre_interfaz, char* nombre_archivo) {
    // Crear y asignar memoria para la nueva estructura
    t_io_fs_delete* io_fs_delete = malloc(sizeof(t_io_fs_delete));
    if (io_fs_delete == NULL) {
        return NULL; // Manejo de error: no se pudo asignar memoria
    }

    // Asignar el PCB
    io_fs_delete->pcb = pcb;

    // Asignar y duplicar el nombre de la interfaz
    io_fs_delete->nombre_interfaz = strdup(nombre_interfaz);
    io_fs_delete->tamanio_nombre_interfaz = strlen(io_fs_delete->nombre_interfaz) + 1;

    // Asignar y duplicar el nombre del archivo
    io_fs_delete->nombre_archivo = strdup(nombre_archivo);
    io_fs_delete->tamanio_nombre_archivo = strlen(io_fs_delete->nombre_archivo) + 1;

    return io_fs_delete;
}

t_paquete* crear_paquete_io_fs_delete(uint32_t tamanio_nombre_interfaz, uint32_t tamanio_nombre_archivo) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    if (paquete == NULL) {
        return NULL; // Error de memoria
    }

    crear_buffer(paquete);
    paquete->codigo_operacion = IO_FS_DELETE;

    // Calcular el tamaño total del buffer basado en las longitudes predeterminadas y proporcionadas
    paquete->buffer->size = sizeof(uint32_t)  // PID
                            + sizeof(uint32_t)  // Quantum
                            + sizeof(uint32_t)  // PC
                            + 4 * sizeof(uint8_t)  // Registros normales
                            + 4 * sizeof(uint32_t)  // Registros extendidos
                            + 2 * sizeof(uint32_t)  // SI y DI
                            + sizeof(estado_proceso)  // Estado
                            + sizeof(uint32_t) + tamanio_nombre_interfaz  // Nombre de la interfaz y su tamaño
                            + sizeof(uint32_t) + tamanio_nombre_archivo;  // Nombre del archivo y su tamaño

    paquete->buffer->stream = malloc(paquete->buffer->size);
    if (paquete->buffer->stream == NULL) {
        free(paquete->buffer);
        free(paquete);
        return NULL; // Error de memoria
    }

    return paquete;
}

void agregar_io_fs_delete_a_paquete(t_paquete* paquete, t_io_fs_delete* io_fs_delete) {
    int desplazamiento = 0;

    // Copiar datos del PCB
    memcpy(paquete->buffer->stream + desplazamiento, &io_fs_delete->pcb->pid, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &io_fs_delete->pcb->quantum, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &io_fs_delete->pcb->cpu_registers->pc, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    // Copiar registros de CPU
    memcpy(paquete->buffer->stream + desplazamiento, io_fs_delete->pcb->cpu_registers->normales, 4 * sizeof(uint8_t));
    desplazamiento += 4 * sizeof(uint8_t);
    memcpy(paquete->buffer->stream + desplazamiento, io_fs_delete->pcb->cpu_registers->extendidos, 4 * sizeof(uint32_t));
    desplazamiento += 4 * sizeof(uint32_t);

    // SI y DI
    memcpy(paquete->buffer->stream + desplazamiento, &io_fs_delete->pcb->cpu_registers->si, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &io_fs_delete->pcb->cpu_registers->di, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    // Estado del proceso
    memcpy(paquete->buffer->stream + desplazamiento, &io_fs_delete->pcb->estado, sizeof(estado_proceso));
    desplazamiento += sizeof(estado_proceso);

    // Nombre de la interfaz y tamaño
    memcpy(paquete->buffer->stream + desplazamiento, &io_fs_delete->tamanio_nombre_interfaz, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, io_fs_delete->nombre_interfaz, io_fs_delete->tamanio_nombre_interfaz);
    desplazamiento += io_fs_delete->tamanio_nombre_interfaz;

    // Nombre del archivo y tamaño
    memcpy(paquete->buffer->stream + desplazamiento, &io_fs_delete->tamanio_nombre_archivo, sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, io_fs_delete->nombre_archivo, io_fs_delete->tamanio_nombre_archivo);
}

void enviar_io_fs_delete(int socket_cliente, t_io_fs_delete* io_fs_delete) {
    // Crear el paquete para la operación IO_FS_DELETE
    uint32_t tamanio_nombre_interfaz = strlen(io_fs_delete->nombre_interfaz) + 1;
    uint32_t tamanio_nombre_archivo = strlen(io_fs_delete->nombre_archivo) + 1;
    t_paquete* paquete = crear_paquete_io_fs_delete(tamanio_nombre_interfaz, tamanio_nombre_archivo);

    // Agregar los datos de la estructura t_io_fs_delete al paquete
    agregar_io_fs_delete_a_paquete(paquete, io_fs_delete);

    // Enviar el paquete al socket indicado
    enviar_paquete(paquete, socket_cliente);

    // Limpiar y liberar memoria utilizada por el paquete
    eliminar_paquete(paquete);
}

t_paquete* crear_paquete_io_fs_truncate(uint32_t tamanio_nombre_interfaz, uint32_t tamanio_nombre_archivo) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    if (paquete == NULL) {
        return NULL; // Error de memoria
    }

    crear_buffer(paquete);
    paquete->codigo_operacion = IO_FS_DELETE;

    // Calcular el tamaño total del buffer basado en las longitudes predeterminadas y proporcionadas
    paquete->buffer->size = sizeof(uint32_t)  // PID
                            + sizeof(uint32_t)  // Quantum
                            + sizeof(uint32_t)  // PC
                            + 4 * sizeof(uint8_t)  // Registros normales
                            + 4 * sizeof(uint32_t)  // Registros extendidos
                            + 2 * sizeof(uint32_t)  // SI y DI
                            + sizeof(estado_proceso)  // Estado
                            + sizeof(uint32_t) + tamanio_nombre_interfaz  // Nombre de la interfaz y su tamaño
                            + sizeof(uint32_t) + tamanio_nombre_archivo  // Nombre del archivo y su tamaño
							+ sizeof(uint32_t); //Nuevo tamanio del archivo

    paquete->buffer->stream = malloc(paquete->buffer->size);
    if (paquete->buffer->stream == NULL) {
        free(paquete->buffer);
        free(paquete);
        return NULL; // Error de memoria
    }

    return paquete;
}

void agregar_io_fs_truncate_a_paquete(t_paquete* paquete, t_io_fs_truncate* io_fs_truncate) {
    int desplazamiento = 0;

    // Copiar datos del PCB
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_truncate->pcb->pid), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_truncate->pcb->quantum), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_truncate->pcb->cpu_registers->pc), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    // Copiar registros normales (AX, BX, CX, DX)
    memcpy(paquete->buffer->stream + desplazamiento, io_fs_truncate->pcb->cpu_registers->normales, sizeof(uint8_t) * 4);
    desplazamiento += sizeof(uint8_t) * 4;

    // Copiar registros extendidos (EAX, EBX, ECX, EDX)
    memcpy(paquete->buffer->stream + desplazamiento, io_fs_truncate->pcb->cpu_registers->extendidos, sizeof(uint32_t) * 4);
    desplazamiento += sizeof(uint32_t) * 4;

    // Copiar registros si y di
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_truncate->pcb->cpu_registers->si), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_truncate->pcb->cpu_registers->di), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    // Copiar estado del proceso
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_truncate->pcb->estado), sizeof(estado_proceso));
    desplazamiento += sizeof(estado_proceso);

    // Copiar datos específicos de la solicitud
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_truncate->tamanio_nombre_interfaz), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, io_fs_truncate->nombre_interfaz, io_fs_truncate->tamanio_nombre_interfaz);
    desplazamiento += io_fs_truncate->tamanio_nombre_interfaz;

    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_truncate->tamanio_nombre_archivo), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, io_fs_truncate->nombre_archivo, io_fs_truncate->tamanio_nombre_archivo);
    desplazamiento += io_fs_truncate->tamanio_nombre_archivo;

    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_truncate->nuevo_tamanio), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
}

void enviar_io_fs_truncate(int socket_cliente, t_io_fs_truncate* io_fs_truncate) {
    // Obtener los tamaños de los nombres de interfaz y archivo
    uint32_t tamanio_nombre_interfaz = strlen(io_fs_truncate->nombre_interfaz) + 1;
    uint32_t tamanio_nombre_archivo = strlen(io_fs_truncate->nombre_archivo) + 1;

    // Crear el paquete utilizando los tamaños calculados
    t_paquete* paquete = crear_paquete_io_fs_truncate(tamanio_nombre_interfaz, tamanio_nombre_archivo);

    // Agregar la información de IO_FS_TRUNCATE al paquete
    agregar_io_fs_truncate_a_paquete(paquete, io_fs_truncate);

    // Enviar el paquete al cliente
    enviar_paquete(paquete, socket_cliente);

    // Liberar el paquete después del envío
    eliminar_paquete(paquete);
}

t_io_fs_write* crear_io_fs_write(t_pcb* pcb, char* nombre_interfaz, char* nombre_archivo, t_list *direcciones_fisicas, uint32_t tamanio, uint32_t puntero_archivo) {
    t_io_fs_write* io_fs_write = malloc(sizeof(t_io_fs_write));
    if (io_fs_write == NULL) {
        return NULL; // Manejo de error: no se pudo asignar memoria
    }

    // Asignar el PCB
    io_fs_write->pcb = pcb;
    // Asignar y duplicar el nombre de la interfaz
    io_fs_write->nombre_interfaz = strdup(nombre_interfaz);
    io_fs_write->tamanio_nombre_interfaz = strlen(io_fs_write->nombre_interfaz) + 1;
    // Asignar y duplicar el nombre del archivo
    io_fs_write->nombre_archivo = strdup(nombre_archivo);
    io_fs_write->tamanio_nombre_archivo = strlen(io_fs_write->nombre_archivo) + 1;
    // Asignar las direcciones físicas
    io_fs_write->direcciones_fisicas = direcciones_fisicas;
    // Asignar tamaño y puntero del archivo
    io_fs_write->tamanio = tamanio;
    io_fs_write->puntero_archivo = puntero_archivo;

    return io_fs_write;
}

t_paquete* crear_paquete_io_fs_write(t_io_fs_write* io_fs_write) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = IO_FS_WRITE;
    crear_buffer(paquete);

    // Calcular el tamaño total del paquete
    uint32_t direcciones_size = list_size(io_fs_write->direcciones_fisicas) * sizeof(uint32_t);
    paquete->buffer->size = sizeof(uint32_t) * (6 + list_size(io_fs_write->direcciones_fisicas)) + // Por cada dirección física
                            sizeof(uint32_t) * 2 + // PID y Quantum
                            sizeof(uint32_t) + // Tamaño y Puntero
                            sizeof(t_cpu_registers) + // Tamaño de los registros de CPU
                            + direcciones_size +
                            io_fs_write->tamanio_nombre_interfaz +
                            io_fs_write->tamanio_nombre_archivo;

    // Asignar memoria para el stream de datos del buffer
    paquete->buffer->stream = malloc(paquete->buffer->size);

    return paquete;
}

void agregar_io_fs_write_a_paquete(t_paquete* paquete, t_io_fs_write* io_fs_write) {
    int desplazamiento = 0;

    // Copiar datos del PCB
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_write->pcb->pid), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_write->pcb->quantum), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_write->pcb->cpu_registers->pc), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    // Copiar registros normales (AX, BX, CX, DX)
    memcpy(paquete->buffer->stream + desplazamiento, io_fs_write->pcb->cpu_registers->normales, sizeof(uint8_t) * 4);
    desplazamiento += sizeof(uint8_t) * 4;

    // Copiar registros extendidos (EAX, EBX, ECX, EDX)
    memcpy(paquete->buffer->stream + desplazamiento, io_fs_write->pcb->cpu_registers->extendidos, sizeof(uint32_t) * 4);
    desplazamiento += sizeof(uint32_t) * 4;


    // Copiar nombre de la interfaz y su tamaño
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_write->tamanio_nombre_interfaz), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, io_fs_write->nombre_interfaz, io_fs_write->tamanio_nombre_interfaz);
    desplazamiento += io_fs_write->tamanio_nombre_interfaz;

    // Copiar nombre del archivo y su tamaño
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_write->tamanio_nombre_archivo), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, io_fs_write->nombre_archivo, io_fs_write->tamanio_nombre_archivo);
    desplazamiento += io_fs_write->tamanio_nombre_archivo;

    // Copiar tamaño y puntero del archivo
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_write->tamanio), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_write->puntero_archivo), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    // Copiar direcciones físicas
    for (int i = 0; i < list_size(io_fs_write->direcciones_fisicas); i++) {
        uint32_t* direccion = list_get(io_fs_write->direcciones_fisicas, i);
        memcpy(paquete->buffer->stream + desplazamiento, direccion, sizeof(uint32_t));
        desplazamiento += sizeof(uint32_t);
    }
}

void enviar_io_fs_write(int socket_cliente, t_io_fs_write* io_fs_write) {
    t_paquete* paquete = crear_paquete_io_fs_write(io_fs_write);
    agregar_io_fs_write_a_paquete(paquete, io_fs_write);
    enviar_paquete(paquete, socket_cliente);
    eliminar_paquete(paquete);
}

t_io_fs_read* crear_io_fs_read(t_pcb* pcb, char* nombre_interfaz, char* nombre_archivo, t_list *direcciones_fisicas, uint32_t tamanio, uint32_t puntero_archivo) {
    t_io_fs_read* io_fs_read = malloc(sizeof(t_io_fs_read));
    if (io_fs_read == NULL) {
        return NULL; // Manejo de error: no se pudo asignar memoria
    }

    // Asignar el PCB
    io_fs_read->pcb = pcb;
    // Asignar y duplicar el nombre de la interfaz
    io_fs_read->nombre_interfaz = strdup(nombre_interfaz);
    io_fs_read->tamanio_nombre_interfaz = strlen(io_fs_read->nombre_interfaz) + 1;
    // Asignar y duplicar el nombre del archivo
    io_fs_read->nombre_archivo = strdup(nombre_archivo);
    io_fs_read->tamanio_nombre_archivo = strlen(io_fs_read->nombre_archivo) + 1;
    // Asignar las direcciones físicas
    io_fs_read->direcciones_fisicas = direcciones_fisicas;
    // Asignar tamaño y puntero del archivo
    io_fs_read->tamanio = tamanio;
    io_fs_read->puntero_archivo = puntero_archivo;

    return io_fs_read;
}

t_paquete* crear_paquete_io_fs_read(t_io_fs_read* io_fs_read) {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = IO_FS_READ;
    crear_buffer(paquete);

    // Calcular el tamaño total del paquete
    uint32_t direcciones_size = list_size(io_fs_read->direcciones_fisicas) * sizeof(uint32_t);
    paquete->buffer->size = sizeof(uint32_t) * (6 + list_size(io_fs_read->direcciones_fisicas)) + // Por cada dirección física
                            sizeof(uint32_t) * 2 + // PID y Quantum
                            sizeof(uint32_t) + // Tamaño y Puntero
                            sizeof(t_cpu_registers) + // Tamaño de los registros de CPU
                            io_fs_read->tamanio_nombre_interfaz +
                            io_fs_read->tamanio_nombre_archivo +
                            direcciones_size;

    // Asignar memoria para el stream de datos del buffer
    paquete->buffer->stream = malloc(paquete->buffer->size);

    return paquete;
}

void agregar_io_fs_read_a_paquete(t_paquete* paquete, t_io_fs_read* io_fs_read) {
    int desplazamiento = 0;

    // Copiar datos del PCB
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_read->pcb->pid), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_read->pcb->quantum), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_read->pcb->cpu_registers->pc), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    // Copiar registros normales (AX, BX, CX, DX)
    memcpy(paquete->buffer->stream + desplazamiento, io_fs_read->pcb->cpu_registers->normales, sizeof(uint8_t) * 4);
    desplazamiento += sizeof(uint8_t) * 4;

    // Copiar registros extendidos (EAX, EBX, ECX, EDX)
    memcpy(paquete->buffer->stream + desplazamiento, io_fs_read->pcb->cpu_registers->extendidos, sizeof(uint32_t) * 4);
    desplazamiento += sizeof(uint32_t) * 4;

    // Copiar nombre de la interfaz y su tamaño
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_read->tamanio_nombre_interfaz), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, io_fs_read->nombre_interfaz, io_fs_read->tamanio_nombre_interfaz);
    desplazamiento += io_fs_read->tamanio_nombre_interfaz;

    // Copiar nombre del archivo y su tamaño
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_read->tamanio_nombre_archivo), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, io_fs_read->nombre_archivo, io_fs_read->tamanio_nombre_archivo);
    desplazamiento += io_fs_read->tamanio_nombre_archivo;

    // Copiar tamaño y puntero del archivo
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_read->tamanio), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);
    memcpy(paquete->buffer->stream + desplazamiento, &(io_fs_read->puntero_archivo), sizeof(uint32_t));
    desplazamiento += sizeof(uint32_t);

    // Copiar direcciones físicas
    for (int i = 0; i < list_size(io_fs_read->direcciones_fisicas); i++) {
        uint32_t* direccion = list_get(io_fs_read->direcciones_fisicas, i);
        memcpy(paquete->buffer->stream + desplazamiento, direccion, sizeof(uint32_t));
        desplazamiento += sizeof(uint32_t);
    }
}

void enviar_io_fs_read(int socket_cliente, t_io_fs_read* io_fs_read) {
    t_paquete* paquete = crear_paquete_io_fs_read(io_fs_read);
    agregar_io_fs_read_a_paquete(paquete, io_fs_read);
    enviar_paquete(paquete, socket_cliente);
    eliminar_paquete(paquete);
}

t_io_fs_truncate* crear_io_fs_truncate(t_pcb* pcb, char* nombre_interfaz, char* nombre_archivo, uint32_t nuevo_tamanio) {
    // Crear y asignar memoria para la nueva estructura
    t_io_fs_truncate* io_fs_truncate = malloc(sizeof(t_io_fs_truncate));
    if (io_fs_truncate == NULL) {
        return NULL; // Manejo de error: no se pudo asignar memoria
    }
    // Asignar el PCB
    io_fs_truncate->pcb = pcb;
    // Asignar y duplicar el nombre de la interfaz
    io_fs_truncate->nombre_interfaz = strdup(nombre_interfaz);
    if (io_fs_truncate->nombre_interfaz == NULL) {
        free(io_fs_truncate);
        return NULL; // Manejo de error: no se pudo duplicar la cadena
    }
    io_fs_truncate->tamanio_nombre_interfaz = strlen(io_fs_truncate->nombre_interfaz) + 1;
    // Asignar y duplicar el nombre del archivo
    io_fs_truncate->nombre_archivo = strdup(nombre_archivo);
    if (io_fs_truncate->nombre_archivo == NULL) {
        free(io_fs_truncate->nombre_interfaz);
        free(io_fs_truncate);
        return NULL; // Manejo de error: no se pudo duplicar la cadena
    }
    io_fs_truncate->tamanio_nombre_archivo = strlen(io_fs_truncate->nombre_archivo) + 1;
    // Asignar el nuevo tamaño
    io_fs_truncate->nuevo_tamanio = nuevo_tamanio;

    return io_fs_truncate;
}

void enviar_fin_io_fs(int socket_cliente, t_pcb *pcb)
{
	t_paquete* paquete = crear_paquete_pcb(FIN_IO_FS);

	agregar_pcb_a_paquete(paquete, pcb);

	enviar_paquete(paquete, socket_cliente);
}
