#include "memoria.h"

t_config *config;
t_log *logger;

char *path_instrucciones;

int socket_memoria;
int socket_kernel;
int socket_cpu;
int socket_entradasalida;

pthread_t hilo_kernel;

t_list *lista_instrucciones_por_proceso;

int main(int argc, char* argv[])
{
    crear_logger();
    crear_config();

    decir_hola("Memoria");

    path_instrucciones = config_get_string_value(config, "PATH_INSTRUCCIONES");

    lista_instrucciones_por_proceso = list_create();

    // Crear socket memoria, tomando el puerto de un archivo.
    // char *puerto_memoria= config_get_string_value(config, "PUERTO_ESCUCHA");
    // int socket_memoria = iniciar_servidor(puerto_memoria);

    // Espero a los clientes.
    // Esto es repetitivo, y no tiene mucho sentido tampoco. Tendriamos que tener 1 socket solo que diferencie desde donde le estan hablando.
    // recibir_kernel();
    // recibir_cpu(socket_memoria);
    // recibir_entradasalida(socket_memoria);

    iniciar_servidor_memoria();

    //Creo un hilo para el planificador de corto plazo
    if (pthread_create(&hilo_kernel, NULL, recibir_kernel, NULL) != 0){
        log_error(logger, "Error al inicializar el Hilo Planificador de Corto Plazo");
        exit(EXIT_FAILURE);
    }
    pthread_detach(hilo_kernel);

    while(1)
    {

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

void iniciar_servidor_memoria() {
    char *puerto_memoria= config_get_string_value(config, "PUERTO_ESCUCHA");
    socket_memoria = iniciar_servidor(puerto_memoria);
    socket_cpu = esperar_cliente(socket_memoria);
    socket_kernel = esperar_cliente(socket_memoria);
    // socket_entradasalida = esperar_cliente(socket_memoria);
}

// void recibir_cpu(int socket_memoria){
//     int socket_cpu = esperar_cliente(socket_memoria);
//     if (socket_cpu == -1) {
//         log_info(logger,"Error al aceptar la conexión del kernel.\n");
//         liberar_conexion(socket_memoria);
//     }
//     recibir_mensaje(socket_cpu);
//     liberar_conexion(socket_cpu);
// }

// void recibir_entradasalida(int socket_memoria){
//     int socket_entradasalida = esperar_cliente(socket_memoria);
//     if (socket_entradasalida == -1) {
//         log_info(logger,"Error al aceptar la conexión del kernel.\n");
//         liberar_conexion(socket_memoria);
//     }
//     recibir_mensaje(socket_entradasalida);
//     liberar_conexion(socket_entradasalida);
// }

/*Esta funcion gestiona los accesos a memoria. 
El tema de la concurrencia a la memoria es una duda que todavia tengo. ¿Que pasa si 2 modulos distintos quieren entrar al mismo tiempo?*/
// void manejar_conexion_entrante(int socket_memoria) {
//     // Esperar una conexión entrante
//     int socket_cliente = esperar_cliente(socket_memoria);
//     if (socket_cliente == -1) {
//         log_info(logger, "Error al aceptar la conexión entrante.\n");
//         return;
//     }

//     // Recibir el primer mensaje para determinar el tipo de módulo
//     t_paquete* paquete = recibir_paquete(socket_cliente);
//     if (paquete == NULL) {
//         log_info(logger, "Error al recibir el mensaje del cliente.\n");
//         liberar_conexion(socket_cliente);
//         return;
//     }

//     // Verificar el tipo de operación del paquete recibido
//     switch (paquete->codigo_operacion) {
//         case KERNEL_CREACION_PROCESO:
//             log_info(logger, "Conexión recibida del Kernel.\n");
//             // Realizar operaciones correspondientes al Kernel
//             // ...
//             break;
//         case CPU_SOLICITAR_INSTRUCCION:
//             log_info(logger, "Conexión recibida del CPU.\n");
//             // Realizar operaciones correspondientes al CPU
//             // ...
//             break;
//         case MENSAJE_ENTRADA_SALIDA:
//             log_info(logger, "Conexión recibida de Entrada/Salida.\n");
//             // Realizar operaciones correspondientes a Entrada/Salida
//             // ...
//             break;
//         default:
//             log_info(logger, "Conexión recibida de un módulo desconocido.\n");
//             break;
//     }

//     // Liberar recursos y cerrar la conexión
//     eliminar_paquete(paquete);
//     liberar_conexion(socket_cliente);
// }