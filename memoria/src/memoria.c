#include "memoria.h"

t_config *config;
t_log *logger;

char *path_instrucciones;

int socket_memoria;
int socket_kernel;
int socket_cpu;
int socket_entradasalida;

pthread_t hilo_kernel;
pthread_t hilo_cpu;

int tamanio_memoria;
int tamanio_pagina;

int marcos_memoria;

void *espacio_de_usuario;

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

    while(1)
    {
        // TODO: Recibir mensajes del módulo E/S
    }

    // Cerrar sockets
    // liberar_conexion(socket_entradasalida); 
    liberar_conexion(socket_kernel); 
    liberar_conexion(socket_cpu);
    liberar_conexion(socket_memoria);

    printf("Terminó\n");

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
    config = config_create("./memoria.config");
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

    espacio_de_usuario = malloc(tamanio_memoria);

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
    // socket_entradasalida = esperar_cliente(socket_memoria);
}