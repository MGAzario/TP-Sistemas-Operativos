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
    //nombre = argv[1];
    //archivo_configuracion = argv[2];
    nombre = "SLP1";
    archivo_configuracion = "./entradasalida.config";
    crear_logger();
    create_config();

    decir_hola("una Interfaz de Entrada/Salida");

    conectar_kernel();

    crear_interfaz();

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
        log_trace(logger, "Interfaz genérica");
        crear_interfaz_generica();
    }
    else if (strcmp(tipo_interfaz, "STDIN") == 0)
    {
        log_trace(logger, "Interfaz STDIN");
        crear_interfaz_stdin();
    }
    else if (strcmp(tipo_interfaz, "STDOUT") == 0)
    {
        log_trace(logger, "Interfaz STDOUT");
        crear_interfaz_stdout();
    }
    else if (strcmp(tipo_interfaz, "DialFS") == 0)
    {
        log_trace(logger, "Interfaz DialFS");
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
            continue; // Vuelve a esperar la próxima operación del Kernel
        }
        
        t_sleep *sleep = recibir_sleep(socket_kernel);
        log_info(logger, "PID: %i - Operacion: SLEEP", sleep->pcb->pid);
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
    conectar_memoria(); // Conectar con la memoria
    // Prueba inicio
    enviar_mensaje_simple(socket_memoria, MENSAJE);
    // Prueba fin

    enviar_nombre_y_tipo(socket_kernel, nombre, STDIN);
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
        if (cod_op != IO_STDIN_READ)
        {
            log_error(logger, "La interfaz esperaba recibir una operación IO_STDIN_READ del Kernel pero recibió otra operación");
            continue; // Vuelve a esperar la próxima operación del Kernel
        }

        // Recibir la estructura t_io_std desde el kernel
        t_io_std *io_stdin_read = recibir_io_std(socket_kernel);
        log_info(logger, "PID: %i - Operacion: STDIN_READ", io_stdin_read->pcb->pid);

        // Leer texto desde el teclado solo si es IO_STDIN_READ
        char *linea;
        printf("Ingrese el texto (máximo %d caracteres): ", io_stdin_read->tamanio_contenido);
        linea = readline("> ");
        log_trace(logger, "Tamaño del texto ingresado: %i", string_length(linea));

        int indice = 0;
        // Enviar datos a la memoria a partir de la dirección lógica especificada
        // Iterar sobre la lista de direcciones físicas
        for (int i = 0; i < list_size(io_stdin_read->direcciones_fisicas); i++)
        {
            t_direccion_y_tamanio *direccion_fisica = list_get(io_stdin_read->direcciones_fisicas, i);
            char *texto = string_substring(linea, indice, direccion_fisica->tamanio);
            enviar_escribir_memoria(socket_memoria, io_stdin_read->pcb->pid, direccion_fisica->direccion, direccion_fisica->tamanio, texto);
            indice += direccion_fisica->tamanio;
            op_code cod_op = recibir_operacion(socket_memoria);
            if (cod_op != MEMORIA_ESCRITA)
            {
                log_error(logger, "La interfaz esperaba recibir una operación MEMORIA_ESCRITA de la Memoria pero recibió otra operación");
            }
            recibir_ok(socket_memoria);
        }

        // Enviar confirmación de lectura completada al kernel
        enviar_fin_io_read(socket_kernel, io_stdin_read->pcb);

        // Liberar memoria de las estructuras utilizadas
        free(io_stdin_read->nombre_interfaz);
        list_destroy_and_destroy_elements(io_stdin_read->direcciones_fisicas, destruir_direccion);
        free(io_stdin_read->pcb->cpu_registers);
        free(io_stdin_read->pcb);
        free(io_stdin_read);
    }
}

void crear_interfaz_stdout() {
    conectar_memoria(); // Conectar con la memoria
    // Prueba inicio
    enviar_mensaje_simple(socket_memoria, MENSAJE);
    // Prueba fin
    
    enviar_nombre_y_tipo(socket_kernel, nombre, STDOUT);
    while (true) {
        log_trace(logger, "Esperando pedido del Kernel");
        op_code cod_op = recibir_operacion(socket_kernel);
        log_trace(logger, "Llegó un pedido del Kernel");

        if(cod_op == DESCONEXION)
        {
            log_warning(logger, "Se desconectó el Kernel");
            while(1);
        }
        if (cod_op != IO_STDOUT_WRITE) {
            log_error(logger, "La interfaz esperaba recibir una operación IO_STDOUT_WRITE del Kernel pero recibió otra operación");
            continue; // Vuelve a esperar la próxima operación del Kernel
        }

        // Recibir la estructura t_io_std desde el kernel
        t_io_std *io_stdout_write = recibir_io_std(socket_kernel);
        log_info(logger, "PID: %i - Operacion: STDOUT_WRITE", io_stdout_write->pcb->pid);

        // Leer desde memoria la información solicitada
        char *texto = string_new();

        for (int i = 0; i < list_size(io_stdout_write->direcciones_fisicas); i++)
        {
            t_direccion_y_tamanio *direccion_fisica = list_get(io_stdout_write->direcciones_fisicas, i);
            enviar_leer_memoria(socket_memoria, io_stdout_write->pcb->pid, direccion_fisica->direccion, direccion_fisica->tamanio);
            op_code cod_op = recibir_operacion(socket_memoria);
            if (cod_op != MEMORIA_LEIDA)
            {
                log_error(logger, "La interfaz esperaba recibir una operación MEMORIA_LEIDA de la Memoria pero recibió otra operación");
            }
            t_lectura *leido = recibir_lectura(socket_memoria);
            string_n_append_con_strnlen(&texto, leido->lectura, leido->tamanio_lectura);
        }

        printf("%s\n", texto);

        // Enviar confirmación de escritura completada al kernel
        enviar_fin_io_write(socket_kernel, io_stdout_write->pcb);

        // Liberar memoria de las estructuras utilizadas
        free(io_stdout_write->nombre_interfaz);
        list_destroy_and_destroy_elements(io_stdout_write->direcciones_fisicas, destruir_direccion);
        free(io_stdout_write->pcb->cpu_registers);
        free(io_stdout_write->pcb);
        free(io_stdout_write);
    }
}

void crear_interfaz_dialfs()
{

    int tiempo_unidad_trabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");
    char *path_base_dialfs = config_get_string_value(config, "PATH_BASE_DIALFS");
    int block_size = config_get_int_value(config, "BLOCK_SIZE");
    int block_count = config_get_int_value(config, "BLOCK_COUNT");
    int retraso_compactacion = config_get_int_value(config, "RETRASO_COMPACTACION");
    enviar_nombre_y_tipo(socket_kernel, nombre, DialFS);
    conectar_memoria();

    // Inicializar archivos de bloques
    t_fs_bloques *bloques = malloc(sizeof(t_fs_bloques));
    bloques->block_size = block_size;
    bloques->block_count = block_count;

    char bloques_path[256];
    // construir la ruta completa del archivo bloques.dat
    snprintf(bloques_path, sizeof(bloques_path), "%s/bloques.dat", path_base_dialfs);
    bloques->archivo = fopen(bloques_path, "r+");

    // Si el archivo no existe, crear y establecer el tamaño adecuado
    if (bloques->archivo == NULL)
    {
        bloques->archivo = fopen(bloques_path, "w+");
        if (bloques->archivo == NULL)
        {
            log_error(logger, "Error al crear bloques.dat");
            exit(EXIT_FAILURE);
        }
        ftruncate(fileno(bloques->archivo), block_size * block_count);
    }
    else
    {
        fseek(bloques->archivo, 0, SEEK_END);
        long file_size = ftell(bloques->archivo);
        if (file_size != block_size * block_count)
        {
            ftruncate(fileno(bloques->archivo), block_size * block_count);
        }
        rewind(bloques->archivo);
    }

    // Inicializar archivos de bitmap
    t_bitarray *bitarray;
    char bitmap_path[256];
    // construir la ruta completa del archivo bitmap.dat
    snprintf(bitmap_path, sizeof(bitmap_path), "%s/bitmap.dat", path_base_dialfs);
    FILE *bitmap_file = fopen(bitmap_path, "r+");
    if (bitmap_file == NULL)
    {
        bitmap_file = fopen(bitmap_path, "w+");
        if (bitmap_file == NULL)
        {
            log_error(logger, "Error al crear bitmap.dat");
            exit(EXIT_FAILURE);
        }
        char *bitmap_data = calloc(block_count, sizeof(char));
        fwrite(bitmap_data, sizeof(char), block_count, bitmap_file);
        free(bitmap_data);
    }

    fseek(bitmap_file, 0, SEEK_SET);
    char *bitmap_data = malloc(block_count);
    fread(bitmap_data, sizeof(char), block_count, bitmap_file);
    bitarray = bitarray_create_with_mode(bitmap_data, block_count, LSB_FIRST);

    // Ejemplo de creación de archivo de metadata
    t_metadata_archivo metadata = {25, 1024};
    crear_metadata_archivo("notas.txt", metadata, path_base_dialfs);
    while (true) {
        log_trace(logger, "Esperando pedido del Kernel");
        op_code cod_op = recibir_operacion(socket_kernel);
        log_trace(logger, "Llegó un pedido del Kernel");

        switch (cod_op)
        {
        case IO_FS_CREATE:
            manejar_io_fs_create();
            break;
        default:
            break;
        }
    }
}

void crear_metadata_archivo(char* nombre_archivo, t_metadata_archivo metadata, char* path_base_dialfs) {
    char archivo_path[256];

    // Construir el path completo del archivo de configuración
    snprintf(archivo_path, sizeof(archivo_path), "%s/%s", path_base_dialfs, nombre_archivo);

    t_config* config_metadata = config_create(archivo_path);
    if (config_metadata == NULL) {
        printf("Error: No se pudo crear el archivo de metadata para %s\n", nombre_archivo);
        return;
    }

    // Convertir valores a string
    char bloque_inicial_str[20];
    char tamanio_archivo_str[20];
    sprintf(bloque_inicial_str, "%d", metadata.bloque_inicial);
    sprintf(tamanio_archivo_str, "%d", metadata.tamanio_archivo);

    // Setear valores en el archivo de configuración
    config_set_value(config_metadata, "BLOQUE_INICIAL", bloque_inicial_str);
    config_set_value(config_metadata, "TAMANIO_ARCHIVO", tamanio_archivo_str);

    // Guardar cambios y destruir la estructura de configuración
    config_save(config_metadata);
    config_destroy(config_metadata);
}

void manejar_io_fs_create(){
    
}