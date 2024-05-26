#include"entradasalida.h"

t_config *config;
t_log *logger;
char *ip_kernel;
char *puerto_kernel;
int socket_kernel;

int main(int argc, char *argv[])
{
    crear_logger();
    create_config();

    decir_hola("una Interfaz de Entrada/Salida");

    conectar_kernel();
    // conectar_memoria();
    crear_interfaz_generica("PruebaIO");

    log_info(logger, "Terminó\n");

    return 0;
}

void crear_logger()
{
    logger = log_create("./entradasalida.log", "LOG_IO", true, LOG_LEVEL_TRACE);
    if (logger == NULL)
    {
        perror("Ocurrió un error al leer el archivo de Log de Entrada/Salida");
        abort();
    }
}

void create_config()
{
    config = config_create("./entradasalida.config");
    if (config == NULL)
    {
        log_error(logger, "Ocurrió un error al leer el archivo de configuración de Entrada/Salida\n");
        abort();
    }
}

void conectar_kernel()
{
    // Establecer conexión con el módulo Kernel
    ip_kernel = config_get_string_value(config, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    socket_kernel = conectar_modulo(ip_kernel, puerto_kernel);
}

void conectar_memoria()
{
    // Establecer conexión con el módulo Memoria
    // char *ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    // char *puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    // int socket_memoria = conectar_modulo(ip_memoria, puerto_memoria);
}

void crear_interfaz_generica(char *nombre)
{
    /*Las interfaces genéricas van a ser las más simples,
    y lo único que van a hacer es que ante una petición van a esperar una cantidad de unidades de trabajo,
    cuyo valor va a venir dado en la petición desde el Kernel.*/
    // La peticion se activa cuando se genera una peticion IO_SLEEP
    // El tiempo de unidad de trabajo se obtiene de las configuraciones
    int tiempo_unidad_trabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");

    enviar_nombre_y_tipo(socket_kernel, nombre, GENERICA);
    // Tiene que estar siempre esperando un mensaje
    while (true)
    {
        /*Se verifica si el primer componente del mensaje (decodificada[0]) es igual a "IO_GEN_SLEEP".
        Si es así, se convierte el segundo componente (decodificada[1]) a un entero (multiplicador).
        Se duerme el tiempo correspondiente, calculado como multiplicador * 1000 * tiempoUnidadTrabajo microsegundos.
        Tras despertar, se envía un mensaje "FIN" al kernel.
        Finalmente, se libera la memoria asignada a respuesta.*/

        log_trace(logger, "Esperando pedido del Kernel");
        op_code cod_op = recibir_operacion(socket_kernel);
        log_trace(logger, "Llegó un pedido del Kernel");

        if (cod_op != IO_GEN_SLEEP)
        {
            log_error(logger, "La interfaz esperaba recibir una operación IO_GEN_SLEEP del Kernel pero recibió otra operación");
        }
        t_sleep *sleep  = recibir_sleep(socket_kernel);
        u_int32_t unidades_de_trabajo = sleep->unidades_de_trabajo;

        log_trace(logger, "El TdUdT es %i", tiempo_unidad_trabajo);
        log_trace(logger, "Las unidades de trabajo son %i", unidades_de_trabajo);
        log_trace(logger, "Empiezo a dormir");
        usleep(unidades_de_trabajo * tiempo_unidad_trabajo * 1000);
        log_trace(logger, "Termino de dormir");

        enviar_fin_sleep(socket_kernel, sleep->pcb);

        free(sleep->pcb->cpu_registers);
        free(sleep->pcb);
        free(sleep);
    }

    liberar_conexion(socket_kernel);
}
