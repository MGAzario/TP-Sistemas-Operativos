#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_cliente.h>
#include <utils/hello.h>

t_config *config;
t_log* logger;

int main(int argc, char* argv[])
{
    decir_hola("una Interfaz de Entrada/Salida");
    
    crear_logger();
    create_config();
    
    conectar_kernel();
    conectar_memoria();

    log_info(logger,"Terminó\n");
    
    return 0;
}

void crear_logger(){
    logger = log_create("./entradasalida.log","LOG_IO",true,LOG_LEVEL_TRACE);
    if(logger == NULL){
		perror("Ocurrió un error al leer el archivo de Log de Entrada/Salida");
		abort();
	}
}

void create_config(){
    config = config_create("./entradasalida.config");
    if (config == NULL)
    {
        log_error(logger,"Ocurrió un error al leer el archivo de configuración de Entrada/Salida\n");
        abort();
    }
}

void conectar_kernel(){
    // Establecer conexión con el módulo Kernel
    char *ip_kernel = config_get_string_value(config, "IP_KERNEL");
    char *puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    int socket_kernel = conectar_modulo(ip_kernel, puerto_kernel);
    if (socket_kernel != -1) {
        enviar_mensaje("Mensaje al kernel desde el Kernel", socket_kernel);
        liberar_conexion(socket_kernel);
    }
    recibir_mensaje(socket_kernel);
    liberar_conexion(socket_kernel);
}

void conectar_memoria(){
     // Establecer conexión con el módulo Memoria
    char *ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char *puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    int socket_memoria = conectar_modulo(ip_memoria, puerto_memoria);
    if (socket_memoria != -1) {
        enviar_mensaje("Mensaje a la Memoria desde el Kernel", socket_memoria);
        liberar_conexion(socket_memoria);
    }
    recibir_mensaje(socket_memoria);
    liberar_conexion(socket_memoria);
}

void interfazGenerica(char* nombre)
{
    /*Se obtienen los valores de configuración IP_KERNEL, PUERTO_KERNEL y TIEMPO_UNIDAD_TRABAJO desde un archivo de configuración, 
    almacenándolos en las variables ip_kernel, puerto_kernel y tiempoUnidadTrabajo, respectivamente.*/

    char *ip_kernel = config_get_string_value(config, "IP_KERNEL");
    char *puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    int tiempoUnidadTrabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");

    //Se establece una conexión con el kernel utilizando la IP y el puerto obtenidos anteriormente.

    int conexion = crearConexion(ip_kernel, puerto_kernel);

    /*Se envía el contenido de nombre al kernel a través de la conexión establecida. 
    La línea free(nombre); está comentada, pero indica que nombre debería liberarse cuando se utilicen parámetros dinámicamente asignados.*/

    enviarMensaje(nombre, conexion);
    //free(nombre);                 DESCOMENTAR CUANDO SE UTILICEN PARAMETROS
    
    /*Se entra en un bucle infinito donde se reciben mensajes del kernel.
    Cada mensaje recibido se almacena en la variable respuesta.
    respuesta se divide en componentes utilizando string_split con la coma como delimitador, almacenándose en el array decodificada.*/

    while(true)
    {
        /*Se verifica si el primer componente del mensaje (decodificada[0]) es igual a "IO_GEN_SLEEP".
        Si es así, se convierte el segundo componente (decodificada[1]) a un entero (multiplicador).
        Se duerme el tiempo correspondiente, calculado como multiplicador * 1000 * tiempoUnidadTrabajo microsegundos.
        Tras despertar, se envía un mensaje "FIN" al kernel.
        Finalmente, se libera la memoria asignada a respuesta.*/

        char* respuesta = recibirMensaje(conexion, logger);
        char** decodificada = string_split(respuesta, ',');

        if(strcmp(decodificada[0], "IO_GEN_SLEEP") == 0)
        {
            int multiplicador = atoi(decodificada[1]);
            usleep(multiplicador * 1000 * tiempoUnidadTrabajo);
            enviarMensaje("FIN", conexion);
        }
        free(respuesta);
    }
}
