#include "entradasalida.h"

t_config *config;
t_log *logger;
t_log_level nivel_del_log;
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
int tamanio_del_archivo_de_bloques;
int tamanio_del_archivo_del_bitmap;
t_list *lista_archivos_por_bloque_inicial;
int tiempo_unidad_trabajo;
int retraso_compactacion;
void *archivo_de_bloques;
void *archivo_del_bitmap;
t_bitarray *bitmap_de_bloques_libres;

int main(int argc, char *argv[])
{
    nombre = argv[1];
    archivo_configuracion = argv[2];
    // nombre = "IO_GEN_SLEEP";
    // archivo_configuracion = "./IO_GEN_SLEEP.config";
    if (argc > 3)
    {
        nivel_del_log = LOG_LEVEL_TRACE;
    }
    else
    {
        nivel_del_log = LOG_LEVEL_INFO;
    }

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
    logger = log_create("./entradasalida.log", "LOG_IO", true, nivel_del_log);
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
    else if (strcmp(tipo_interfaz, "DIALFS") == 0)
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

        free(sleep->nombre_interfaz);
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

        // Leer texto desde el teclado
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
            free(texto);
        }

        // Enviar confirmación de lectura completada al kernel
        enviar_fin_io_read(socket_kernel, io_stdin_read->pcb);

        // Liberar memoria de las estructuras utilizadas
        free(io_stdin_read->nombre_interfaz);
        list_destroy_and_destroy_elements(io_stdin_read->direcciones_fisicas, destruir_direccion);
        free(io_stdin_read->pcb->cpu_registers);
        free(io_stdin_read->pcb);
        free(io_stdin_read);
        free(linea);
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
        void *texto = malloc(io_stdout_write->tamanio_contenido + 1);

        int desplazamiento = 0;
        for (int i = 0; i < list_size(io_stdout_write->direcciones_fisicas); i++)
        {
            t_direccion_y_tamanio *direccion_fisica = list_get(io_stdout_write->direcciones_fisicas, i);
            log_debug(logger, "Direccion %i y tamaño %i", direccion_fisica->direccion, direccion_fisica->tamanio);
            enviar_leer_memoria(socket_memoria, io_stdout_write->pcb->pid, direccion_fisica->direccion, direccion_fisica->tamanio);
            op_code cod_op = recibir_operacion(socket_memoria);
            if (cod_op != MEMORIA_LEIDA)
            {
                log_error(logger, "La interfaz esperaba recibir una operación MEMORIA_LEIDA de la Memoria pero recibió otra operación");
            }
            t_lectura *leido = recibir_lectura(socket_memoria);
            memcpy(texto + desplazamiento, leido->lectura, leido->tamanio_lectura);
            desplazamiento += leido->tamanio_lectura;
            free(leido->lectura);
            free(leido);
        }
        char fin_de_cadena = '\0';
        memcpy(texto + desplazamiento, &(fin_de_cadena), sizeof(char));

        printf("%s \n", (char *) texto);

        // Enviar confirmación de escritura completada al kernel
        enviar_fin_io_write(socket_kernel, io_stdout_write->pcb);

        // Liberar memoria de las estructuras utilizadas
        free(io_stdout_write->nombre_interfaz);
        list_destroy_and_destroy_elements(io_stdout_write->direcciones_fisicas, destruir_direccion);
        free(io_stdout_write->pcb->cpu_registers);
        free(io_stdout_write->pcb);
        free(io_stdout_write);
        free(texto);
    }
}

void crear_interfaz_dialfs()
{
    tiempo_unidad_trabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");
    path_base_dialfs = config_get_string_value(config, "PATH_BASE_DIALFS");
    block_size = config_get_int_value(config, "BLOCK_SIZE");
    block_count = config_get_int_value(config, "BLOCK_COUNT");
    retraso_compactacion = config_get_int_value(config, "RETRASO_COMPACTACION");
    tamanio_del_archivo_de_bloques = block_count * block_size;
    tamanio_del_archivo_del_bitmap = block_count / 8;

    conectar_memoria();
    // Prueba inicio
    enviar_mensaje_simple(socket_memoria, MENSAJE);
    // Prueba fin
    enviar_nombre_y_tipo(socket_kernel, nombre, DialFS);

    lista_archivos_por_bloque_inicial = list_create();
    for(int i = 0; i < block_count; i++)
    {
        char placeholder[25] = "No hay bloque inicial";
        list_add(lista_archivos_por_bloque_inicial, placeholder);
    }

    DIR *d;
    struct dirent *dir;
    d = opendir(path_base_dialfs);
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (strcmp(dir->d_name, ".") != 0 
                && strcmp(dir->d_name, "..") != 0
                && strcmp(dir->d_name, "bloques.dat") != 0
                && strcmp(dir->d_name, "bitmap.dat") != 0)
            {
                log_trace(logger, "Se encontró un archivo");
                t_metadata_archivo *metadata_archivo = archivo(dir->d_name);
                list_replace(lista_archivos_por_bloque_inicial, metadata_archivo->bloque_inicial, dir->d_name);
                free(metadata_archivo);
            }
        }
        // closedir(d);
    }

    // construir la ruta completa del archivo bloques.dat
    char *path_archivo_de_bloques = string_new();
    string_append(&path_archivo_de_bloques, path_base_dialfs);
    string_append(&path_archivo_de_bloques, "/bloques.dat");
    // Creación del archivo de bloques
    int file_bloques = open(path_archivo_de_bloques, O_CREAT | O_EXCL | O_RDWR, 00777);
    if (file_bloques == -1)
    {
        file_bloques = open(path_archivo_de_bloques, O_RDWR, 00777);
        if (file_bloques == -1)
        {
            log_error(logger, "Error al crear o abrir el archivo de bloques");
        }
        archivo_de_bloques = mmap(NULL, tamanio_del_archivo_de_bloques, PROT_READ | PROT_WRITE, MAP_SHARED, file_bloques, 0);
    }
    else
    {
        ftruncate(file_bloques, tamanio_del_archivo_de_bloques);
        archivo_de_bloques = mmap(NULL, tamanio_del_archivo_de_bloques, PROT_READ | PROT_WRITE, MAP_SHARED, file_bloques, 0);
        msync(archivo_de_bloques, tamanio_del_archivo_de_bloques, MS_SYNC);
    }
    
    // construir la ruta completa del archivo bitmap.dat
    char *path_bitmap = string_new();
    string_append(&path_bitmap, path_base_dialfs);
    string_append(&path_bitmap, "/bitmap.dat");
    // Creación del archivo del bitmap
    int file_bitmap = open(path_bitmap, O_CREAT | O_EXCL | O_RDWR, 00777);
    if (file_bitmap == -1)
    {
        file_bitmap = open(path_bitmap, O_RDWR, 00777);
        if (file_bitmap == -1)
        {
            log_error(logger, "Error al crear o abrir el archivo del bitmap");
        }
        archivo_del_bitmap = mmap(NULL, tamanio_del_archivo_del_bitmap, PROT_READ | PROT_WRITE, MAP_SHARED, file_bitmap, 0);
        bitmap_de_bloques_libres = bitarray_create_with_mode(archivo_del_bitmap, tamanio_del_archivo_del_bitmap, LSB_FIRST);
    }
    else
    {
        ftruncate(file_bitmap, tamanio_del_archivo_del_bitmap);
        archivo_del_bitmap = mmap(NULL, tamanio_del_archivo_del_bitmap, PROT_READ | PROT_WRITE, MAP_SHARED, file_bitmap, 0);
        bitmap_de_bloques_libres = bitarray_create_with_mode(archivo_del_bitmap, tamanio_del_archivo_del_bitmap, LSB_FIRST);
        for (int i = 0; i < bitarray_get_max_bit(bitmap_de_bloques_libres); i++)
        {
            bitarray_clean_bit(bitmap_de_bloques_libres, i);
        }
        msync(archivo_del_bitmap, tamanio_del_archivo_del_bitmap, MS_SYNC);
    }

    while (true)
    {
        log_trace(logger, "Esperando pedido del Kernel");
        op_code cod_op = recibir_operacion(socket_kernel);
        log_trace(logger, "Llegó un pedido del Kernel");
        usleep(tiempo_unidad_trabajo * 1000);

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

void crear_metadata_archivo(char *nombre_archivo, t_metadata_archivo *metadata)
{

    // Construir el path completo del archivo de configuración
    char *archivo_path = string_new();
    string_append(&archivo_path, path_base_dialfs);
    string_append(&archivo_path, "/");
    string_append(&archivo_path, nombre_archivo);

    FILE *f = fopen(archivo_path, "wb");
    fclose(f);

    t_config *config_metadata = config_create(archivo_path);
    free(archivo_path);
    if (config_metadata == NULL)
    {
        printf("Error: No se pudo crear el archivo de metadata para %s\n", nombre_archivo);
    }

    // Convertir valores a string
    char bloque_inicial_str[20];
    char tamanio_archivo_str[20];
    sprintf(bloque_inicial_str, "%d", metadata->bloque_inicial);
    sprintf(tamanio_archivo_str, "%d", metadata->tamanio_archivo);

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
    // Recibir la estructura t_io_fs_archivo desde el Kernel
    log_debug(logger, "El Kernel solicito crear un archivo");
    t_io_fs_archivo *solicitud = recibir_io_fs_archivo(socket_kernel);
    log_info(logger, "PID: %i - Operacion: IO_FS_CREATE", solicitud->pcb->pid);


    // Extraer el PID y el nombre del archivo desde la solicitud
    uint32_t pid = solicitud->pcb->pid;
    char *nombre_archivo = solicitud->nombre_archivo;

    // Log de la acción
    log_info(logger, "PID: %d - Crear Archivo: %s", pid, nombre_archivo);

    // Buscar un bloque libre en el bitmap
    int bloque_inicial = -1;
    for (int i = 0; i < block_count; i++)
    {
        if (!bitarray_test_bit(bitmap_de_bloques_libres, i))
        {
            log_trace(logger, "El bloque %i estaba libre", i);
            bloque_inicial = i;
            bitarray_set_bit(bitmap_de_bloques_libres, i);
            msync(archivo_del_bitmap, tamanio_del_archivo_del_bitmap, MS_SYNC);
            list_replace(lista_archivos_por_bloque_inicial, i, nombre_archivo);
            break;
        }
    }

    if (bloque_inicial == -1)
    {
        log_error(logger, "No hay bloques disponibles para crear el archivo %s", nombre_archivo);
    }

    // Crear la metadata del archivo
    t_metadata_archivo *metadata = malloc(sizeof(t_metadata_archivo));
    metadata->bloque_inicial = bloque_inicial;
    metadata->tamanio_archivo = 0;

    crear_metadata_archivo(nombre_archivo, metadata);
    log_trace(logger, "Archivo de metadatos creado");

    // Enviar confirmacion a Kernel
    enviar_fin_io_fs(socket_kernel, solicitud->pcb);

    //  Liberar memoria
    free(solicitud->nombre_interfaz);
    //free(solicitud->nombre_archivo); //Nuevo free 
    free(solicitud->pcb->cpu_registers);
    free(solicitud->pcb);
    free(solicitud);
    free(metadata);
}

void manejar_io_fs_delete()
{
    // Recibir la estructura t_io_fs_delete desde el kernel
    log_debug(logger, "El Kernel solicito eliminar un archivo");
    t_io_fs_archivo *io_fs_delete = recibir_io_fs_archivo(socket_kernel);
    log_info(logger, "PID: %i - Operacion: IO_FS_DELETE", io_fs_delete->pcb->pid);

    // Log de la acción
    log_info(logger, "PID: %d - Eliminar Archivo: %s", io_fs_delete->pcb->pid, io_fs_delete->nombre_archivo);

    // Leer el archivo de metadatos para obtener bloques utilizados
    t_metadata_archivo *metadata_archivo = archivo(io_fs_delete->nombre_archivo);

    int bloques_usados = metadata_archivo->tamanio_archivo / block_size; // Calcular cantidad de bloques
    if ((metadata_archivo->tamanio_archivo % block_size != 0) || (metadata_archivo->tamanio_archivo == 0))
    {
        bloques_usados++;
    }

    // Actualizar bitmap
    for (int i = 0; i < bloques_usados; i++)
    {
        bitarray_clean_bit(bitmap_de_bloques_libres, metadata_archivo->bloque_inicial + i);
    }

    // Eliminar el archivo de metadatos
    char *archivo_path = string_new();
    string_append(&archivo_path, path_base_dialfs);
    string_append(&archivo_path, "/");
    string_append(&archivo_path, io_fs_delete->nombre_archivo);
    if (remove(archivo_path) == 0)
    {
        log_info(logger, "PID: %d - Eliminar Archivo: %s", io_fs_delete->pcb->pid, io_fs_delete->nombre_archivo);
    }
    else
    {
        log_error(logger, "PID: %d - Eliminar Archivo: %s - Error al intentar eliminar el archivo.", io_fs_delete->pcb->pid, io_fs_delete->nombre_archivo);
    }
    free(archivo_path);

    // Enviar confirmacion a Kernel
    enviar_fin_io_fs(socket_kernel, io_fs_delete->pcb);

    // Liberar estructura de solicitud después de su uso
    free(metadata_archivo);
    free(io_fs_delete->nombre_interfaz);
    //free(io_fs_delete->nombre_archivo); //Nuevo free
    free(io_fs_delete->pcb->cpu_registers);
    free(io_fs_delete->pcb);
    free(io_fs_delete);
}

void manejar_io_fs_truncate()
{
    log_debug(logger, "El Kernel solicito truncar un archivo");

    // Recibir la solicitud de IO_FS_TRUNCATE
    t_io_fs_truncate *io_fs_truncate = recibir_io_fs_truncate(socket_kernel);
    log_info(logger, "PID: %i - Operacion: IO_FS_TRUNCATE", io_fs_truncate->pcb->pid);

    // Log de la acción
    log_info(logger, "PID: %d - Truncar Archivo: %s", io_fs_truncate->pcb->pid, io_fs_truncate->nombre_archivo);

    // Buscar si el archivo existe en el sistema de archivos DialFS
    t_metadata_archivo *metadata_archivo = archivo(io_fs_truncate->nombre_archivo);

    // Calcular el nuevo tamaño y actualizar la metadata
    int nuevo_tamanio = (int) io_fs_truncate->nuevo_tamanio;

    if (nuevo_tamanio != metadata_archivo->tamanio_archivo)
    {
        log_trace(logger, "Truncando archivo %s a %d bytes.", io_fs_truncate->nombre_archivo, nuevo_tamanio);

        // Actualizar el bitmap y archivo de bloques si necesario
        actualizar_estructura_bloques(metadata_archivo->bloque_inicial,
            metadata_archivo->tamanio_archivo,
            nuevo_tamanio, io_fs_truncate->pcb->pid,
            io_fs_truncate->nombre_archivo);
        
        char *archivo_path = string_new();
        string_append(&archivo_path, path_base_dialfs);
        string_append(&archivo_path, "/");
        string_append(&archivo_path, io_fs_truncate->nombre_archivo);
        t_config *metadata = config_create(archivo_path);
        free(archivo_path);
        config_set_value(metadata, "TAMANIO_ARCHIVO", string_itoa(nuevo_tamanio));
        config_save(metadata);
        config_destroy(metadata);
        metadata_archivo->tamanio_archivo = nuevo_tamanio;
        //free(string_itoa(nuevo_tamanio));
    }

    enviar_fin_io_fs(socket_kernel, io_fs_truncate->pcb);

    // Liberar recursos
    free(metadata_archivo);
    free(io_fs_truncate->nombre_interfaz);
    //free(io_fs_truncate->nombre_archivo); //Nuevo free
    free(io_fs_truncate->pcb->cpu_registers);
    free(io_fs_truncate->pcb);
    free(io_fs_truncate);
}

void manejar_io_fs_write()
{
    // Recibir la estructura t_io_fs_rw desde el Kernel
    t_io_fs_rw *io_fs_write = recibir_io_fs_rw(socket_kernel);
    log_info(logger, "PID: %i - Operacion: IO_FS_WRITE", io_fs_write->pcb->pid);

    log_info(logger, "PID: %d - Escribir Archivo: %s - Tamaño a Escribir: %u - Puntero Archivo: %i",
             io_fs_write->pcb->pid, io_fs_write->nombre_archivo, io_fs_write->tamanio, io_fs_write->puntero_archivo);

    void *valor;
    valor = malloc(io_fs_write->tamanio);
    int desplazamiento_puntero = 0;

    for (int i = 0; i < list_size(io_fs_write->direcciones_fisicas); i++)
    {
        t_direccion_y_tamanio *direccion_y_tamanio = list_get(io_fs_write->direcciones_fisicas, i);
        enviar_leer_memoria(socket_memoria, io_fs_write->pcb->pid, direccion_y_tamanio->direccion, direccion_y_tamanio->tamanio);
        op_code cod_op = recibir_operacion(socket_memoria);
        if(cod_op != MEMORIA_LEIDA)
        {
            log_error(logger, "El DialFS esperaba recibir una operación MEMORIA_LEIDA de la Memoria pero recibió otra operación");
        }
        t_lectura *leido = recibir_lectura(socket_memoria);

        memcpy(valor + desplazamiento_puntero, leido->lectura, leido->tamanio_lectura);
        desplazamiento_puntero += leido->tamanio_lectura;
        free(leido->lectura);
        free(leido);
    }

    escribir_archivo(io_fs_write->nombre_archivo, io_fs_write->puntero_archivo, valor, io_fs_write->tamanio);
    log_trace(logger, "Archivo escrito");

    // Enviar confirmación de escritura completada al Kernel
    enviar_fin_io_fs(socket_kernel, io_fs_write->pcb);

    // Liberar recursos
    free(valor); //Nuevo free
    free(io_fs_write->nombre_interfaz);
    free(io_fs_write->nombre_archivo);
    free(io_fs_write->pcb->cpu_registers);
    free(io_fs_write->pcb);
    list_destroy_and_destroy_elements(io_fs_write->direcciones_fisicas, destruir_direccion);
    free(io_fs_write);
}

void manejar_io_fs_read()
{
    t_io_fs_rw *io_fs_read = recibir_io_fs_rw(socket_kernel);
    log_info(logger, "PID: %i - Operacion: IO_FS_READ", io_fs_read->pcb->pid);

    // Logear inicio de la operación con el formato específico
    log_info(logger, "PID: %d - Leer Archivo: %s - Tamaño a Leer: %u - Puntero Archivo: %u",
             io_fs_read->pcb->pid, io_fs_read->nombre_archivo, io_fs_read->tamanio, io_fs_read->puntero_archivo);

    void *valor;
    valor = leer_archivo(io_fs_read->nombre_archivo, io_fs_read->puntero_archivo, io_fs_read->tamanio);
    int desplazamiento_puntero = 0;

    for (int i = 0; i < list_size(io_fs_read->direcciones_fisicas); i++)
    {
        t_direccion_y_tamanio *direccion_y_tamanio = list_get(io_fs_read->direcciones_fisicas, i);
        void *valor_a_escribir = malloc(direccion_y_tamanio->tamanio);
        memcpy(valor_a_escribir, valor + desplazamiento_puntero, direccion_y_tamanio->tamanio);
        desplazamiento_puntero += direccion_y_tamanio->tamanio;

        enviar_escribir_memoria(socket_memoria, io_fs_read->pcb->pid, direccion_y_tamanio->direccion, direccion_y_tamanio->tamanio, valor_a_escribir);
        free(valor_a_escribir);
        op_code cod_op = recibir_operacion(socket_memoria);
        if(cod_op != MEMORIA_ESCRITA)
        {
            log_error(logger, "El DialFS esperaba recibir una operación MEMORIA_ESCRITA de la Memoria pero recibió otra operación");
        }
        recibir_ok(socket_memoria);
    }

    log_trace(logger, "Archivo leído");

    //Envio a Kernel
    enviar_fin_io_fs(socket_kernel, io_fs_read->pcb);

    // Liberar recursos de la estructura recibida
    free(io_fs_read->nombre_interfaz);
    free(io_fs_read->nombre_archivo);
    free(io_fs_read->pcb->cpu_registers);
    free(io_fs_read->pcb);
    list_destroy_and_destroy_elements(io_fs_read->direcciones_fisicas, destruir_direccion);
    free(io_fs_read);
    free(valor); //Nuevo free
}

void actualizar_estructura_bloques(int bloque_inicial, int tamanio_actual, int nuevo_tamanio, int pid, char *archivo_nombre)
{
    int bloques_actuales = tamanio_actual / block_size;
    if ((tamanio_actual % block_size != 0) || (tamanio_actual == 0))
    {
        bloques_actuales++;
    }

    int nuevos_bloques = nuevo_tamanio / block_size;
    if (nuevo_tamanio % block_size != 0)
    {
        nuevos_bloques++;
    }

    if (nuevos_bloques > bloques_actuales)
    {
        verificar_y_compactar_fs(bloque_inicial, bloques_actuales, nuevos_bloques, pid, archivo_nombre);
    }
    else if (nuevos_bloques < bloques_actuales)
    {
        for (int i = nuevos_bloques; i < bloques_actuales; i++)
        {
            bitarray_clean_bit(bitmap_de_bloques_libres, bloque_inicial + i);
        }
    }

    msync(archivo_de_bloques, tamanio_del_archivo_de_bloques, MS_SYNC);
    msync(archivo_del_bitmap, tamanio_del_archivo_del_bitmap, MS_SYNC);

    log_debug(logger, "PID: %d - Estructura de bloques actualizada. Bloques usados ahora: %d", pid, nuevos_bloques);
}

void verificar_y_compactar_fs(int bloque_inicial, int bloques_actuales, int nuevos_bloques, int pid, char *archivo_nombre)
{
    bool espacio_contiguo = true;
    for (int i = bloques_actuales; i < nuevos_bloques; i++)
    {
        if (bitarray_test_bit(bitmap_de_bloques_libres, bloque_inicial + i))
        {
            log_trace(logger, "El bit %i está ocuapado", bloque_inicial + i);
            espacio_contiguo = false;
            break;
        }
    }

    if (!espacio_contiguo)
    {
        log_debug(logger, "PID: %d - Espacio no contiguo detectado, iniciando compactación", pid);
        log_info(logger, "PID: %i - Inicio Compactación.", pid);
        compactar_fs(bloque_inicial, bloques_actuales, archivo_nombre);
        log_info(logger, "PID: %i - Fin Compactación.", pid);
        log_debug(logger, "PID: %d - Compactación completada", pid);
        t_metadata_archivo *metadata_archivo = archivo(archivo_nombre);
        bloque_inicial = metadata_archivo->bloque_inicial;
        free(metadata_archivo);
        for (int i = bloques_actuales; i < nuevos_bloques; i++)
        {
            bitarray_set_bit(bitmap_de_bloques_libres, bloque_inicial + i);
        }
    }
    else
    {
        for (int i = bloques_actuales; i < nuevos_bloques; i++)
        {
            bitarray_set_bit(bitmap_de_bloques_libres, bloque_inicial + i);
        }
    }
}

void compactar_fs(int bloque_inicial, int bloques_actuales, char *archivo_nombre)
{
    log_trace(logger, "Iniciando compactación del sistema de archivos");

    int total_blocks = block_count;

    // Guardamos el contenido del archivo en un buffer para poder liberar sus bloques y compactar tranquilamente
    void *buffer_archivo = leer_archivo_compactacion(archivo_nombre);
    for (int i = 0; i < bloques_actuales; i++)
    {
        bitarray_clean_bit(bitmap_de_bloques_libres, bloque_inicial + i);
    }

    int indice_escritura = 0; // Dónde escribir el próximo bloque compactado
    int final_de_la_compactacion = 0; // El último bloque del último archivo en ser compactado

    // Recorrer todos los bloques y mover los ocupados al inicio
    for (int i = 0; i < total_blocks; i++)
    {
        if (bitarray_test_bit(bitmap_de_bloques_libres, i))
        {
            if(indice_escritura > i)
            {
                continue;
            }
            if (indice_escritura != i)
            {
                // Simular el movimiento de bloques en el archivo físico
                final_de_la_compactacion = mover_archivo(i, indice_escritura);
                log_trace(logger, "Último bloque compactado: %i", final_de_la_compactacion);

                indice_escritura = final_de_la_compactacion;
            }
            else
            {
                indice_escritura++; // El bloque ya está en la posición correcta
            }
        }
    }

    escribir_archivo_compactacion(archivo_nombre, final_de_la_compactacion, buffer_archivo);

    // Retrasar la compactación para simular tiempo de procesamiento
    usleep(retraso_compactacion * 1000);
}

int mover_archivo(int bloque_origen, int bloque_destino)
{
    log_trace(logger, "Buscando el archivo de bloque inicial %i", bloque_origen);
    char *nombre_archivo_a_mover = list_get(lista_archivos_por_bloque_inicial, bloque_origen);
    t_metadata_archivo *metadata_archivo_a_mover = archivo(nombre_archivo_a_mover);

    int tamanio_archivo_a_mover = metadata_archivo_a_mover->tamanio_archivo;
    int bloques_del_archivo = tamanio_archivo_a_mover / block_size; // Calcular cantidad de bloques
    if ((tamanio_archivo_a_mover % block_size != 0) || (tamanio_archivo_a_mover == 0))
    {
        bloques_del_archivo++;
    }

    // Leer el bloque origen
    void *buffer = leer_archivo_compactacion(nombre_archivo_a_mover);
    for (int i = 0; i < bloques_del_archivo; i++)
    {
        bitarray_clean_bit(bitmap_de_bloques_libres, metadata_archivo_a_mover->bloque_inicial + i);
    }
    char placeholder[25] = "No hay bloque inicial";
    list_replace(lista_archivos_por_bloque_inicial, bloque_origen, placeholder);

    // Escribir en la posición destino
    escribir_archivo_compactacion(nombre_archivo_a_mover, bloque_destino, buffer);
    
    // free(nombre_archivo_a_mover);
    free(metadata_archivo_a_mover);

    return bloque_destino + bloques_del_archivo;
}

t_metadata_archivo *archivo(char *nombre)
{
    char *archivo_path = string_new();
    string_append(&archivo_path, path_base_dialfs);
    string_append(&archivo_path, "/");
    string_append(&archivo_path, nombre);

    t_config *config_metadata = config_create(archivo_path);
    if(config_metadata == NULL)
    {
        log_error(logger, "No se encontró el archivo %s en el DialFS", nombre);
    }
    free(archivo_path);

    t_metadata_archivo *metadata = malloc(sizeof(t_metadata_archivo));
    metadata->bloque_inicial = config_get_int_value(config_metadata, "BLOQUE_INICIAL");
    metadata->tamanio_archivo = config_get_int_value(config_metadata, "TAMANIO_ARCHIVO");

    config_destroy(config_metadata);

    return metadata;
}

void escribir_archivo(char *nombre_archivo, int puntero, void *valor, uint32_t tamanio)
{
    int tamanio_a_escribir = (int) tamanio;

    t_metadata_archivo *metadata_archivo = archivo(nombre_archivo);

    int bloque_del_puntero = puntero / block_size + metadata_archivo->bloque_inicial;
    free(metadata_archivo);
    int desplazamiento_en_primer_bloque = puntero % block_size;
    log_trace(logger, "El puntero apunta al bloque %i, con un desplazamiento de %i bytes", bloque_del_puntero, desplazamiento_en_primer_bloque);

    // Si alcanza con un bloque
    if (tamanio_a_escribir <= block_size - desplazamiento_en_primer_bloque)
    {
        memcpy(archivo_de_bloques + bloque_del_puntero * block_size + desplazamiento_en_primer_bloque,
            valor,
            tamanio_a_escribir);
    }
    // Si hacen falta al menos dos bloques
    else
    {
        memcpy(archivo_de_bloques + bloque_del_puntero * block_size + desplazamiento_en_primer_bloque,
            valor,
            block_size - desplazamiento_en_primer_bloque);
        tamanio_a_escribir -= block_size - desplazamiento_en_primer_bloque;
        int desplazamiento_valor = block_size - desplazamiento_en_primer_bloque;
        while (tamanio_a_escribir >= block_size)
        {
            memcpy(archivo_de_bloques + bloque_del_puntero * block_size + desplazamiento_en_primer_bloque + desplazamiento_valor,
                valor + desplazamiento_valor,
                block_size);
            tamanio_a_escribir -= block_size;
            desplazamiento_valor += block_size;
        }
        if (tamanio_a_escribir > 0)
        {
            memcpy(archivo_de_bloques + bloque_del_puntero * block_size + desplazamiento_en_primer_bloque + desplazamiento_valor,
                valor + desplazamiento_valor,
                tamanio_a_escribir);
        }
    }
    msync(archivo_de_bloques, block_count * block_size, MS_SYNC);
}

void *leer_archivo(char *nombre_archivo, int puntero, uint32_t tamanio)
{
    int tamanio_a_leer = (int) tamanio;

    t_metadata_archivo *metadata_archivo = archivo(nombre_archivo);

    int bloque_del_puntero = puntero / block_size + metadata_archivo->bloque_inicial;
    free(metadata_archivo);
    int desplazamiento_en_primer_bloque = puntero % block_size;
    log_trace(logger, "El puntero apunta al bloque %i, con un desplazamiento de %i bytes", bloque_del_puntero, desplazamiento_en_primer_bloque);

    void *valor;
    valor = malloc(tamanio);

    // Si alcanza con un bloque
    if (tamanio_a_leer <= block_size - desplazamiento_en_primer_bloque)
    {
        memcpy(valor,
            archivo_de_bloques + bloque_del_puntero * block_size + desplazamiento_en_primer_bloque,
            tamanio_a_leer);
    }
    // Si hacen falta al menos dos bloques
    else
    {
        memcpy(valor,
            archivo_de_bloques + bloque_del_puntero * block_size + desplazamiento_en_primer_bloque,
            block_size - desplazamiento_en_primer_bloque);
        tamanio_a_leer -= block_size - desplazamiento_en_primer_bloque;
        int desplazamiento_valor = block_size - desplazamiento_en_primer_bloque;
        while (tamanio_a_leer >= block_size)
        {
            memcpy(valor + desplazamiento_valor,
                archivo_de_bloques + bloque_del_puntero * block_size + desplazamiento_en_primer_bloque + desplazamiento_valor,
                block_size);
            tamanio_a_leer -= block_size;
            desplazamiento_valor += block_size;
        }
        if (tamanio_a_leer > 0)
        {
            memcpy(valor + desplazamiento_valor,
                archivo_de_bloques + bloque_del_puntero * block_size + desplazamiento_en_primer_bloque + desplazamiento_valor,
                tamanio_a_leer);
        }
    }

    return valor;
}

void escribir_archivo_compactacion(char *nombre_archivo, int bloque_inicial, void *valor)
{
    t_metadata_archivo *metadata_archivo = archivo(nombre_archivo);
    int tamanio_a_escribir = metadata_archivo->tamanio_archivo;
    metadata_archivo->bloque_inicial = bloque_inicial;
    free(metadata_archivo);

    bitarray_set_bit(bitmap_de_bloques_libres, bloque_inicial);
    memcpy(archivo_de_bloques + bloque_inicial * block_size,
        valor,
        block_size);
    tamanio_a_escribir -= block_size;
    int desplazamiento_valor = block_size;
    int bloques_extra = 1;
    while (tamanio_a_escribir >= block_size)
    {
        bitarray_set_bit(bitmap_de_bloques_libres, bloque_inicial + bloques_extra);
        memcpy(archivo_de_bloques + bloque_inicial * block_size + desplazamiento_valor,
            valor + desplazamiento_valor,
            block_size);
        tamanio_a_escribir -= block_size;
        desplazamiento_valor += block_size;
        bloques_extra++;
    }
    if (tamanio_a_escribir > 0)
    {
        bitarray_set_bit(bitmap_de_bloques_libres, bloque_inicial + bloques_extra);
        memcpy(archivo_de_bloques + bloque_inicial * block_size + desplazamiento_valor,
            valor + desplazamiento_valor,
            tamanio_a_escribir);
    }
    free(valor);

    char *archivo_path = string_new();
    string_append(&archivo_path, path_base_dialfs);
    string_append(&archivo_path, "/");
    string_append(&archivo_path, nombre_archivo);
    t_config *metadata = config_create(archivo_path);
    config_set_value(metadata, "BLOQUE_INICIAL",string_itoa(bloque_inicial));
    config_save(metadata);
    config_destroy(metadata);
    free(archivo_path);

    list_replace(lista_archivos_por_bloque_inicial, bloque_inicial, nombre_archivo);
    //free(string_itoa(bloque_inicial));
}

void *leer_archivo_compactacion(char *nombre_archivo)
{
    t_metadata_archivo *metadata_archivo = archivo(nombre_archivo);

    int tamanio_a_leer = metadata_archivo->tamanio_archivo;
    int bloque_del_puntero = metadata_archivo->bloque_inicial;
    int bloques_a_leer = metadata_archivo->tamanio_archivo / block_size; // Calcular cantidad de bloques
    if ((metadata_archivo->tamanio_archivo % block_size != 0) || (metadata_archivo->tamanio_archivo == 0))
    {
        bloques_a_leer++;
    }
    log_trace(logger, "Leerá %i bytes (%i bloques) desde el bloque %i (el tamaño de bloque es %i)",
        tamanio_a_leer, bloques_a_leer, bloque_del_puntero, block_size);
    free(metadata_archivo);

    void *valor;
    valor = malloc(bloques_a_leer * block_size);

    int desplazamiento_valor = 0;
    for(int i = 0; i < bloques_a_leer; i++)
    {
        memcpy(valor + desplazamiento_valor,
            archivo_de_bloques + bloque_del_puntero * block_size + desplazamiento_valor,
            block_size);
        desplazamiento_valor += block_size;
    }

    return valor;
}

void lista_de_bloques_iniciales()
{
    for (int i = 0; i < list_size(lista_archivos_por_bloque_inicial); i++)
    {
        log_trace(logger, "%i: %s", i, (char *)list_get(lista_archivos_por_bloque_inicial, i));
    }
}