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

char *path_base_dialfs;
int block_size;
int block_count;
// Inicializar archivos de bitmap
t_bitarray *bitarray;
FILE *bitmap_file;
t_bitarray *bitarray;
char bitmap_path[256];

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

        if (cod_op == DESCONEXION)
        {
            log_warning(logger, "Se desconectó el Kernel");
            while (1)
                ;
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
    conectar_memoria(); // Conectar con la memoria
    enviar_nombre_y_tipo(socket_kernel, nombre, STDIN);
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
        // Iterar sobre la lista de direcciones físicas
        for (int i = 0; i < list_size(io_stdin_read->direcciones_fisicas); i++)
        {
            int *direccion_fisica = list_get(io_stdin_read->direcciones_fisicas, i);

            // Enviar la solicitud de escribir en memoria para cada dirección física
            enviar_escribir_memoria(socket_memoria, io_stdin_read->pcb->pid, *direccion_fisica, io_stdin_read->tamanio_contenido, buffer_bytes);
        }
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

void crear_interfaz_stdout()
{
    conectar_memoria(); // Conectar con la memoria
    enviar_nombre_y_tipo(socket_kernel, nombre, STDOUT);
    while (true)
    {
        log_trace(logger, "Esperando pedido del Kernel");
        op_code cod_op = recibir_operacion(socket_kernel);
        log_trace(logger, "Llegó un pedido del Kernel");

        if (cod_op != IO_STDOUT_WRITE)
        {
            log_error(logger, "La interfaz esperaba recibir una operación IO_STDOUT_WRITE del Kernel pero recibió otra operación");
            continue; // Vuelve a esperar la próxima operación del Kernel
        }

        // Recibir la estructura t_io_stdout_write desde el kernel
        t_io_stdout_write *io_stdout_write = recibir_io_stdout_write(socket_kernel);

        // Leer desde memoria la información solicitada
        enviar_leer_memoria(socket_memoria, io_stdout_write->pcb->pid, io_stdout_write->direccion_logica, io_stdout_write->tamaño);

        // Recibir la respuesta de lectura desde memoria
        t_lectura *lectura = recibir_lectura(socket_memoria);
        op_code cod_lectura = recibir_operacion(socket_memoria);
        while (true)
        {
            if (cod_lectura != MEMORIA_LEIDA)
            {
                log_error(logger, "La interfaz esperaba recibir una lectura de memoria");
                continue; // Vuelve a esperar que le llegue lo que pidio de memoria
            }
            // Imprimir por pantalla la lectura obtenida
            printf("Contenido leído desde la dirección lógica %u:\n%s\n", io_stdout_write->direccion_logica, (char *)lectura->lectura);
            // Liberar memoria de la lectura obtenida
            free(lectura->lectura);
            free(lectura);
            break;
        }
        // Enviar confirmación de lectura completada al kernel
        enviar_fin_io_stdout_write(socket_kernel, io_stdout_write->pcb);
        // Liberar memoria de las estructuras utilizadas
        free(io_stdout_write->pcb);
        free(io_stdout_write);
    }
    liberar_conexion(socket_memoria); // Cerrar conexión con la memoria
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

    // construir la ruta completa del archivo bitmap.dat
    snprintf(bitmap_path, sizeof(bitmap_path), "%s/bitmap.dat", path_base_dialfs);
    bitmap_file = fopen(bitmap_path, "r+");
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
    crear_metadata_archivo("notas.txt", metadata);
    while (true)
    {
        log_trace(logger, "Esperando pedido del Kernel");
        op_code cod_op = recibir_operacion(socket_kernel);
        log_trace(logger, "Llegó un pedido del Kernel");

        switch (cod_op)
        {
        case IO_FS_CREATE:
            manejar_io_fs_create();
            break;
        case IO_FS_DELETE:
            manejar_io_fs_delete();
            break;
        case IO_FS_TRUNCATE:
            manejar_io_fs_truncate();
            break;
        case IO_FS_WRITE:
            manejar_io_fs_write();
            break;
        case IO_FS_READ:
            manejar_io_fs_read();
            break;
        default:
            break;
        }
    }
}

void crear_metadata_archivo(char *nombre_archivo, t_metadata_archivo metadata)
{
    char archivo_path[256];

    // Construir el path completo del archivo de configuración
    snprintf(archivo_path, sizeof(archivo_path), "%s/%s", path_base_dialfs, nombre_archivo);

    t_config *config_metadata = config_create(archivo_path);
    if (config_metadata == NULL)
    {
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

// Funciones para las instrucciones
void manejar_io_fs_create()
{
    // Recibir la estructura t_io_fs_create desde el Kernel
    t_io_fs_create *solicitud = recibir_io_fs_create(socket_kernel);

    if (solicitud == NULL)
    {
        log_error(logger, "Error al recibir la solicitud de creación de archivo");
        return;
    }

    // Extraer el PID y el nombre del archivo desde la solicitud
    uint32_t pid = solicitud->pcb->pid;
    char *nombre_archivo = solicitud->nombre_archivo;

    // Log de la acción
    log_info(logger, "PID: %d - Crear Archivo: %s", pid, nombre_archivo);

    // Buscar un bloque libre en el bitmap
    int bloque_inicial = -1;
    for (int i = 0; i < block_count; i++)
    {
        if (!bitarray_test_bit(bitarray, i))
        {
            bloque_inicial = i;
            bitarray_set_bit(bitarray, i);
            break;
        }
    }

    if (bloque_inicial == -1)
    {
        log_error(logger, "No hay bloques disponibles para crear el archivo %s", nombre_archivo);
        free(solicitud);
        return;
    }

    // Crear la metadata del archivo
    t_metadata_archivo metadata;
    metadata.bloque_inicial = bloque_inicial;
    metadata.tamanio_archivo = 0;

    crear_metadata_archivo(nombre_archivo, metadata);

    // Guardar cambios en el bitmap
    fseek(bitmap_file, 0, SEEK_SET);
    fwrite(bitarray->bitarray, sizeof(char), bitarray_get_max_bit(bitarray) / 8, bitmap_file);
    fflush(bitmap_file);

    // Enviar confirmacion a Kernel
    enviar_fin_io_fs(socket_kernel, solicitud->pcb);

    //  Liberar memoria
    free(solicitud->nombre_interfaz);
    free(solicitud->nombre_archivo);
    free(solicitud->pcb->cpu_registers);
    free(solicitud->pcb);
    free(solicitud);
}

void manejar_io_fs_delete(int socket_kernel)
{
    // Recibir la estructura t_io_fs_delete desde el kernel
    t_io_fs_delete *io_fs_delete = recibir_io_fs_delete(socket_kernel);
    if (io_fs_delete == NULL)
    {
        log_error(logger, "Error al recibir los datos de IO_FS_DELETE.");
        return;
    }

    char metadata_path[256];
    snprintf(metadata_path, sizeof(metadata_path), "%s/%s.metadata", path_base_dialfs, io_fs_delete->nombre_archivo);

    if (access(metadata_path, F_OK) != 0)
    {
        log_warning(logger, "PID: %d - Eliminar Archivo: %s - El archivo no existe.", io_fs_delete->pcb->pid, io_fs_delete->nombre_archivo);
        return;
    }

    // Leer el archivo de metadatos para obtener bloques utilizados
    t_config *metadata_config = config_create(metadata_path);
    if (metadata_config == NULL)
    {
        log_error(logger, "No se pudo abrir el archivo de metadatos para %s", io_fs_delete->nombre_archivo);
        return;
    }

    int bloque_inicial = config_get_int_value(metadata_config, "BLOQUE_INICIAL");
    int tamanio_archivo = config_get_int_value(metadata_config, "TAMANIO_ARCHIVO");
    int bloques_usados = (tamanio_archivo + block_size - 1) / block_size; // Calcular cantidad de bloques

    // Actualizar bitmap
    for (int i = 0; i < bloques_usados; i++)
    {
        bitarray_clean_bit(bitarray, bloque_inicial + i);
    }

    // Resetear los datos en el archivo de bloques
    fseek(bloques->archivo, bloque_inicial * block_size, SEEK_SET);
    char *empty_data = calloc(1, block_size); // Data buffer con ceros
    for (int i = 0; i < bloques_usados; i++)
    {
        fwrite(empty_data, block_size, 1, bloques->archivo); // Escribir ceros en cada bloque usado
    }
    fflush(bloques->archivo); // Asegurar que se escriba en disco
    free(empty_data);

    // Eliminar el archivo de metadatos
    if (remove(metadata_path) == 0)
    {
        log_info(logger, "PID: %d - Eliminar Archivo: %s", io_fs_delete->pcb->pid, io_fs_delete->nombre_archivo);
    }
    else
    {
        log_error(logger, "PID: %d - Eliminar Archivo: %s - Error al intentar eliminar el archivo.", io_fs_delete->pcb->pid, io_fs_delete->nombre_archivo);
    }

    config_destroy(metadata_config); // Liberar la configuración del metadata

    // Enviar confirmacion a Kernel
    enviar_fin_io_fs(socket_kernel, io_fs_delete->pcb);

    // Liberar estructura de solicitud después de su uso
    free(io_fs_delete->nombre_interfaz);
    free(io_fs_delete->nombre_archivo);
    free(io_fs_delete);
}

void manejar_io_fs_truncate()
{
    log_debug(logger, "Recibiendo pedido de IO_FS_TRUNCATE");

    // Recibir la solicitud de IO_FS_TRUNCATE
    t_io_fs_truncate *io_fs_truncate = recibir_io_fs_truncate(socket_entrada_salida);

    // Buscar si el archivo existe en el sistema de archivos DialFS
    char metadata_path[256];
    snprintf(metadata_path, sizeof(metadata_path), "%s/%s.metadata", path_base_dialfs, io_fs_truncate->nombre_archivo);

    t_config *metadata = config_create(metadata_path);
    if (metadata == NULL)
    {
        log_warning(logger, "Archivo %s no existe. No se puede truncar.", io_fs_truncate->nombre_archivo);
        enviar_error_truncate(io_fs_truncate->pcb); // Asumiendo que esta función maneja la respuesta de error
        return;
    }

    // Leer el bloque inicial y tamaño actual del archivo desde metadata
    int bloque_inicial = config_get_int_value(metadata, "BLOQUE_INICIAL");
    int tamanio_actual = config_get_int_value(metadata, "TAMANIO_ARCHIVO");

    // Calcular el nuevo tamaño y actualizar la metadata
    int nuevo_tamanio = io_fs_truncate->nuevo_tamanio; // asumimos que esta es la forma como se obtuvo el tamaño

    if (nuevo_tamanio != tamanio_actual)
    {
        config_set_value(metadata, "TAMANIO_ARCHIVO", string_itoa(nuevo_tamanio));
        config_save(metadata);
        log_info(logger, "Archivo %s truncado a %d bytes.", io_fs_truncate->nombre_archivo, nuevo_tamanio);

        // Actualizar el bitmap y archivo de bloques si necesario
        actualizar_estructura_bloques(bloque_inicial, tamanio_actual, nuevo_tamanio);
    }

    // TODO Asumiendo que hay alguna función que maneje la respuesta de éxito
    enviar_fin_io_fs(socket_kernel, io_fs_truncate->pcb);

    // Liberar recursos
    config_destroy(metadata);
    free(io_fs_truncate->nombre_interfaz);
    free(io_fs_truncate->nombre_archivo);
    free(io_fs_truncate->pcb->cpu_registers);
    free(io_fs_truncate->pcb);
    free(io_fs_truncate);
}

void manejar_io_fs_write()
{
    // Recibir la estructura t_io_fs_write desde el Kernel
    t_io_fs_write *io_fs_write = recibir_io_fs_write(socket_kernel);

    log_info(logger, "PID: %d - Escribir Archivo: %s - Tamaño a Escribir: %u - Puntero Archivo: %u",
             io_fs_write->pcb->pid, io_fs_write->nombre_archivo, io_fs_write->tamanio, io_fs_write->puntero_archivo);

    // Verificar que el archivo exista y pueda abrirse
    char archivo_path[1024];
    snprintf(archivo_path, sizeof(archivo_path), "%s/%s", path_base_dialfs, io_fs_write->nombre_archivo);
    FILE *archivo = fopen(archivo_path, "r+");
    if (!archivo)
    {
        log_error(logger, "No se pudo abrir el archivo %s para escritura", io_fs_write->nombre_archivo);
        enviar_error_al_kernel(io_fs_write->pcb, "Archivo no accesible");
        return;
    }

    fseek(archivo, io_fs_write->puntero_archivo, SEEK_SET);
    t_list_iterator *iterator = list_iterator_create(io_fs_write->direcciones_fisicas);
    while (list_iterator_has_next(iterator))
    {
        uint32_t *direccion_fisica = list_iterator_next(iterator);

        // Solicitar la lectura de memoria para cada dirección física
        enviar_leer_memoria(socket_memoria, io_fs_write->pcb->pid, *direccion_fisica, io_fs_write->tamanio);

        // Recibir los datos leídos de memoria
        t_lectura *lectura = recibir_lectura(socket_memoria);
        if (lectura && lectura->lectura)
        {
            fwrite(lectura->lectura, sizeof(char), lectura->tamanio_lectura, archivo);

            // Liberar recursos de lectura
            free(lectura->lectura);
            free(lectura);
        }
        else
        {
            log_error(logger, "Error al recibir datos de memoria para la dirección %u", *direccion_fisica);
            break;
        }
    }
    list_iterator_destroy(iterator);

    fflush(archivo);
    fclose(archivo);

    // Enviar confirmación de escritura completada al Kernel
    enviar_fin_io_fs(socket_kernel, io_fs_write->pcb);

    // Liberar recursos
    free(io_fs_write->nombre_interfaz);
    free(io_fs_write->nombre_archivo);
    free(io_fs_write->pcb->cpu_registers);
    free(io_fs_write->pcb);
    free(io_fs_write);

    log_trace(logger, "IO_FS_WRITE completado para el archivo %s", io_fs_write->nombre_archivo);
}

void manejar_io_fs_read()
{
    t_io_fs_read *solicitud = recibir_io_fs_read(socket_kernel);

    // Logear inicio de la operación con el formato específico
    log_info(logger, "PID: %d - Leer Archivo: %s - Tamaño a Leer: %u - Puntero Archivo: %u",
             solicitud->pcb->pid, solicitud->nombre_archivo, solicitud->tamanio, solicitud->puntero_archivo);

    // Abrir el archivo para lectura
    FILE *file = fopen(solicitud->nombre_archivo, "rb");
    if (file == NULL)
    {
        log_error(logger, "No se pudo abrir el archivo %s", solicitud->nombre_archivo);
        return;
    }

    // Posicionarse en el archivo según el puntero_archivo
    fseek(file, solicitud->puntero_archivo, SEEK_SET);
    char *buffer = malloc(solicitud->tamanio);
    size_t bytesRead = fread(buffer, 1, solicitud->tamanio, file);
    fclose(file);

    if (bytesRead != solicitud->tamanio)
    {
        log_warning(logger, "Se leyeron %zu bytes en lugar de %u", bytesRead, solicitud->tamanio);
    }

    // Escribir los datos en memoria según las direcciones físicas
    for (int i = 0; i < list_size(solicitud->direcciones_fisicas); i++)
    {
        uint32_t *direccion_fisica = list_get(solicitud->direcciones_fisicas, i);
        enviar_escribir_memoria(socket_memoria, solicitud->pcb->pid, *direccion_fisica, solicitud->tamanio, buffer);
    }

    free(buffer);
    log_info(logger, "Escritura en memoria completada para el PID: %d", solicitud->pcb->pid);

    //Envio a Kernel
    enviar_fin_io_fs(socket_kernel, solicitud->pcb);

    // Liberar recursos de la estructura recibida
    free(solicitud->nombre_interfaz);
    free(solicitud->nombre_archivo);
    list_destroy_and_destroy_elements(solicitud->direcciones_fisicas, free);
    free(solicitud->pcb->cpu_registers);
    free(solicitud->pcb);
    free(solicitud);
}

void actualizar_estructura_bloques(int bloque_inicial, int tamanio_actual, int nuevo_tamanio, int pid)
{
    int bloques_actuales = (tamanio_actual + block_size - 1) / block_size;
    int nuevos_bloques = (nuevo_tamanio + block_size - 1) / block_size;

    if (nuevos_bloques > bloques_actuales)
    {
        verificar_y_compactar_fs(bloque_inicial, bloques_actuales, nuevos_bloques, pid);
        for (int i = bloques_actuales; i < nuevos_bloques; i++)
        {
            if (!bitarray_test_bit(bitarray, bloque_inicial + i))
            {
                bitarray_set_bit(bitarray, bloque_inicial + i);
            }
        }
    }
    else if (nuevos_bloques < bloques_actuales)
    {
        for (int i = nuevos_bloques; i < bloques_actuales; i++)
        {
            bitarray_clean_bit(bitarray, bloque_inicial + i);
        }
    }

    if (nuevo_tamanio < tamanio_actual)
    {
        fseek(bloques->archivo, bloque_inicial * block_size + nuevo_tamanio, SEEK_SET);
        char *empty_data = calloc(block_size * (bloques_actuales - nuevos_bloques), 1);
        fwrite(empty_data, 1, block_size * (bloques_actuales - nuevos_bloques), bloques->archivo);
        free(empty_data);
    }

    fseek(bitmap_file, 0, SEEK_SET);
    fwrite(bitarray->bitarray, sizeof(char), bitarray->size, bitmap_file);
    fflush(bitmap_file);

    log_info(logger, "PID: %d - Estructura de bloques actualizada. Bloques usados ahora: %d", pid, nuevos_bloques);
}

void verificar_y_compactar_fs(int bloque_inicial, int bloques_actuales, int nuevos_bloques, int pid)
{
    bool espacio_contiguo = true;
    for (int i = bloques_actuales; i < nuevos_bloques; i++)
    {
        if (bitarray_test_bit(bitarray, bloque_inicial + i))
        {
            espacio_contiguo = false;
            break;
        }
    }

    if (!espacio_contiguo)
    {
        log_info(logger, "PID: %d - Espacio no contiguo detectado, iniciando compactación", pid);
        compactar_fs();
        log_info(logger, "PID: %d - Compactación completada", pid);
    }
}

void compactar_fs()
{
    log_info(logger, "Iniciando compactación del sistema de archivos");

    // Simulación: identificar todos los bloques ocupados y libres
    int total_blocks = block_count;
    bool *blocks_ocupados = calloc(total_blocks, sizeof(bool));

    // Marcar los bloques ocupados en una matriz temporal para facilitar la compactación
    for (int i = 0; i < total_blocks; i++)
    {
        blocks_ocupados[i] = bitarray_test_bit(bitarray, i);
    }

    int indice_escritura = 0; // Dónde escribir el próximo bloque compactado

    // Recorrer todos los bloques y mover los ocupados al inicio
    for (int i = 0; i < total_blocks; i++)
    {
        if (blocks_ocupados[i])
        {
            if (indice_escritura != i)
            {
                // Simular el movimiento de bloques en el archivo físico
                mover_bloque(bloques->archivo, i, indice_escritura);

                // Actualizar el bitmap
                bitarray_clean_bit(bitarray, i);
                bitarray_set_bit(bitarray, indice_escritura);

                indice_escritura++;
            }
            else
            {
                indice_escritura++; // El bloque ya está en la posición correcta
            }
        }
    }

    free(blocks_ocupados);

    // Escribir los cambios al archivo de bitmap
    fseek(bitmap_file, 0, SEEK_SET);
    fwrite(bitarray->bitarray, sizeof(char), bitarray->size, bitmap_file);
    fflush(bitmap_file);

    // Retrasar la compactación para simular tiempo de procesamiento
    sleep(retraso_compactacion);

    log_info(logger, "Compactación completada");
}

void mover_bloque(FILE *archivo, int bloque_origen, int bloque_destino)
{
    size_t tamano_bloque = block_size;
    char *buffer = malloc(tamano_bloque);

    // Leer el bloque origen
    fseek(archivo, bloque_origen * tamano_bloque, SEEK_SET);
    fread(buffer, tamano_bloque, 1, archivo);

    // Escribir en la posición destino
    fseek(archivo, bloque_destino * tamano_bloque, SEEK_SET);
    fwrite(buffer, tamano_bloque, 1, archivo);

    // Limpiar el bloque origen
    char *empty_data = calloc(tamano_bloque, 1);
    fseek(archivo, bloque_origen * tamano_bloque, SEEK_SET);
    fwrite(empty_data, tamano_bloque, 1, archivo);

    free(buffer);
    free(empty_data);
}
