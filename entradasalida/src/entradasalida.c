#include "entradasalida.h"

t_config *config;
t_log *logger;
char *ip_kernel;
char *puerto_kernel;
int socket_kernel;

char *nombre;
char *archivo_configuracion;

char *ip_memoria;
char *puerto_memoria;
int socket_memoria;

int main(int argc, char *argv[])
{
    // nombre = argv[1];
    // archivo_configuracion = argv[2];
    nombre = "nombre";
    archivo_configuracion = "./entradasalida.config";
    crear_logger();
    create_config();

    decir_hola("una Interfaz de Entrada/Salida");

    conectar_kernel();

    // conectar_memoria();
    crear_interfaz();

    conectar_memoria();
    // Prueba inicio
    enviar_mensaje_simple(socket_memoria, MENSAJE);
    sleep(2);
    enviar_mensaje_simple(socket_memoria, MENSAJE);
    // Prueba fin
    crear_interfaz_generica("PruebaIO");


    log_info(logger, "Terminó\n");
    liberar_conexion(socket_kernel);
    return 0;
}
// Funciones de configuracion y funcionamiento general
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
    config = config_create(archivo_configuracion);
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
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    socket_memoria = conectar_modulo(ip_memoria, puerto_memoria);
}

// Creacion de interfaces
void crear_interfaz()
{
    char *tipo_interfaz = config_get_string_value(config, "TIPO_INTERFAZ");
    if (strcmp(tipo_interfaz, "GENERICA") == 0)
    {
        crear_interfaz_generica();
    }
    else if (strcmp(tipo_interfaz, "STDIN") == 0)
    {
        crear_interfaz_stdin();
    }
    else if (strcmp(tipo_interfaz, "STDOUT") == 0)
    {
        crear_interfaz_stdout();
    }
    else if (strcmp(tipo_interfaz, "DialFS") == 0)
    {
        crear_interfaz_dialfs();
    }
    else
    {
        log_error(logger, "Error: TIPO_INTERFAZ '%s' no es válido.", tipo_interfaz);
    }
}

void crear_interfaz_generica()
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

        log_trace(logger, "Esperando pedido del Kernel");
        op_code cod_op = recibir_operacion(socket_kernel);
        log_trace(logger, "Llegó un pedido del Kernel");

        if(cod_op == DESCONEXION)
        {
            log_warning(logger, "Se desconectó el Kernel");
            while(1);
        }
        if (cod_op != IO_GEN_SLEEP)
        {
            log_error(logger, "La interfaz esperaba recibir una operación IO_GEN_SLEEP del Kernel pero recibió otra operación");
        }
        t_sleep *sleep = recibir_sleep(socket_kernel);
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
}

void crear_interfaz_stdin()
{
    int socket_memoria;
    conectar_memoria(); // Conectar con la memoria

    while (true)
    {
        log_trace(logger, "Esperando pedido del Kernel");
        op_code cod_op = recibir_operacion(socket_kernel);
        log_trace(logger, "Llegó un pedido del Kernel");

        if (cod_op != IO_STDIN_READ)
        {
            log_error(logger, "La interfaz esperaba recibir una operación IO_STDIN_READ del Kernel pero recibió otra operación");
            continue; // Vuelve a esperar la próxima operación del Kernel
        }

        // Recibir la estructura t_io_stdin_read desde el kernel
        t_io_stdin_read *io_stdin_read = recibir_io_stdin_read(socket_kernel);

        // Marcar la interfaz como ocupada
        interfaz_stdin_read->ocupada = true;

        // Leer texto desde el teclado solo si es IO_STDIN_READ
        char buffer[io_stdin_read->tamanio_contenido];
        printf("Ingrese el texto (máximo %d caracteres): ", io_stdin_read->tamanio_contenido - 1);
        fgets(buffer, io_stdin_read->tamanio_contenido, stdin);

        // Eliminar el salto de línea final generado por fgets
        buffer[strcspn(buffer, "\n")] = '\0';

        // Convertir el texto a un buffer de bytes para enviar a memoria
        void *buffer_bytes = malloc(io_stdin_read->tamanio_contenido);
        memcpy(buffer_bytes, buffer, io_stdin_read->tamanio_contenido);

        // Enviar datos a la memoria a partir de la dirección lógica especificada
        enviar_escribir_memoria(socket_memoria, io_stdin_read->pcb->pid, io_stdin_read->direcciones_fisicas, io_stdin_read->tamanio_contenido, buffer_bytes);

        // Liberar memoria del buffer de bytes
        free(buffer_bytes);

        // Enviar confirmación de lectura completada al kernel
        enviar_fin_io_stdin_read(socket_kernel, io_stdin_read->pcb);

        // Liberar memoria de las estructuras utilizadas
        free(io_stdin_read->nombre_interfaz);
        list_destroy_and_destroy_elements(io_stdin_read->direcciones_fisicas, free);
        free(io_stdin_read->pcb->cpu_registers);
        free(io_stdin_read->pcb);
        free(io_stdin_read);
    }

    liberar_conexion(socket_memoria); // Cerrar conexión con la memoria
}

void crear_interfaz_stdout() {
    int socket_memoria;
    conectar_memoria(&socket_memoria);

    enviar_nombre_y_tipo(socket_kernel, nombre, STDOUT);
    while (true) {
        log_trace(logger, "Esperando pedido del Kernel");
        op_code cod_op = recibir_operacion(socket_kernel);
        log_trace(logger, "Llegó un pedido del Kernel");

        if (cod_op != IO_STDOUT_WRITE) {
            log_error(logger, "La interfaz esperaba recibir una operación IO_STDOUT_WRITE del Kernel pero recibió otra operación");
            continue;
        }

        // Recibir la solicitud de escritura de STDOUT
        t_io_std *io_write_stdout = recibir_io_std(socket_kernel);

        //TODO: Leer el valor desde la memoria}
        /*
        char *valor = malloc(io_write_stdout->tamanio);
        recibir_datos_memoria(socket_memoria, io_write_stdout->direccion_fisica, valor, io_write_stdout->tamanio);

        // Mostrar el valor por STDOUT
        printf("Valor leído desde la dirección física %u: %s\n", io_write_stdout->direccion_fisica, valor);

        // Liberar la memoria del valor
        free(valor);
        */
        // Liberar la memoria de la estructura de solicitud de escritura de STDOUT
        free(io_write_stdout->pcb->cpu_registers);
        free(io_write_stdout->pcb);
        free(io_write_stdout);
    }

    liberar_conexion(socket_memoria);
}

void crear_interfaz_dialfs()
{

}