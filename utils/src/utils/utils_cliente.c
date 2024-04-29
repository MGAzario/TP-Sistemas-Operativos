#include "utils_cliente.h"
#include "estados.h"
#include "registros.h"

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

void enviar_paquete(t_paquete* paquete, int socket_cliente)
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
t_paquete* crear_paquete_pcb() {
    t_paquete* paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = DISPATCH;
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

void enviar_paquete_pcb(t_paquete* paquete, int socket_cliente) {

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

void enviar_pcb(int socket_cliente, t_pcb *pcb)
{
    // Crear un paquete para enviar los PCBs
    t_paquete* paquete = crear_paquete_pcb();
    //Agrega el PCB a enviar
    agregar_pcb_a_paquete(paquete, pcb);
    //Lo envia a traves de la conexion
    enviar_paquete_pcb(paquete, socket_cliente);
}