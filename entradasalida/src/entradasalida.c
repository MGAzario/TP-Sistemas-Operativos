#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_cliente.h>
#include <utils/utils_server.h>
#include <utils/hello.h>

t_config *config;
t_log* logger;
char *ip_kernel;
char *puerto_kernel;
int socket_kernel;


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
    ip_kernel = config_get_string_value(config, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    socket_kernel = conectar_modulo(ip_kernel, puerto_kernel);
    if (socket_kernel != -1) {
        enviar_mensaje("Mensaje al kernel desde el Kernel", socket_kernel);
        liberar_conexion(socket_kernel);
    }
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

void crear_interfaz_generica(char* nombre)
{
    /*Las interfaces genéricas van a ser las más simples, 
    y lo único que van a hacer es que ante una petición van a esperar una cantidad de unidades de trabajo, 
    cuyo valor va a venir dado en la petición desde el Kernel.*/
    //La peticion se activa cuando se genera una peticion IO_SLEEP
    //El tiempo de unidad de trabajo se obtiene de las configuraciones
    int tiempo_unidad_trabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");

    
    //Tiene que estar siempre esperando un mensaje
    while(true)
    {
        /*Se verifica si el primer componente del mensaje (decodificada[0]) es igual a "IO_GEN_SLEEP".
        Si es así, se convierte el segundo componente (decodificada[1]) a un entero (multiplicador).
        Se duerme el tiempo correspondiente, calculado como multiplicador * 1000 * tiempoUnidadTrabajo microsegundos.
        Tras despertar, se envía un mensaje "FIN" al kernel.
        Finalmente, se libera la memoria asignada a respuesta.*/
        conectar_kernel();

        t_list peticion_kernel = list_create();
        peticion_kernel = recibir_paquete(socket_kernel);
        char* mensaje = list_take(peticion_kernel,1);

        if(strcmp(mensaje, "IO_GEN_SLEEP") == 0)
        {
            int multiplicador = atoi(list_take(peticion_kernel,2));
            usleep(multiplicador * 1000 * tiempo_unidad_trabajo);
            enviar_mensaje("FIN", conexion);
        }
        free(respuesta);

        liberar_conexion(socket_kernel);
    }
}
