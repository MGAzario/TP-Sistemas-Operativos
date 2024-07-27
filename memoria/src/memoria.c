#include "memoria.h"

t_config *config;
t_log *logger;

char *path_instrucciones;

int socket_memoria;
int socket_kernel;
int socket_cpu;
int socket_entradasalida;

pthread_t hilo_cpu;
pthread_t hilo_kernel;
pthread_t hilo_entradasalida[100];
int numero_de_entradasalida;

int tamanio_memoria;
int tamanio_pagina;

int marcos_memoria;

int retardo;

void *espacio_de_usuario;
sem_t mutex_espacio_de_usuario;

t_bitarray *marcos_libres;

t_list *lista_tablas_de_paginas;

t_list *lista_instrucciones_por_proceso;

int main(int argc, char* argv[])
{
    crear_logger();
    crear_config();

    decir_hola("Memoria");

    inicializar_variables_globales();

    iniciar_servidor_memoria();

    // Creo un hilo para recibir mensajes del Kernel
    if (pthread_create(&hilo_kernel, NULL, recibir_kernel, NULL) != 0){
        log_error(logger, "Error al inicializar el Hilo del Kernel");
        exit(EXIT_FAILURE);
    }
    pthread_detach(hilo_kernel);

    // Creo un hilo para recibir mensajes del CPU
    if (pthread_create(&hilo_cpu, NULL, recibir_cpu, NULL) != 0){
        log_error(logger, "Error al inicializar el Hilo del CPU");
        exit(EXIT_FAILURE);
    }
    pthread_detach(hilo_cpu);

    // Recibo mensajes de interfaces
    while(1)
    {
        int socket_entradasalida = esperar_cliente(socket_memoria);
        log_trace(logger, "Se conectó una interfaz con socket %i", socket_entradasalida);

        int *socket = &socket_entradasalida;

        pthread_create(&hilo_entradasalida[numero_de_entradasalida], NULL, interfaz, socket);
        pthread_detach(hilo_entradasalida[numero_de_entradasalida]);
        numero_de_entradasalida++;
    }

    liberar_conexion(socket_kernel); 
    liberar_conexion(socket_cpu);
    liberar_conexion(socket_memoria);

    return 0;
}

void crear_logger(){
    logger = log_create("./memoria.log","LOG_MEMORIA",true,LOG_LEVEL_TRACE);
    if(logger == NULL){
		perror("Ocurrió un error al leer el archivo de Log de Memoria");
		abort();
	}
}

void crear_config(){
    config = config_create("./memoria_deadlock.config");
    if (config == NULL)
    {
        log_error(logger,"Ocurrió un error al leer el archivo de configuración\n");
        abort();
    }
}

void inicializar_variables_globales()
{
    path_instrucciones = config_get_string_value(config, "PATH_INSTRUCCIONES");
    tamanio_memoria = config_get_int_value(config, "TAM_MEMORIA");
    tamanio_pagina = config_get_int_value(config, "TAM_PAGINA");
    retardo = config_get_int_value(config, "RETARDO_RESPUESTA");

    numero_de_entradasalida = 0;

    espacio_de_usuario = malloc(tamanio_memoria);
    sem_init(&mutex_espacio_de_usuario, 0, 1);

    marcos_memoria = tamanio_memoria / tamanio_pagina;

    // para crear un bitarray hace falta pasarle el tamaño en bytes (8 bits),
    // así que por las dudas verificamos que la cantidad de marcos sea un múltiplo de 8
    if (marcos_memoria % 8 != 0) {
        marcos_memoria = (int) ceil((float) marcos_memoria / (float) 8) * 8;
        log_debug(logger, "El tamaño de la memoria es %i y el tamaño de la página es %i. Por lo tanto, la memoria tiene %i marcos (redondeado)",
        tamanio_memoria, tamanio_pagina, marcos_memoria);
    }
    else
    {
        log_debug(logger, "El tamaño de la memoria es %i y el tamaño de la página es %i. Por lo tanto, la memoria tiene %i marcos",
        tamanio_memoria, tamanio_pagina, marcos_memoria);
    }
    void *bitarray_memoria = malloc(marcos_memoria / 8);
    marcos_libres = bitarray_create_with_mode(bitarray_memoria, marcos_memoria / 8, LSB_FIRST);
    for (int i = 0; i < bitarray_get_max_bit(marcos_libres); i++)
    {
        bitarray_clean_bit(marcos_libres, i);
    }
    estado_del_bitarray();

    lista_tablas_de_paginas = list_create();
    lista_instrucciones_por_proceso = list_create();
}

void iniciar_servidor_memoria() {
    char *puerto_memoria= config_get_string_value(config, "PUERTO_ESCUCHA");
    socket_memoria = iniciar_servidor(puerto_memoria);
    socket_cpu = esperar_cliente(socket_memoria);
    socket_kernel = esperar_cliente(socket_memoria);
}

void *interfaz(void *socket)
{
    int socket_interfaz = *(int *) socket;
    bool sigue_conectado = true;
    while(sigue_conectado)
    {
        op_code cod_op = recibir_operacion(socket_interfaz);

        switch (cod_op)
        {
            case MENSAJE:
                recibir_ok(socket_interfaz);
                log_debug(logger, "Mensaje recibido exitosamente de la interfaz de socket %i", socket_interfaz);
                break;
            case LEER_MEMORIA:
                log_trace(logger, "Recibí una solicitud de una interfaz para leer de Memoria");
                usleep(retardo * 1000);
                t_leer_memoria *leer_memoria = recibir_leer_memoria(socket_interfaz);
                void *lectura = leer(leer_memoria->direccion, leer_memoria->tamanio);
                log_info(logger, "PID: %i - Accion: LEER - Direccion fisica: %i - Tamaño: %i",
                    leer_memoria->pid,
                    leer_memoria->direccion,
                    leer_memoria->tamanio);
                enviar_lectura(socket_interfaz, lectura, leer_memoria->tamanio);
                break;
            case ESCRIBIR_MEMORIA:
                log_trace(logger, "Recibí una solicitud de una interfaz para escribir en Memoria");
                usleep(retardo * 1000);
                t_escribir_memoria *escribir_memoria = recibir_escribir_memoria(socket_interfaz);
                escribir(escribir_memoria->direccion, escribir_memoria->tamanio, escribir_memoria->valor);
                log_info(logger, "PID: %i - Accion: ESCRIBIR - Direccion fisica: %i - Tamaño: %i",
                    escribir_memoria->pid,
                    escribir_memoria->direccion,
                    escribir_memoria->tamanio);
                enviar_mensaje_simple(socket_interfaz, MEMORIA_ESCRITA);
                break;
            case DESCONEXION:
                log_warning(logger, "Se desconectó una interfaz");
                sigue_conectado = false;
                break;
            default:
                log_warning(logger, "Mensaje desconocido de una interfaz: %i", cod_op);
                while(1);
                break;
        }
    }
    return NULL;
}

void *leer(int direccion_fisica, int tamanio)
{
    void *valor = malloc(tamanio);
    sem_wait(&mutex_espacio_de_usuario);
    memcpy(valor, espacio_de_usuario + direccion_fisica, tamanio);
    sem_post(&mutex_espacio_de_usuario);
    return valor;
}

void escribir(int direccion_fisica, int tamanio, void *valor)
{
    sem_wait(&mutex_espacio_de_usuario);
    memcpy(espacio_de_usuario + direccion_fisica, valor, tamanio);
    sem_post(&mutex_espacio_de_usuario);
}