#include "kernel.h"

/*-----------------------------ESTRUCTURAS GLOBALES--------------------------------------------------------------------*/

t_config *config;
t_log *logger;
t_list *lista_interfaces;
t_queue *cola_new;
t_queue *cola_ready;
t_queue *cola_prio;

t_list *lista_bloqueados;

t_temporal *cron_quant_vrr;

t_list *lista_recursos;

/*-----------------------------VARIABLES GLOBALES--------------------------------------------------------------------*/

int ultimo_pid;
char *ip_cpu;
char *ip_memoria;

int socket_cpu_dispatch;
int socket_cpu_interrupt;
int socket_memoria;
int socket_kernel;
int numero_de_entradasalida;
int grado_multiprogramacion_activo;
int grado_multiprogramacion_max;
int quantum;

t_pcb *pcb_ejecutandose;
bool proceso_en_ejecucion;

char *algoritmo_planificacion;

bool planificar;

/*-----------------------------SEMAFOROS--------------------------------------------------------------------*/
sem_t sem_multiprogramacion;
sem_t mutex_cola_new;
sem_t sem_proceso_ready;
sem_t sem_round_robin;
sem_t sem_vrr_block;
sem_t sem_planificacion;
sem_t mutex_memoria;
sem_t mutex_quantum;

/*-----------------------------HILOS--------------------------------------------------------------------*/
pthread_t hilo_planificador_largo_plazo;
pthread_t hilo_planificador_corto_plazo;
pthread_t hilo_recibir_entradasalida;
pthread_t hilo_entradasalida[100];
pthread_t hilo_quantum_vrr;

int main(int argc, char *argv[])
{
    sem_init(&sem_multiprogramacion, 0, 0);
    sem_init(&mutex_cola_new, 0, 1);
    sem_init(&sem_proceso_ready, 0, 0);
    sem_init(&sem_round_robin, 0, 1);
    sem_init(&sem_vrr_block, 0, 0);
    sem_init(&sem_planificacion, 0, 0);
    sem_init(&mutex_memoria, 0, 1);
    sem_init(&mutex_quantum,0,1);

    planificar = false;

    crear_logger();
    crear_config();

    decir_hola("Kernel");

    obtener_recursos();

    // El grado de multiprogramacion va a ser una variable global, que van a manejar entre CPU y Kernel
    grado_multiprogramacion_max = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
    // El grado activo empieza en 0 y se ira incrementando
    grado_multiprogramacion_activo = 0;

    // Creo las colas que voy a usar para guardar mis PCBs
    cola_new = queue_create();
    cola_ready = queue_create();
    cola_prio = queue_create();
    lista_bloqueados = list_create();

    // Usan la misma IP, se las paso por parametro.
    // Conexiones con CPU: Dispatch e Interrupt
    ip_cpu = config_get_string_value(config, "IP_CPU");
    conectar_dispatch_cpu(ip_cpu);
    conectar_interrupt_cpu(ip_cpu);

    // Kernel a Memoria
    conectar_memoria();

    // Entrada salida a Kernel
    char *puerto_kernel = config_get_string_value(config, "PUERTO_ESCUCHA");
    socket_kernel = iniciar_servidor(puerto_kernel);

    numero_de_entradasalida = 0;
    lista_interfaces = list_create();

    // Creo un hilo para el planificador de largo plazo
    if (pthread_create(&hilo_recibir_entradasalida, NULL, recibir_entradasalida, NULL) != 0)
    {
        log_error(logger, "Error al inicializar el Hilo de Entradas y Salidas");
        exit(EXIT_FAILURE);
    }

    algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");

    proceso_en_ejecucion = false;

    // Creo un hilo para el planificador de largo plazo
    if (pthread_create(&hilo_planificador_largo_plazo, NULL, planificador_largo_plazo, NULL) != 0)
    {
        log_error(logger, "Error al inicializar el Hilo Planificador de Largo Plazo");
        exit(EXIT_FAILURE);
    }
    // Este hilo debe ser independiente dado que el planificador nunca se debe apagar.
    pthread_detach(hilo_planificador_largo_plazo);
    // Creo un hilo para el planificador de corto plazo
    if (pthread_create(&hilo_planificador_corto_plazo, NULL, planificador_corto_plazo, NULL) != 0)
    {
        log_error(logger, "Error al inicializar el Hilo Planificador de Corto Plazo");
        exit(EXIT_FAILURE);
    }
    // Este hilo debe ser independiente dado que el planificador nunca se debe apagar.
    pthread_detach(hilo_planificador_corto_plazo);

    ultimo_pid = 0;

    consola();

    liberar_conexion(socket_cpu_dispatch);
    liberar_conexion(socket_cpu_interrupt);

    log_info(logger, "Terminó");

    return 0;
}

void crear_logger()
{
    logger = log_create("./kernel.log", "LOG_KERNEL", false, LOG_LEVEL_TRACE);
    if (logger == NULL)
    {
        perror("Ocurrió un error al leer el archivo de Log de Kernel");
        abort();
    }
}

void crear_config()
{
    config = config_create("./kernel_deadlock.config");
    if (config == NULL)
    {
        log_error(logger, "Ocurrió un error al leer el archivo de Configuración del Kernel\n");
        abort();
    }
}

void conectar_memoria()
{
    // Establecer conexión con el módulo Memoria
    ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    char *puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    socket_memoria = conectar_modulo(ip_memoria, puerto_memoria);
}

void conectar_dispatch_cpu(char *ip_cpu)
{
    // Establecer conexión con el módulo CPU (dispatch)
    char *puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    socket_cpu_dispatch = conectar_modulo(ip_cpu, puerto_cpu_dispatch);
}

void conectar_interrupt_cpu(char *ip_cpu)
{
    // Establecer conexión con el módulo CPU (interrupt)
    char *puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
    socket_cpu_interrupt = conectar_modulo(ip_cpu, puerto_cpu_interrupt);
}

void obtener_recursos()
{
    lista_recursos = list_create();

    char **recursos;
    char **instancias_recursos;
    recursos = config_get_array_value(config, "RECURSOS");
    instancias_recursos = config_get_array_value(config, "INSTANCIAS_RECURSOS");

    int i = 0;
    while (recursos[i] != NULL)
    {
        t_manejo_de_recurso *recurso = malloc(sizeof(t_manejo_de_recurso));
        recurso->nombre = recursos[i];
        recurso->instancias = atoi(instancias_recursos[i]);
        recurso->procesos_esperando = queue_create();
        recurso->procesos_asignados = list_create();
        list_add(lista_recursos, recurso);
        i++;
    }

    for (int i = 0; i < list_size(lista_recursos); i++)
    {
        t_manejo_de_recurso *recurso_a_mostrar = (t_manejo_de_recurso *)list_get(lista_recursos, i);
        log_debug(logger, "El recurso %s tiene %i instancias", recurso_a_mostrar->nombre, recurso_a_mostrar->instancias);
    }
}

/*-----------------------------ENTRADA SALIDA--------------------------------------------------------------------*/
void *recibir_entradasalida()
{
    while (1)
    {
        // Espero a un cliente (entradasalida).
        int socket_entradasalida = esperar_cliente(socket_kernel);
        log_trace(logger, "Se conectó una interfaz con socket %i", socket_entradasalida);

        op_code cod_op = recibir_operacion(socket_entradasalida);
        if (cod_op != NOMBRE_Y_TIPO_IO)
        {
            log_error(logger, "El Kernel esperaba recibir el nombre y tipo de la interfaz pero recibió otra cosa");
        }
        t_nombre_y_tipo_io *nombre_y_tipo = recibir_nombre_y_tipo(socket_entradasalida);

        // Guardamos nombre y socket de la interfaz
        t_interfaz *interfaz = malloc(sizeof(t_interfaz));
        cargar_interfaz_recibida(interfaz, socket_entradasalida, nombre_y_tipo->nombre, nombre_y_tipo->tipo);

        log_trace(logger, "La interfaz tiene nombre %s", interfaz->nombre);

        // Creamos el hilo. Manejo de interfaces va a chequear el tipo de interfaz
        pthread_create(&hilo_entradasalida[numero_de_entradasalida], NULL, manejo_interfaces, interfaz);
        // Creamos el hilo
        pthread_detach(hilo_entradasalida[numero_de_entradasalida]);
        numero_de_entradasalida++;

        free(nombre_y_tipo);
    }

    // Cerrar conexiónes con el cliente
    liberar_conexion(socket_kernel);
}

void *manejo_interfaces(void *interfaz_hilo)
{
    bool sigue_conectado = true;
    while (sigue_conectado)
    {
        t_interfaz *interfaz = interfaz_hilo;
        switch (interfaz->tipo)
        {
        case GENERICA:
            sigue_conectado = fin_sleep(interfaz);
            break;
        case STDIN:
            sigue_conectado = fin_io_read(interfaz);
            break;
        case STDOUT:
            sigue_conectado = fin_io_write(interfaz);
            break;
        case DialFS:
            sigue_conectado = fin_io_fs(interfaz);
            break;
        }
    }
    return NULL;
}

void cargar_interfaz_recibida(t_interfaz *interfaz, int socket_entradasalida, char *nombre, tipo_interfaz tipo)
{
    interfaz->socket = socket_entradasalida;
    interfaz->nombre = nombre;
    interfaz->tipo = tipo;
    interfaz->ocupada = false;
    interfaz->cola_procesos_esperando = queue_create();
    list_add(lista_interfaces, interfaz);
}

bool fin_sleep(t_interfaz *interfaz)
{
    op_code cod_op = recibir_operacion(interfaz->socket);
    if (cod_op == DESCONEXION)
    {
        log_warning(logger, "Se desconectó la interfaz %s", interfaz->nombre);
        return false;
        // TODO: Liberar estructuras
    }
    else if (cod_op != FIN_SLEEP)
    {
        log_error(logger, "El Kernel esperaba recibir el aviso de fin de sleep pero recibió otra cosa");
        return false;
    }
    else
    {
        desbloquear_proceso_io(interfaz);
        if (!list_is_empty(interfaz->cola_procesos_esperando->elements))
        {
            t_sleep *sleep = queue_pop(interfaz->cola_procesos_esperando);
            enviar_sleep(interfaz->socket, sleep->pcb, sleep->nombre_interfaz, sleep->unidades_de_trabajo);
        }
        return true;
    }
}

bool fin_io_read(t_interfaz *interfaz)
{
    op_code cod_op = recibir_operacion(interfaz->socket);
    if (cod_op == DESCONEXION)
    {
        log_warning(logger, "Se desconectó la interfaz %s", interfaz->nombre);
        return false;
        // TODO: Liberar estructuras
    }
    else if (cod_op != FIN_IO_READ)
    {
        log_error(logger, "El Kernel esperaba recibir el aviso de fin de IO_READ pero recibió otra cosa");
        return false;
    }
    else
    {
        desbloquear_proceso_io(interfaz);
        if (!list_is_empty(interfaz->cola_procesos_esperando->elements))
        {
            t_io_std *io_stdin_read = queue_pop(interfaz->cola_procesos_esperando);
            enviar_io_stdin_read(interfaz->socket, io_stdin_read);
        }
        return true;
    }
}

bool fin_io_write(t_interfaz *interfaz)
{
    op_code cod_op = recibir_operacion(interfaz->socket);
    if (cod_op == DESCONEXION)
    {
        log_warning(logger, "Se desconectó la interfaz %s", interfaz->nombre);
        return false;
        // TODO: Liberar estructuras
    }
    else if (cod_op != FIN_IO_WRITE)
    {
        log_error(logger, "El Kernel esperaba recibir el aviso de fin de IO_WRITE pero recibió otra cosa");
        return false;
    }
    else
    {
        desbloquear_proceso_io(interfaz);
        if (!list_is_empty(interfaz->cola_procesos_esperando->elements))
        {
            t_io_std *io_stdout_write = queue_pop(interfaz->cola_procesos_esperando);
            enviar_io_stdout_write(interfaz->socket, io_stdout_write);
        }
        return true;
    }
}

//La finalizacion de procesos para File System es siempre igual, reutilizo el mismo codigo.
bool fin_io_fs(t_interfaz *interfaz)
{
    op_code cod_op = recibir_operacion(interfaz->socket);
    if (cod_op == DESCONEXION)
    {
        log_warning(logger, "Se desconectó la interfaz %s", interfaz->nombre);
        return false; //Desconecta la interfaz
        // TODO: Liberar estructuras
    }
    else if (cod_op != FIN_IO_FS)
    {
        log_error(logger, "El Kernel esperaba recibir el aviso de fin de IO_FS pero recibió otra cosa");
        return false; // False desconecta la interfaz
    }
    else
    {
        desbloquear_proceso_io(interfaz);
        if (!list_is_empty(interfaz->cola_procesos_esperando->elements))
        {
            t_io_fs_comodin *comodin = queue_pop(interfaz->cola_procesos_esperando);
            switch(comodin->operacion)
            {
            case CREATE:
                t_io_fs_archivo *io_fs_create = malloc(sizeof(t_io_fs_archivo));
                io_fs_create->pcb = comodin->pcb;
                io_fs_create->nombre_interfaz = comodin->nombre_interfaz;
                io_fs_create->tamanio_nombre_interfaz = comodin->tamanio_nombre_interfaz;
                io_fs_create->nombre_archivo = comodin->nombre_archivo;
                io_fs_create->tamanio_nombre_archivo = comodin->tamanio_nombre_archivo;
                enviar_io_fs_create(interfaz->socket, io_fs_create);
                break;
            case DELETE:
                t_io_fs_archivo *io_fs_delete = malloc(sizeof(t_io_fs_archivo));
                io_fs_delete->pcb = comodin->pcb;
                io_fs_delete->nombre_interfaz = comodin->nombre_interfaz;
                io_fs_delete->tamanio_nombre_interfaz = comodin->tamanio_nombre_interfaz;
                io_fs_delete->nombre_archivo = comodin->nombre_archivo;
                io_fs_delete->tamanio_nombre_archivo = comodin->tamanio_nombre_archivo;
                enviar_io_fs_delete(interfaz->socket, io_fs_delete);
                break;
            case TRUNCATE:
                t_io_fs_truncate *io_fs_truncate = malloc(sizeof(t_io_fs_truncate));
                io_fs_truncate->pcb = comodin->pcb;
                io_fs_truncate->nombre_interfaz = comodin->nombre_interfaz;
                io_fs_truncate->tamanio_nombre_interfaz = comodin->tamanio_nombre_interfaz;
                io_fs_truncate->nombre_archivo = comodin->nombre_archivo;
                io_fs_truncate->tamanio_nombre_archivo = comodin->tamanio_nombre_archivo;
                io_fs_truncate->nuevo_tamanio = comodin->tamanio;
                enviar_io_fs_truncate(interfaz->socket, io_fs_truncate);
                break;
            case WRITE:
                t_io_fs_rw *io_fs_write = malloc(sizeof(t_io_fs_rw));
                io_fs_write->pcb = comodin->pcb;
                io_fs_write->nombre_interfaz = comodin->nombre_interfaz;
                io_fs_write->tamanio_nombre_interfaz = comodin->tamanio_nombre_interfaz;
                io_fs_write->nombre_archivo = comodin->nombre_archivo;
                io_fs_write->tamanio_nombre_archivo = comodin->tamanio_nombre_archivo;
                io_fs_write->tamanio = comodin->tamanio;
                io_fs_write->puntero_archivo = comodin->puntero_archivo;
                io_fs_write->direcciones_fisicas = comodin->direcciones_fisicas;
                enviar_io_fs_write(interfaz->socket, io_fs_write);
                break;
            case READ:
                t_io_fs_rw *io_fs_read = malloc(sizeof(t_io_fs_rw));
                io_fs_read->pcb = comodin->pcb;
                io_fs_read->nombre_interfaz = comodin->nombre_interfaz;
                io_fs_read->tamanio_nombre_interfaz = comodin->tamanio_nombre_interfaz;
                io_fs_read->nombre_archivo = comodin->nombre_archivo;
                io_fs_read->tamanio_nombre_archivo = comodin->tamanio_nombre_archivo;
                io_fs_read->tamanio = comodin->tamanio;
                io_fs_read->puntero_archivo = comodin->puntero_archivo;
                io_fs_read->direcciones_fisicas = comodin->direcciones_fisicas;
                enviar_io_fs_read(interfaz->socket, io_fs_read);
                break;
            default:
                log_error(logger, "Hay un proceso esperando para usar la interfaz, pero no se sabe qué operación necesita");
                break;
            }
            free(comodin);
        }
        return true;
    }
}

// Genero desbloquear procesos IO para no repetir codigo, los desbloqueos van a ser siempre iguales para todas las interfaces
void desbloquear_proceso_io(t_interfaz *interfaz)
{
    // La interfaz ya no está más ocupada
    interfaz->ocupada = false;
    // Recibir el PCB que se desbloqueará
    t_pcb *pcb_a_desbloquear = recibir_pcb(interfaz->socket);
    log_trace(logger, "Una interfaz le avisó al kernel que ya puede desbloquear el proceso %i", pcb_a_desbloquear->pid);

    // Buscamos el proceso en BLOCKED y lo mandamos a READY
    for (int i = 0; i < list_size(lista_bloqueados); i++)
    {
        t_pcb *pcb = (t_pcb *)list_get(lista_bloqueados, i);
        if (pcb->pid == pcb_a_desbloquear->pid)
        {
            list_remove(lista_bloqueados, i);
            pcb->estado = READY;

            if (strcmp(algoritmo_planificacion, "VRR") == 0)
            {
                queue_push(cola_prio, pcb);
            }
            else
            {
                queue_push(cola_ready, pcb);
                log_info(logger, "PID: %i - Estado Anterior: BLOCK - Estado Actual: READY", pcb_a_desbloquear->pid);
                mostrar_cola_ready();
            }

            sem_post(&sem_proceso_ready);
            if (list_is_empty(interfaz->cola_procesos_esperando->elements))
            {
                interfaz->ocupada = false;
            }
        }
    }
}

// FUNCION DEPRECADA ??

void *interfaz_generica(void *interfaz_sleep)
{
    bool sigue_conectado = true;
    while (sigue_conectado)
    {
        t_interfaz *interfaz = interfaz_sleep;

        op_code cod_op = recibir_operacion(interfaz->socket);
        if (cod_op == DESCONEXION)
        {
            log_warning(logger, "Se desconectó la interfaz %s", interfaz->nombre);
            sigue_conectado = false;
            // TODO: Liberar estructuras
        }
        else if (cod_op != FIN_SLEEP)
        {
            log_error(logger, "El Kernel esperaba recibir el aviso de fin de sleep pero recibió otra cosa");
        }
        else
        {
            t_pcb *pcb_a_desbloquear = recibir_pcb(interfaz->socket);

            // Buscamos el proceso en BLOCKED y lo mandamos a READY
            for (int i = 0; i < list_size(lista_bloqueados); i++)
            {
                t_pcb *pcb = (t_pcb *)list_get(lista_bloqueados, i);
                if (pcb->pid == pcb_a_desbloquear->pid)
                {
                    list_remove(lista_bloqueados, i);
                    pcb->estado = READY;

                    sem_post(&sem_proceso_ready);
                    interfaz->ocupada = false;
                }
            }
        }
    }
    return NULL;
}

/*-----------------------------PROCESOS Y CPU--------------------------------------------------------------------*/

// Requisito Checkpoint: Es capaz de crear un PCB y planificarlo por FIFO y RR.
void crear_pcb(char *path)
{
    t_pcb *pcb = malloc(sizeof(t_pcb));
    // Registros. Asumo que se inicializan todos en cero
    t_cpu_registers *registros = malloc(sizeof(t_cpu_registers));
    registros->pc = 0;
    registros->normales[AX] = 0;
    registros->normales[BX] = 0;
    registros->normales[CX] = 0;
    registros->normales[DX] = 0;
    registros->extendidos[EAX] = 0;
    registros->extendidos[EBX] = 0;
    registros->extendidos[ECX] = 0;
    registros->extendidos[EDX] = 0;
    registros->si = 0;
    registros->di = 0;
    // Asignar el PID e incrementarlo
    pcb->pid = ultimo_pid;
    ultimo_pid++;
    // Termino de completar el PCB
    pcb->cpu_registers = registros;
    pcb->quantum = 0;
    pcb->estado = NEW;

    // Informar a la memoria
    sem_wait(&mutex_memoria);
    enviar_creacion_proceso(socket_memoria, pcb, path);
    log_trace(logger, "Estoy esperando el OK de la memoria");

    // Esperar recibir OK de la memoria
    op_code cod_op = recibir_operacion(socket_memoria);
    if (cod_op != CREACION_PROCESO_OK)
    {
        log_error(logger, "La memoria mandó otra cosa en lugar de un OK al solicitarle crear el proceso de PID %i", pcb->pid);
        return;
    }
    recibir_ok(socket_memoria);
    sem_post(&mutex_memoria);
    log_trace(logger, "Recibí OK de la memoria luego de pedirle crear un proceso");

    // El PCB se agrega a la cola de los procesos NEW
    sem_wait(&mutex_cola_new);
    queue_push(cola_new, pcb);
    log_info(logger, "Se crea el proceso %d en NEW", ultimo_pid);
    log_debug(logger, "Hay %i procesos en NEW", queue_size(cola_new));
    // Despertar el mover procesos a ready (si el grado de multiprogramación lo permite)
    if (grado_multiprogramacion_activo < grado_multiprogramacion_max)
    {
        log_debug(logger, "El grado de multiprogramación máximo es %i, y actualmente hay %i proceso, por lo tanto deberá pasar a READY",
            grado_multiprogramacion_max, grado_multiprogramacion_activo);
        sem_post(&sem_multiprogramacion);
        // Aumentamos el grado de multiprogramacion activo
        grado_multiprogramacion_activo++;
    }
    else
    {
        log_debug(logger, "El grado de multiprogramación máximo es %i, y actualmente hay %i proceso, por lo tanto se quedará en NEW",
            grado_multiprogramacion_max, grado_multiprogramacion_activo);
    }
    sem_post(&mutex_cola_new);
}

void encontrar_y_eliminar_proceso(int pid_a_eliminar)
{
    // Buscamos en EXEC
    if(proceso_en_ejecucion)
    {
        if (pcb_ejecutandose->pid == pid_a_eliminar)
        {
            enviar_interrupcion(socket_cpu_interrupt, pcb_ejecutandose, FINALIZAR_PROCESO);
            return;
        }
    }

    // Buscamos en la cola de NEW
    for (int i = 0; i < list_size(cola_new->elements); i++)
    {
        t_pcb *pcb = (t_pcb *)list_get(cola_new->elements, i);
        if (pcb->pid == pid_a_eliminar)
        {
            list_remove(cola_new->elements, i);

            // Acá no se pudo usar eliminar_proceso() porque eso implicaría liberar un espacio en el grado de multiprogramación;
            // este proceso está en NEW y por lo tanto no afecta al grado de multiprogramación
            enviar_finalizacion_proceso(socket_memoria, pcb);
            op_code cod_op = recibir_operacion(socket_memoria);
            if (cod_op != FINALIZACION_PROCESO_OK)
            {
                log_error(logger, "El Kernel esperaba recibir un OK de la memoria (luego de pedirle finalizar un proceso) pero recibió otra cosa");
            }
            recibir_ok(socket_memoria);
            log_trace(logger, "Recibí OK de la memoria luego de pedirle finalizar un proceso");

            if (pcb_ejecutandose->pid == pcb->pid)
            {
                free(pcb_ejecutandose->cpu_registers);
                free(pcb_ejecutandose);
            }
            free(pcb->cpu_registers);
            free(pcb);

            log_info(logger, "Finaliza el proceso %i - Motivo: INTERRUPTED_BY_USER", pid_a_eliminar);
            return;
        }
    }

    // Buscamos en la cola de READY
    for (int i = 0; i < list_size(cola_ready->elements); i++)
    {
        t_pcb *pcb = (t_pcb *)list_get(cola_ready->elements, i);
        if (pcb->pid == pid_a_eliminar)
        {
            list_remove(cola_ready->elements, i);
            eliminar_proceso(pcb);
            log_info(logger, "Finaliza el proceso %i - Motivo: INTERRUPTED_BY_USER", pid_a_eliminar);
            return;
        }
    }

    // Buscamos en la lista de BLOCK
    for (int i = 0; i < list_size(lista_bloqueados); i++)
    {
        t_pcb *pcb = (t_pcb *)list_get(lista_bloqueados, i);
        if (pcb->pid == pid_a_eliminar)
        {
            for (int j = 0; j < list_size(lista_interfaces); j++)
            {
                t_interfaz *interfaz = (t_interfaz *)list_get(lista_interfaces, j);
                for (int k = 0; k < list_size(interfaz->cola_procesos_esperando->elements); k++)
                {
                    t_pcb *pcb_esperando = (t_pcb *)list_get(interfaz->cola_procesos_esperando->elements, k);
                    if (pcb_esperando->pid == pid_a_eliminar)
                    {
                        list_remove(interfaz->cola_procesos_esperando->elements, pid_a_eliminar);
                    }
                }
            }

            list_remove(lista_bloqueados, i);
            eliminar_proceso(pcb);
            log_info(logger, "Finaliza el proceso %i - Motivo: INTERRUPTED_BY_USER", pid_a_eliminar);
            return;
        }
    }

    log_error(logger, "Se buscó en EXEC, NEW, READY y BLOCK, pero no se encontró el procesó a eliminar");
}

/*En caso de que el grado de multiprogramación lo permita, los procesos creados podrán pasar de la cola de NEW a la cola de READY,
caso contrario, se quedarán a la espera de que finalicen procesos que se encuentran en ejecución*/
void *planificador_largo_plazo()
{
    while (1)
    {
        // Esperar hasta que se cree un nuevo PCB (o un proceso pase a EXIT)
        log_trace(logger, "PLP: Esperando nuevo proceso");
        sem_wait(&sem_multiprogramacion);
        log_trace(logger, "PLP: Llegó un nuevo proceso");

        // Verificamos que la planificación no esté pausada
        comprobar_planificacion();

        // Mover procesos de la cola de NEW a la cola de READY
        log_trace(logger, "El grado de multiprogramación permite que un proceso entre a ready");
        // Tomar el primer proceso de la cola de NEW
        sem_wait(&mutex_cola_new);
        t_pcb *proceso_nuevo = queue_pop(cola_new);
        sem_post(&mutex_cola_new);
        // Cambiar el estado del proceso a READY
        proceso_nuevo->estado = READY;

        // Agregar el proceso a la cola de READY

        queue_push(cola_ready, proceso_nuevo);
        log_info(logger, "PID: %i - Estado Anterior: NEW - Estado Actual: READY", proceso_nuevo->pid);
        mostrar_cola_ready();

        sem_post(&sem_proceso_ready);
        
        log_debug(logger, "Hay %i procesos en READY", queue_size(cola_ready));

        // Reducir la cantidad de procesos en la cola de NEW
        log_trace(logger, "PLP: El proceso fue enviado a ready");
    }
}

// Planificador corto plazo
void *planificador_corto_plazo()
{
    pthread_t hilo_quantum_block;
    quantum = config_get_int_value(config, "QUANTUM");
    if (strcmp(algoritmo_planificacion, "VRR") == 0)
    {
        pthread_create(&hilo_quantum_block, NULL, (void *)quantum_block, NULL);
    }
    
    while (1)
    {
        // Esperar a que se cree un nuevo PCB
        log_trace(logger, "PCP: Esperando que llegue un proceso a ready");
        sem_wait(&sem_proceso_ready);
        log_trace(logger, "PCP: Un proceso entró a ready");

        // Verificamos que la planificación no esté pausada
        comprobar_planificacion();

        if (strcmp(algoritmo_planificacion, "FIFO") == 0)
        {
            planificar_fifo();
        }
        else if (strcmp(algoritmo_planificacion, "RR") == 0)
        {
            planificar_round_robin();
        }
        else if (strcmp(algoritmo_planificacion, "VRR") == 0)
        {
            planificar_vrr();
        }
        else
        {
            log_error(logger, "Algoritmo de planificación desconocido");
            exit(EXIT_FAILURE);
        }
    }
}

// Función para planificar los procesos usando FIFO
void planificar_fifo()
{
    if (!queue_is_empty(cola_ready))
    {
        // FIFO toma el primer proceso de la cola
        t_pcb *proceso_a_ejecutar = queue_pop(cola_ready);

        // Estado pasa a ejecucion
        proceso_a_ejecutar->estado = EXEC;

        // Le envío el pcb al CPU a traves del dispatch
        enviar_pcb(socket_cpu_dispatch, proceso_a_ejecutar);
        log_info(logger, "PID: %i - Estado Anterior: READY - Estado Actual: EXEC", proceso_a_ejecutar->pid);

        // Guardamos registro en el Kernel de qué proceso está ejecutándose
        pcb_ejecutandose = proceso_a_ejecutar;
        proceso_en_ejecucion = true;

        // Espero a que el CPU me devuelva el proceso
        esperar_cpu();
    }
}

// Función para planificar los procesos usando Round Robin

void planificar_round_robin()
{
    pthread_t hilo_quantum_rr;
    log_trace(logger, "Inicia ciclo");
    sem_wait(&sem_round_robin);

    // Ahora mismo, hasta que no se termine el quantum, si un proceso finaliza, el siguiente no se ejecuta.
    if (!queue_is_empty(cola_ready))
    {
        //INICIAR_PROCESO PRUEBARR
        log_trace(logger, "Entro if");
        // Obtener el proceso listo para ejecutarse de la cola
        t_pcb *proceso_a_ejecutar = queue_pop(cola_ready);
        //log_trace(logger, "FALLO ENVIAR INT");
        // wait(sem_mutex_interrupt)
        
        // sem_post(sem_mutex_interrupt)
        
        // Cambiar el estado del proceso a EXEC
        proceso_a_ejecutar->estado = EXEC;

        enviar_pcb(socket_cpu_dispatch, proceso_a_ejecutar);
        log_info(logger, "PID: %i - Estado Anterior: READY - Estado Actual: EXEC", proceso_a_ejecutar->pid);
        pcb_ejecutandose = proceso_a_ejecutar;
        proceso_en_ejecucion = true;
        log_trace(logger, "PCBenviado");
        pthread_create(&hilo_quantum_rr, NULL, (void *)quantum_count, proceso_a_ejecutar);
        pthread_detach(hilo_quantum_rr);
        
        esperar_cpu();
        
        log_trace(logger, "Termino ciclo");
        // Si el proceso que envie tiene quantum, voy a chequear cuando tengo que decirle al CPU que corte
        // Tambien lo podriamos hacer del lado de CPU, pero funcionalmente no estaria OK.
        // TODO, desalojar de CPU con interrupcion. ¿Ya está hecho?
    }
    // mutex en la cola de interrupt
}

void planificar_vrr()
{
    sem_wait(&sem_round_robin);

    if ((!queue_is_empty(cola_ready)) || (!queue_is_empty(cola_prio)))
    {
        t_pcb *proceso_a_ejecutar;
        // Obtener el proceso listo para ejecutarse de la cola
        
        if (!queue_is_empty(cola_prio))
        {
            log_trace(logger, "Entro cola prio");
            proceso_a_ejecutar = queue_pop(cola_prio);
        }
        else if (!queue_is_empty(cola_ready))
        {
            log_trace(logger, "Entro cola ready");
            proceso_a_ejecutar = queue_pop(cola_ready);
        }
    
    // sem_mutex_interrupt
    // sem_wait(sem_mutex_interrupt);
    //enviar_interrupcion(socket_cpu_interrupt, pcb_ejecutandose, FIN_DE_QUANTUM);
    //esperar_cpu();
    // sem_post(sem_mutex_interrupt);
    //  Cambiar el estado del proceso a EXEC

    proceso_a_ejecutar->estado = EXEC;

    enviar_pcb(socket_cpu_dispatch, proceso_a_ejecutar);
    log_info(logger, "PID: %i - Estado Anterior: READY - Estado Actual: EXEC", proceso_a_ejecutar->pid);
    log_trace(logger, "Envio PCB");
    cron_quant_vrr = temporal_create();
    pcb_ejecutandose = proceso_a_ejecutar;
    free(proceso_a_ejecutar);
    free(proceso_a_ejecutar->cpu_registers);
    proceso_en_ejecucion = true;
    pthread_create(&hilo_quantum_vrr, NULL, (void *)quantum_count, pcb_ejecutandose);
    pthread_detach(hilo_quantum_vrr);
    esperar_cpu(); //Cambie de lugar la interrupcion al hilo de quantum, deje comentado lo que estaba antes

    log_trace(logger, "Termino ciclo");
    }

}

void quantum_block()
{
    while(1)
    {
        sem_wait(&sem_vrr_block);
        log_trace(logger, "Tenemos un 3312 (proceso bloqueado)");
        sem_wait(&mutex_quantum);
        pcb_ejecutandose->quantum = (int)temporal_gettime(cron_quant_vrr); // agregarlo a todas las io
        log_trace(logger, "Emapanda? %d", pcb_ejecutandose->quantum);
        sem_post(&mutex_quantum);
        temporal_destroy(cron_quant_vrr);
        pthread_cancel(hilo_quantum_vrr);
        //pthread_kill(hilo_quantum_vrr, SIGKILL);
        sem_post(&sem_round_robin);
    }
}

// en caso de que muera el proceso, se resetea cron_quant_vrr y se mata a quantumcount y se agrega a queue_prio
//  valor del timer en el pcb, se manda sempostvrr

void quantum_count(void *proceso_con_quantum)
{
    /*
    //propuesta solucion vrr facu/martin
    t_pcb *pcb = proceso_con_quantum;

    sem_wait(&mutex_quantum); 
    log_trace(logger, "A ver flaco? %d", pcb_ejecutandose->quantum);
    usleep((quantum - pcb_ejecutandose->quantum) * 1000);
    pcb_ejecutandose->quantum = 0;
    sem_post(&mutex_quantum);

    if (strcmp(algoritmo_planificacion, "VRR") == 0)
    {
        temporal_destroy(cron_quant_vrr);
    }
    if(proceso_en_ejecucion)
    {
        enviar_interrupcion(socket_cpu_interrupt, pcb, FIN_DE_QUANTUM);
    }
    log_trace(logger, "Esperando CPU");
    
    sem_post(&sem_round_robin);
    */
    t_pcb *pcb = proceso_con_quantum;
    
    usleep(quantum * 1000);
    if (strcmp(algoritmo_planificacion, "VRR") == 0)
    {
        temporal_destroy(cron_quant_vrr);
    }
    if(proceso_en_ejecucion)
    {
        enviar_interrupcion(socket_cpu_interrupt, pcb, FIN_DE_QUANTUM);
    }
    log_trace(logger, "Esperando CPU");
    
    sem_post(&sem_round_robin);
}

void esperar_cpu()
{
    log_trace(logger, "PCP: Esperando respuesta del CPU");
    op_code cod_op = recibir_operacion(socket_cpu_dispatch);
    log_trace(logger, "PCP: Llegó una respuesta del CPU");

    if (cod_op != WAIT &&
        cod_op != SIGNAL)
    {
        proceso_en_ejecucion = false;
    }
    
    // Verificamos que la planificación no esté pausada
    comprobar_planificacion();
    
    switch (cod_op)
    {
    case DISPATCH: // este case es solo una prueba
        log_debug(logger, "Recibí una respuesta del CPU!");
        if (1)
        {
            t_pcb *pcb_prueba = recibir_pcb(socket_cpu_dispatch);

            log_info(logger, "AX: %i", pcb_prueba->cpu_registers->normales[AX]);
            log_info(logger, "BX: %i", pcb_prueba->cpu_registers->normales[BX]);

            free(pcb_prueba->cpu_registers);
            free(pcb_prueba);
        }
        // while(1){}
        // esperar_cpu();
        break;

    case OUT_OF_MEMORY:
        log_debug(logger, "El CPU informa que, luego de pedirle un RESIZE ampliativo a la Memoria, esta le respondió que no tiene más espacio");
        t_pcb *pcb_out_of_memory = recibir_pcb(socket_cpu_dispatch);
        log_info(logger, "Finaliza el proceso %i - Motivo: OUT_OF_MEMORY", pcb_out_of_memory->pid);
        eliminar_proceso(pcb_out_of_memory);
        break;
    case IO_GEN_SLEEP:
        log_debug(logger, "El CPU pidió un sleep");
        t_sleep *sleep = recibir_sleep(socket_cpu_dispatch);

        // Hay que mandarle a la interfaz generica el sleep que llega de CPU
        // Vamos a recibir un paquete desde el dispatch proveniente de CPU que tiene la info de IO_GEN_SLEEP

        t_interfaz *interfaz_sleep = NULL;
        // Buscamos la interfaz por su nombre
        for (int i = 0; i < list_size(lista_interfaces); i++)
        {
            t_interfaz *interfaz_en_lista = (t_interfaz *)list_get(lista_interfaces, i);
            if (strcmp(sleep->nombre_interfaz, interfaz_en_lista->nombre) == 0)
            {
                interfaz_sleep = list_get(lista_interfaces, i);
            }
        }

        // Si la interfaz no existe mandamos el proceso a EXIT
        if (interfaz_sleep == NULL)
        {
            log_info(logger, "Finaliza el proceso %i - Motivo: INVALID_INTERFACE", sleep->pcb->pid);
            eliminar_proceso(sleep->pcb);
        }

        // Si la interfaz no es del tipo "Interfaz Genérica" mandamos el proceso a EXIT
        else if (interfaz_sleep->tipo != GENERICA)
        {
            log_info(logger, "Finaliza el proceso %i - Motivo: INVALID_INTERFACE", sleep->pcb->pid);
            eliminar_proceso(sleep->pcb);
        }

        else
        {
            sleep->pcb->estado = BLOCKED;
            list_add(lista_bloqueados, sleep->pcb);
            log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCKED", sleep->pcb->pid);
            log_info(logger, "PID: %i - Bloqueado por: %s", sleep->pcb->pid, interfaz_sleep->nombre);
            if (strcmp(algoritmo_planificacion, "VRR") == 0)
            {
                sem_post(&sem_vrr_block);
            }
            // Verficamos si la interfaz está ocupada
            if (!interfaz_sleep->ocupada)
            {
                enviar_sleep(interfaz_sleep->socket, sleep->pcb, sleep->nombre_interfaz, sleep->unidades_de_trabajo);
                interfaz_sleep->ocupada = true;
            }
            else
            {
                queue_push(interfaz_sleep->cola_procesos_esperando, sleep);
                log_debug(logger, "La interfaz estaba ocupada y el proceso entró en la cola espera");
            }
        }

        // free(sleep->pcb->cpu_registers);
        // free(sleep->pcb);
        // free(sleep);


        break;
    case IO_STDIN_READ:
        // t_io_std *io_stdin_read = recibir_io_std(socket_cpu_dispatch);

        // log_debug(logger, "PID: %i", io_stdin_read->pcb->pid);
        // log_debug(logger, "program counter: %i", io_stdin_read->pcb->cpu_registers->pc);
        // log_debug(logger, "quantum: %i", io_stdin_read->pcb->quantum);
        // log_debug(logger, "estado: %i", io_stdin_read->pcb->estado);
        // log_debug(logger, "SI: %i", io_stdin_read->pcb->cpu_registers->si);
        // log_debug(logger, "DI: %i", io_stdin_read->pcb->cpu_registers->di);
        // log_debug(logger, "AX: %i", io_stdin_read->pcb->cpu_registers->normales[AX]);
        // log_debug(logger, "BX: %i", io_stdin_read->pcb->cpu_registers->normales[BX]);
        // log_debug(logger, "CX: %i", io_stdin_read->pcb->cpu_registers->normales[CX]);
        // log_debug(logger, "DX: %i", io_stdin_read->pcb->cpu_registers->normales[DX]);
        // log_debug(logger, "EAX: %i", io_stdin_read->pcb->cpu_registers->extendidos[EAX]);
        // log_debug(logger, "EBX: %i", io_stdin_read->pcb->cpu_registers->extendidos[EBX]);
        // log_debug(logger, "ECX: %i", io_stdin_read->pcb->cpu_registers->extendidos[ECX]);
        // log_debug(logger, "EDX: %i", io_stdin_read->pcb->cpu_registers->extendidos[EDX]);

        // log_debug(logger, "Nombre de la interfaz: %s", io_stdin_read->nombre_interfaz);
        
        // log_debug(logger, "Tamaño de la interfaz: %i", io_stdin_read->tamanio_nombre_interfaz);

        // log_debug(logger, "Tamaño del contenido: %i", io_stdin_read->tamanio_contenido);

        // t_direccion_y_tamanio *primera_direccion = list_get(io_stdin_read->direcciones_fisicas, 0);
        // t_direccion_y_tamanio *segunda_direccion = list_get(io_stdin_read->direcciones_fisicas, 1);
        // t_direccion_y_tamanio *tercera_direccion = list_get(io_stdin_read->direcciones_fisicas, 2);

        // log_debug(logger, "Primera dirección: %i; Tamaño: %i", primera_direccion->direccion, primera_direccion->tamanio);
        // log_debug(logger, "Segunda dirección: %i; Tamaño: %i", segunda_direccion->direccion, segunda_direccion->tamanio);
        // log_debug(logger, "Segunda dirección: %i; Tamaño: %i", tercera_direccion->direccion, tercera_direccion->tamanio);

        // while(1);
        pedido_io_stdin_read();
        break;
    case IO_STDOUT_WRITE:
        pedido_io_stdout_write();
        break;
    case INSTRUCCION_EXIT:
        log_debug(logger, "El CPU informa que le llegó una instrucción EXIT");
        t_pcb *pcb_exit = recibir_pcb(socket_cpu_dispatch);
        log_info(logger, "Finaliza el proceso %i - Motivo: SUCCESS", pcb_exit->pid);
        eliminar_proceso(pcb_exit);
        break;
    case INTERRUPCION:
        log_debug(logger, "Al CPU el llegó una interrupción, así que envía el pcb junto con el motivo de la interrupción");
        t_interrupcion *interrupcion = recibir_interrupcion(socket_cpu_dispatch);

        // después se puede cambiar este if por un switch cuando haya varios motivos de interrupción
        if (interrupcion->motivo == FINALIZAR_PROCESO)
        {
            log_debug(logger, "Este motivo es FINALIZAR_PROCESO, entonces se elimina el proceso");
            eliminar_proceso(interrupcion->pcb);
            log_info(logger, "Finaliza el proceso %i - Motivo: INTERRUPTED_BY_USER", interrupcion->pcb->pid);
        }
        if (interrupcion->motivo == FIN_DE_QUANTUM)
        {
            if (strcmp(algoritmo_planificacion, "RR") == 0)
            {
                log_debug(logger, "Este motivo es FIN_DE_QUANTUM con RR, entonces se agrega al final de la cola el proceso");
                log_info(logger, "PID: %i - Desalojado por fin de Quantum", interrupcion->pcb->pid);
                interrupcion->pcb->estado = READY;
                queue_push(cola_ready, interrupcion->pcb);
                sem_post(&sem_proceso_ready);
            }
            if (strcmp(algoritmo_planificacion, "VRR") == 0)
            {
                log_debug(logger, "Este motivo es FIN_DE_QUANTUM con VRR, entonces se agrega al final de la cola el proceso");
                interrupcion->pcb->estado = READY;
                queue_push(cola_ready, interrupcion->pcb);
                sem_post(&sem_proceso_ready);
            }
        }
        break;
    case WAIT:
        log_debug(logger, "El CPU informa que le llegó una instrucción WAIT");
        t_recurso *recurso_wait = recibir_recurso(socket_cpu_dispatch);
        t_manejo_de_recurso *recurso_a_asignar = NULL;
        for (int i = 0; i < list_size(lista_recursos); i++)
        {
            t_manejo_de_recurso *recurso_buscado = (t_manejo_de_recurso *)list_get(lista_recursos, i);
            if (strcmp(recurso_buscado->nombre, recurso_wait->nombre) == 0)
            {
                recurso_a_asignar = recurso_buscado;
            }
        }

        if (recurso_a_asignar == NULL)
        {
            log_info(logger, "Finaliza el proceso %i - Motivo: INVALID_RESOURCE", recurso_wait->pcb->pid);
            proceso_en_ejecucion = false;
            eliminar_proceso(recurso_wait->pcb);
        }
        else
        {
            if (recurso_a_asignar->instancias > 0)
            {
                // Al proceso se le asigna una instancia
                int ya_tiene_una_instancia = -1;
                for (int i = 0; i < list_size(recurso_a_asignar->procesos_asignados); i++)
                {
                    t_instancias_por_procesos *instancias_por_proceso =
                        (t_instancias_por_procesos *)list_get(recurso_a_asignar->procesos_asignados, i);
                    if (instancias_por_proceso->pid == recurso_wait->pcb->pid)
                    {
                        ya_tiene_una_instancia = i;
                        instancias_por_proceso->instancias++;
                    }
                }
                if (ya_tiene_una_instancia == -1)
                {
                    t_instancias_por_procesos *instancias_proceso = malloc(sizeof(t_instancias_por_procesos));
                    instancias_proceso->pid = recurso_wait->pcb->pid;
                    instancias_proceso->instancias = 1;
                    list_add(recurso_a_asignar->procesos_asignados, instancias_proceso);
                }

                // Se le quita una instanica disponible al recurso
                recurso_a_asignar->instancias--;
                enviar_pcb(socket_cpu_dispatch, recurso_wait->pcb);
                esperar_cpu();
            }
            else
            {
                log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCKED", recurso_wait->pcb->pid);
                log_info(logger, "PID: %i - Bloqueado por: %s", recurso_wait->pcb->pid, recurso_wait->nombre);
                recurso_wait->pcb->estado = BLOCKED;
                proceso_en_ejecucion = false;
                list_add(lista_bloqueados, recurso_wait->pcb);
                t_numero *pid = malloc(sizeof(t_numero));
                pid->numero = recurso_wait->pcb->pid;
                queue_push(recurso_a_asignar->procesos_esperando, pid);
            }
        }

        break;
    case SIGNAL:
        log_debug(logger, "El CPU informa que le llegó una instrucción SIGNAL");
        t_recurso *recurso_signal = recibir_recurso(socket_cpu_dispatch);
        t_manejo_de_recurso *recurso_a_liberar = NULL;
        for (int i = 0; i < list_size(lista_recursos); i++)
        {
            t_manejo_de_recurso *recurso_buscado = (t_manejo_de_recurso *)list_get(lista_recursos, i);
            if (strcmp(recurso_buscado->nombre, recurso_signal->nombre) == 0)
            {
                recurso_a_liberar = recurso_buscado;
            }
        }

        if (recurso_a_liberar == NULL)
        {
            log_info(logger, "Finaliza el proceso %i - Motivo: INVALID_RESOURCE", recurso_signal->pcb->pid);
            proceso_en_ejecucion = false;
            eliminar_proceso(recurso_signal->pcb);
        }
        else
        {
            // Al proceso que hizo el signal se le quita una instancia
            for (int i = 0; i < list_size(recurso_a_liberar->procesos_asignados); i++)
            {
                t_instancias_por_procesos *instancias_por_pid =
                    (t_instancias_por_procesos *)list_get(recurso_a_liberar->procesos_asignados, i);
                if (instancias_por_pid->pid == recurso_signal->pcb->pid)
                {
                    instancias_por_pid->instancias--;
                    if (instancias_por_pid->instancias == 0)
                    {
                        list_remove_and_destroy_element(recurso_a_liberar->procesos_asignados, i, free);
                    }
                }
            }

            if (list_is_empty(recurso_a_liberar->procesos_esperando->elements))
            {
                // Se le agrega una instanica disponible al recurso
                recurso_a_liberar->instancias++;
                enviar_pcb(socket_cpu_dispatch, recurso_signal->pcb);
                esperar_cpu();
            }
            else
            {
                t_numero *proceso = queue_pop(recurso_a_liberar->procesos_esperando);

                // Al proceso que estaba esperando se le asigna una instancia
                int ya_tiene_una_instancia = -1;
                for (int i = 0; i < list_size(recurso_a_liberar->procesos_asignados); i++)
                {
                    t_instancias_por_procesos *instancias_por_proceso =
                        (t_instancias_por_procesos *)list_get(recurso_a_liberar->procesos_asignados, i);
                    if (instancias_por_proceso->pid == proceso->numero)
                    {
                        ya_tiene_una_instancia = i;
                        instancias_por_proceso->instancias++;
                    }
                }
                if (ya_tiene_una_instancia == -1)
                {
                    t_instancias_por_procesos *instancias_proceso = malloc(sizeof(t_instancias_por_procesos));
                    instancias_proceso->pid = proceso->numero;
                    instancias_proceso->instancias = 1;
                    list_add(recurso_a_liberar->procesos_asignados, instancias_proceso);
                }

                // El proceso que estaba esperando deja de estar bloqueado y entra en la cola de READY
                for (int i = 0; i < list_size(lista_bloqueados); i++)
                {
                    t_pcb *pcb = (t_pcb *)list_get(lista_bloqueados, i);
                    if (pcb->pid == proceso->numero)
                    {
                        pcb->estado = READY;
                        list_remove(lista_bloqueados, i);
                        queue_push(cola_ready, pcb);
                        log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", pcb->pid);
                        mostrar_cola_ready();
                        sem_post(&sem_proceso_ready);
                    }
                }
                free(proceso);
                enviar_pcb(socket_cpu_dispatch, recurso_signal->pcb);
                esperar_cpu();
            }
        }

        break;
    case IO_FS_CREATE:
        pedido_io_fs_create();
        break;
    case IO_FS_DELETE:
        pedido_io_fs_delete();
        break;
    case IO_FS_TRUNCATE:
        pedido_io_fs_truncate();
        break;
    case IO_FS_WRITE:
        pedido_io_fs_write();
        break;
    case IO_FS_READ:
        pedido_io_fs_read();
        break;
    default:
        log_warning(logger, "Mensaje desconocido del CPU: %i", cod_op);
        break;
    }
}
void pedido_io_stdin_read()
{
    log_debug(logger, "El CPU pidió un IO_STDIN_READ");

    t_io_std *io_stdin_read = recibir_io_std(socket_cpu_dispatch);

    t_interfaz *interfaz_stdin_read = NULL;

    // Buscamos la interfaz por su nombre
    for (int i = 0; i < list_size(lista_interfaces); i++)
    {
        t_interfaz *interfaz_en_lista = list_get(lista_interfaces, i);
        if (strcmp(io_stdin_read->nombre_interfaz, interfaz_en_lista->nombre) == 0)
        {
            interfaz_stdin_read = list_get(lista_interfaces, i);
        }
    }

    // Si la interfaz no existe mandamos el proceso a EXIT
    if (interfaz_stdin_read == NULL)
    {
        log_info(logger, "Finaliza el proceso %i - Motivo: INVALID_INTERFACE", io_stdin_read->pcb->pid);
        eliminar_proceso(io_stdin_read->pcb);
    }

    // Si la interfaz no es del tipo "STDIN" mandamos el proceso a EXIT
    else if (interfaz_stdin_read->tipo != STDIN)
    {
        log_info(logger, "Finaliza el proceso %i - Motivo: INVALID_INTERFACE", io_stdin_read->pcb->pid);
        eliminar_proceso(io_stdin_read->pcb);
    }

    else
    {
        io_stdin_read->pcb->estado = BLOCKED;
        list_add(lista_bloqueados, io_stdin_read->pcb);
        log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCKED", io_stdin_read->pcb->pid);
        log_info(logger, "PID: %i - Bloqueado por: %s", io_stdin_read->pcb->pid, interfaz_stdin_read->nombre);

        if (strcmp(algoritmo_planificacion, "VRR") == 0)
        {
            sem_post(&sem_vrr_block);
        }

        // Verificamos si la interfaz está ocupada
        if (!interfaz_stdin_read->ocupada)
        {
            // Enviar la solicitud de IO_STDIN_READ a la interfaz
            enviar_io_stdin_read(interfaz_stdin_read->socket, io_stdin_read);
            interfaz_stdin_read->ocupada = true;
        }
        else
        {
            queue_push(interfaz_stdin_read->cola_procesos_esperando, io_stdin_read);
            log_debug(logger, "La interfaz estaba ocupada y el proceso entró en la cola espera");
        }
    }

    // Liberar memoria de la estructura t_io_std
    // free(io_stdin_read->nombre_interfaz);
    // free(io_stdin_read->pcb->cpu_registers);
    // free(io_stdin_read->pcb);
    // free(io_stdin_read);
}

void pedido_io_stdout_write() {
    log_debug(logger, "El CPU pidió un IO_STDOUT_WRITE");

    t_io_std *io_stdout_write = recibir_io_std(socket_cpu_dispatch);

    t_interfaz *interfaz_stdout_write = NULL;

    // Buscamos la interfaz por su nombre
    for (int i = 0; i < list_size(lista_interfaces); i++) {
        t_interfaz *interfaz_en_lista = list_get(lista_interfaces, i);
        if (strcmp(io_stdout_write->nombre_interfaz, interfaz_en_lista->nombre) == 0) {
            interfaz_stdout_write = list_get(lista_interfaces, i);
        }
    }

    // Si la interfaz no existe mandamos el proceso a EXIT
    if (interfaz_stdout_write == NULL) {
        log_info(logger, "Finaliza el proceso %i - Motivo: INVALID_INTERFACE", io_stdout_write->pcb->pid);
        eliminar_proceso(io_stdout_write->pcb);
    }

    // Si la interfaz no es del tipo "STDOUT" mandamos el proceso a EXIT
    else if (interfaz_stdout_write->tipo != STDOUT) {
        log_info(logger, "Finaliza el proceso %i - Motivo: INVALID_INTERFACE", io_stdout_write->pcb->pid);
        eliminar_proceso(io_stdout_write->pcb);
    }

    else
    {
        io_stdout_write->pcb->estado = BLOCKED;
        list_add(lista_bloqueados, io_stdout_write->pcb);
        log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCKED", io_stdout_write->pcb->pid);
        log_info(logger, "PID: %i - Bloqueado por: %s", io_stdout_write->pcb->pid, interfaz_stdout_write->nombre);

        if (strcmp(algoritmo_planificacion, "VRR") == 0) {
            sem_post(&sem_vrr_block);
        }

        // Verificamos si la interfaz está ocupada
        if (!interfaz_stdout_write->ocupada) {
            // Enviar la solicitud de IO_STDOUT_WRITE a la interfaz
            enviar_io_stdout_write(interfaz_stdout_write->socket, io_stdout_write);
            interfaz_stdout_write->ocupada = true;
        }
        else
        {
            queue_push(interfaz_stdout_write->cola_procesos_esperando, io_stdout_write);
            log_debug(logger, "La interfaz estaba ocupada y el proceso entró en la cola espera");
        }
    }

    // Liberar memoria de la estructura t_io_stdout_write
    // free(io_stdout_write->nombre_interfaz);
    // free(io_stdout_write->pcb->cpu_registers);
    // free(io_stdout_write->pcb);
    // free(io_stdout_write);
}

void pedido_io_fs_create() {
    log_debug(logger, "El CPU pidió un IO_FS_CREATE");

    t_io_fs_archivo *io_fs_create = recibir_io_fs_archivo(socket_cpu_dispatch);

    t_interfaz *interfaz_fs_create = NULL;

    // Buscamos la interfaz por su nombre
    for (int i = 0; i < list_size(lista_interfaces); i++) {
        t_interfaz *interfaz_en_lista = list_get(lista_interfaces, i);
        if (strcmp(io_fs_create->nombre_interfaz, interfaz_en_lista->nombre) == 0) {
            interfaz_fs_create = interfaz_en_lista;
        }
    }

    // Si la interfaz no existe mandamos el proceso a EXIT
    if (interfaz_fs_create == NULL) {
        log_info(logger, "Finaliza el proceso %i - Motivo: INVALID_INTERFACE", io_fs_create->pcb->pid);
        eliminar_proceso(io_fs_create->pcb);
    }

    // Si la interfaz no es del tipo "DialFS" mandamos el proceso a EXIT
    else if (interfaz_fs_create->tipo != DialFS) {
        log_info(logger, "Finaliza el proceso %i - Motivo: INVALID_INTERFACE", io_fs_create->pcb->pid);
        eliminar_proceso(io_fs_create->pcb);
    }

    else
    {
        io_fs_create->pcb->estado = BLOCKED;
        list_add(lista_bloqueados, io_fs_create->pcb);
        log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCKED", io_fs_create->pcb->pid);
        log_info(logger, "PID: %i - Bloqueado por: %s", io_fs_create->pcb->pid, interfaz_fs_create->nombre);

        if (strcmp(algoritmo_planificacion, "VRR") == 0) {
            sem_post(&sem_vrr_block);
        }

        // Verificamos si la interfaz está ocupada
        if (!interfaz_fs_create->ocupada) {
            // Enviar la solicitud de IO_FS_CREATE a la interfaz
            enviar_io_fs_create(interfaz_fs_create->socket, io_fs_create);
            interfaz_fs_create->ocupada = true;
        } else {
            t_io_fs_comodin *comodin = malloc(sizeof(t_io_fs_comodin));
            comodin->operacion = CREATE;
            comodin->pcb = io_fs_create->pcb;
            comodin->nombre_interfaz = io_fs_create->nombre_interfaz;
            comodin->tamanio_nombre_interfaz = io_fs_create->tamanio_nombre_interfaz;
            comodin->nombre_archivo = io_fs_create->nombre_archivo;
            comodin->tamanio_nombre_archivo = io_fs_create->tamanio_nombre_archivo;
            comodin->tamanio = 0;
            comodin->puntero_archivo = 0;
            comodin->direcciones_fisicas = list_create();
            free(io_fs_create);
            queue_push(interfaz_fs_create->cola_procesos_esperando, comodin);
            log_debug(logger, "La interfaz estaba ocupada y el proceso entró en la cola espera");
        }
    }

    // Liberar memoria de la estructura t_io_fs_archivo
    // free(io_fs_create->nombre_interfaz);
    // free(io_fs_create->nombre_archivo);
    // free(io_fs_create->pcb->cpu_registers);
    // free(io_fs_create->pcb);
    // free(io_fs_create);
}

void pedido_io_fs_delete() {
    log_debug(logger, "El CPU pidió un IO_FS_DELETE");

    t_io_fs_archivo *io_fs_delete = recibir_io_fs_archivo(socket_cpu_dispatch);

    t_interfaz *interfaz_fs_delete = NULL;

    // Buscar la interfaz por su nombre
    for (int i = 0; i < list_size(lista_interfaces); i++) {
        t_interfaz *interfaz_en_lista = list_get(lista_interfaces, i);
        if (strcmp(io_fs_delete->nombre_interfaz, interfaz_en_lista->nombre) == 0) {
            interfaz_fs_delete = interfaz_en_lista;
        }
    }

    // Si la interfaz no existe, mandar el proceso a EXIT
    if (interfaz_fs_delete == NULL) {
        log_info(logger, "Finaliza el proceso %i - Motivo: INVALID_INTERFACE", io_fs_delete->pcb->pid);
        eliminar_proceso(io_fs_delete->pcb);
        return;
    }

    // Si la interfaz no es del tipo "DialFS", mandar el proceso a EXIT
    if (interfaz_fs_delete->tipo != DialFS) {
        log_info(logger, "Finaliza el proceso %i - Motivo: INVALID_INTERFACE", io_fs_delete->pcb->pid);
        eliminar_proceso(io_fs_delete->pcb);
        return;
    }

    io_fs_delete->pcb->estado = BLOCKED;
    list_add(lista_bloqueados, io_fs_delete->pcb);
    log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCKED", io_fs_delete->pcb->pid);
    log_info(logger, "PID: %i - Bloqueado por: %s", io_fs_delete->pcb->pid, interfaz_fs_delete->nombre);

    if (strcmp(algoritmo_planificacion, "VRR") == 0) {
        sem_post(&sem_vrr_block);
    }

    // Verificar si la interfaz está ocupada
    if (!interfaz_fs_delete->ocupada) {
        // Enviar la solicitud de IO_FS_DELETE a la interfaz
        enviar_io_fs_delete(interfaz_fs_delete->socket, io_fs_delete);
        interfaz_fs_delete->ocupada = true;
    } else {
        t_io_fs_comodin *comodin = malloc(sizeof(t_io_fs_comodin));
        comodin->operacion = DELETE;
        comodin->pcb = io_fs_delete->pcb;
        comodin->nombre_interfaz = io_fs_delete->nombre_interfaz;
        comodin->tamanio_nombre_interfaz = io_fs_delete->tamanio_nombre_interfaz;
        comodin->nombre_archivo = io_fs_delete->nombre_archivo;
        comodin->tamanio_nombre_archivo = io_fs_delete->tamanio_nombre_archivo;
        comodin->tamanio = 0;
        comodin->puntero_archivo = 0;
        comodin->direcciones_fisicas = list_create();
        free(io_fs_delete);
        queue_push(interfaz_fs_delete->cola_procesos_esperando, comodin);
        log_debug(logger, "La interfaz estaba ocupada y el proceso entró en la cola espera");
    }

    // Liberar memoria de la estructura t_io_fs_delete
    // free(io_fs_delete->nombre_interfaz);
    // free(io_fs_delete->nombre_archivo);
    // free(io_fs_delete->pcb->cpu_registers);
    // free(io_fs_delete->pcb);
    // free(io_fs_delete);
}

void pedido_io_fs_truncate() {
    log_debug(logger, "El CPU pidió un IO_FS_TRUNCATE");

    t_io_fs_truncate *io_fs_truncate = recibir_io_fs_truncate(socket_cpu_dispatch);

    t_interfaz *interfaz_fs_truncate = NULL;

    // Buscar la interfaz por su nombre
    for (int i = 0; i < list_size(lista_interfaces); i++) {
        t_interfaz *interfaz_en_lista = list_get(lista_interfaces, i);
        if (strcmp(io_fs_truncate->nombre_interfaz, interfaz_en_lista->nombre) == 0) {
            interfaz_fs_truncate = interfaz_en_lista;
        }
    }

    // Si la interfaz no existe, mandar el proceso a EXIT
    if (interfaz_fs_truncate == NULL) {
        log_info(logger, "Finaliza el proceso %i - Motivo: INVALID_INTERFACE", io_fs_truncate->pcb->pid);
        eliminar_proceso(io_fs_truncate->pcb);
        return;
    }

    // Si la interfaz no es del tipo "DialFS", mandar el proceso a EXIT
    if (interfaz_fs_truncate->tipo != DialFS) {
        log_info(logger, "Finaliza el proceso %i - Motivo: INVALID_INTERFACE", io_fs_truncate->pcb->pid);
        eliminar_proceso(io_fs_truncate->pcb);
        return;
    }

    io_fs_truncate->pcb->estado = BLOCKED;
    list_add(lista_bloqueados, io_fs_truncate->pcb);
    log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCKED", io_fs_truncate->pcb->pid);
    log_info(logger, "PID: %i - Bloqueado por: %s", io_fs_truncate->pcb->pid, interfaz_fs_truncate->nombre);

    if (strcmp(algoritmo_planificacion, "VRR") == 0) {
        sem_post(&sem_vrr_block);
    }
    
    // Verificar si la interfaz está ocupada
    if (!interfaz_fs_truncate->ocupada) {
        // Enviar la solicitud de IO_FS_TRUNCATE a la interfaz
        enviar_io_fs_truncate(interfaz_fs_truncate->socket, io_fs_truncate);
        interfaz_fs_truncate->ocupada = true;
    } else {
        t_io_fs_comodin *comodin = malloc(sizeof(t_io_fs_comodin));
        comodin->operacion = TRUNCATE;
        comodin->pcb = io_fs_truncate->pcb;
        comodin->nombre_interfaz = io_fs_truncate->nombre_interfaz;
        comodin->tamanio_nombre_interfaz = io_fs_truncate->tamanio_nombre_interfaz;
        comodin->nombre_archivo = io_fs_truncate->nombre_archivo;
        comodin->tamanio_nombre_archivo = io_fs_truncate->tamanio_nombre_archivo;
        comodin->tamanio = io_fs_truncate->nuevo_tamanio;
        comodin->puntero_archivo = 0;
        comodin->direcciones_fisicas = list_create();
        free(io_fs_truncate);
        queue_push(interfaz_fs_truncate->cola_procesos_esperando, comodin);
        log_debug(logger, "La interfaz estaba ocupada y el proceso entró en la cola espera");
    }

    // Liberar memoria de la estructura t_io_fs_truncate
    // free(io_fs_truncate->nombre_interfaz);
    // free(io_fs_truncate->nombre_archivo);
    // free(io_fs_truncate->pcb->cpu_registers);
    // free(io_fs_truncate->pcb);
    // free(io_fs_truncate);
}

void pedido_io_fs_write() {
    log_debug(logger, "El CPU pidió un IO_FS_WRITE");

    t_io_fs_rw *io_fs_write = recibir_io_fs_rw(socket_cpu_dispatch);

    t_interfaz *interfaz_fs_write = NULL;

    // Buscamos la interfaz por su nombre
    for (int i = 0; i < list_size(lista_interfaces); i++) {
        t_interfaz *interfaz_en_lista = list_get(lista_interfaces, i);
        if (strcmp(io_fs_write->nombre_interfaz, interfaz_en_lista->nombre) == 0) {
            interfaz_fs_write = interfaz_en_lista;
        }
    }

    // Si la interfaz no existe mandamos el proceso a EXIT
    if (interfaz_fs_write == NULL) {
        log_info(logger, "Finaliza el proceso %i - Motivo: INVALID_INTERFACE", io_fs_write->pcb->pid);
        eliminar_proceso(io_fs_write->pcb);
        return;
    }

    // Si la interfaz no es del tipo "DialFS", mandamos el proceso a EXIT
    if (interfaz_fs_write->tipo != DialFS) {
        log_info(logger, "Finaliza el proceso %i - Motivo: INVALID_INTERFACE", io_fs_write->pcb->pid);
        eliminar_proceso(io_fs_write->pcb);
        return;
    }

    io_fs_write->pcb->estado = BLOCKED;
    list_add(lista_bloqueados, io_fs_write->pcb);
    log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCKED", io_fs_write->pcb->pid);
    log_info(logger, "PID: %i - Bloqueado por: %s", io_fs_write->pcb->pid, interfaz_fs_write->nombre);

    if (strcmp(algoritmo_planificacion, "VRR") == 0) {
        sem_post(&sem_vrr_block);
    }

    // Verificamos si la interfaz está ocupada
    if (!interfaz_fs_write->ocupada) {
        // Enviar la solicitud de IO_FS_WRITE a la interfaz
        enviar_io_fs_write(interfaz_fs_write->socket, io_fs_write);
        interfaz_fs_write->ocupada = true;
    } else {
        t_io_fs_comodin *comodin = malloc(sizeof(t_io_fs_comodin));
        comodin->operacion = WRITE;
        comodin->pcb = io_fs_write->pcb;
        comodin->nombre_interfaz = io_fs_write->nombre_interfaz;
        comodin->tamanio_nombre_interfaz = io_fs_write->tamanio_nombre_interfaz;
        comodin->nombre_archivo = io_fs_write->nombre_archivo;
        comodin->tamanio_nombre_archivo = io_fs_write->tamanio_nombre_archivo;
        comodin->tamanio = io_fs_write->tamanio;
        comodin->puntero_archivo = io_fs_write->puntero_archivo;
        comodin->direcciones_fisicas = io_fs_write->direcciones_fisicas;
        free(io_fs_write);
        queue_push(interfaz_fs_write->cola_procesos_esperando, comodin);
        log_debug(logger, "La interfaz estaba ocupada y el proceso entró en la cola espera");
    }

    // Liberar memoria de la estructura t_io_fs_rw
    // free(io_fs_write->nombre_interfaz);
    // free(io_fs_write->nombre_archivo);
    // free(io_fs_write->pcb->cpu_registers);
    // free(io_fs_write->pcb);
    // free(io_fs_write);
}


void pedido_io_fs_read() {
    log_debug(logger, "El CPU pidió un IO_FS_READ");

    t_io_fs_rw *io_fs_read = recibir_io_fs_rw(socket_cpu_dispatch);

    t_interfaz *interfaz_fs_read = NULL;

    // Buscar la interfaz por su nombre
    for (int i = 0; i < list_size(lista_interfaces); i++) {
        t_interfaz *interfaz_en_lista = list_get(lista_interfaces, i);
        if (strcmp(io_fs_read->nombre_interfaz, interfaz_en_lista->nombre) == 0) {
            interfaz_fs_read = interfaz_en_lista;
        }
    }

    // Si la interfaz no existe, mandar el proceso a EXIT
    if (interfaz_fs_read == NULL) {
        log_info(logger, "Finaliza el proceso %i - Motivo: INVALID_INTERFACE", io_fs_read->pcb->pid);
        eliminar_proceso(io_fs_read->pcb);
        return;
    }

    // Si la interfaz no es del tipo "DialFS", mandar el proceso a EXIT
    if (interfaz_fs_read->tipo != DialFS) {
        log_info(logger, "Finaliza el proceso %i - Motivo: INVALID_INTERFACE", io_fs_read->pcb->pid);
        eliminar_proceso(io_fs_read->pcb);
        return;
    }

    io_fs_read->pcb->estado = BLOCKED;
    list_add(lista_bloqueados, io_fs_read->pcb);
    log_info(logger, "PID: %i - Estado Anterior: EXEC - Estado Actual: BLOCKED", io_fs_read->pcb->pid);
    log_info(logger, "PID: %i - Bloqueado por: %s", io_fs_read->pcb->pid, interfaz_fs_read->nombre);

    if (strcmp(algoritmo_planificacion, "VRR") == 0) {
        sem_post(&sem_vrr_block);
    }

    // Verificar si la interfaz está ocupada
    if (!interfaz_fs_read->ocupada) {
        // Enviar la solicitud de IO_FS_READ a la interfaz
        enviar_io_fs_read(interfaz_fs_read->socket, io_fs_read);
        interfaz_fs_read->ocupada = true;
    } else {
        t_io_fs_comodin *comodin = malloc(sizeof(t_io_fs_comodin));
        comodin->operacion = READ;
        comodin->pcb = io_fs_read->pcb;
        comodin->nombre_interfaz = io_fs_read->nombre_interfaz;
        comodin->tamanio_nombre_interfaz = io_fs_read->tamanio_nombre_interfaz;
        comodin->nombre_archivo = io_fs_read->nombre_archivo;
        comodin->tamanio_nombre_archivo = io_fs_read->tamanio_nombre_archivo;
        comodin->tamanio = io_fs_read->tamanio;
        comodin->puntero_archivo = io_fs_read->puntero_archivo;
        comodin->direcciones_fisicas = io_fs_read->direcciones_fisicas;
        free(io_fs_read);
        queue_push(interfaz_fs_read->cola_procesos_esperando, comodin);
        log_debug(logger, "La interfaz estaba ocupada y el proceso entró en la cola espera");
    }

    // Liberar memoria de la estructura t_io_fs_read
    // free(io_fs_read->nombre_interfaz);
    // free(io_fs_read->nombre_archivo);
    // free(io_fs_read->pcb->cpu_registers);
    // free(io_fs_read->pcb);
    // free(io_fs_read);
}

void bloquear_proceso(t_pcb *pcb)
{

    pcb->estado = BLOCKED;
    list_add(lista_bloqueados, pcb);
}

void desbloquear_proceso(t_pcb *pcb_a_desbloquear)
{
    for (int i = 0; i < list_size(lista_bloqueados); i++)
    {
        t_pcb *pcb = (t_pcb *)list_get(lista_bloqueados, i);
        if (pcb->pid == pcb_a_desbloquear->pid)
        {
            list_remove(lista_bloqueados, i);
            pcb->estado = READY;
            sem_post(&sem_proceso_ready);
        }
    }
}

void eliminar_proceso(t_pcb *pcb)
{
    sem_wait(&mutex_memoria);
    enviar_finalizacion_proceso(socket_memoria, pcb);

    op_code cod_op = recibir_operacion(socket_memoria);
    if (cod_op != FINALIZACION_PROCESO_OK)
    {
        log_error(logger, "El Kernel esperaba recibir un OK de la memoria (luego de pedirle finalizar un proceso) pero recibió otra cosa");
    }
    recibir_ok(socket_memoria);
    sem_post(&mutex_memoria);
    log_trace(logger, "Recibí OK de la memoria luego de pedirle finalizar un proceso");

    if (pcb_ejecutandose->pid == pcb->pid)
    {
        free(pcb_ejecutandose->cpu_registers);
        free(pcb_ejecutandose);
    }

    // Se liberan los recursos que tiene asignado el proceso, si los hubiere
    for (int i = 0; i < list_size(lista_recursos); i++)
    {
        bool el_recurso_tiene_nuevas_instancias_disponibles = false;
        t_manejo_de_recurso *recurso_buscado = (t_manejo_de_recurso *)list_get(lista_recursos, i);
        for (int j = 0; j < list_size(recurso_buscado->procesos_esperando->elements); j++)
        {
            t_numero *pid_esperando_recurso =
                (t_numero *)list_get(recurso_buscado->procesos_esperando->elements, j);
            if (pid_esperando_recurso->numero == pcb->pid)
            {
                list_remove_and_destroy_element(recurso_buscado->procesos_esperando->elements, j, free);
            }
        }
        for (int j = 0; j < list_size(recurso_buscado->procesos_asignados); j++)
        {
            t_instancias_por_procesos *instancias_por_pid =
                (t_instancias_por_procesos *)list_get(recurso_buscado->procesos_asignados, j);
            if (instancias_por_pid->pid == pcb->pid)
            {
                recurso_buscado->instancias += instancias_por_pid->instancias;
                list_remove_and_destroy_element(recurso_buscado->procesos_asignados, j, free);
                el_recurso_tiene_nuevas_instancias_disponibles = true;
            }
        }
        if(el_recurso_tiene_nuevas_instancias_disponibles && !list_is_empty(recurso_buscado->procesos_esperando->elements))
        {
            t_numero *proceso = queue_pop(recurso_buscado->procesos_esperando);

            // Al proceso que estaba esperando se le asigna una instancia
            int ya_tiene_una_instancia = -1;
            for (int j = 0; j < list_size(recurso_buscado->procesos_asignados); j++)
            {
                t_instancias_por_procesos *instancias_por_proceso =
                    (t_instancias_por_procesos *)list_get(recurso_buscado->procesos_asignados, j);
                if (instancias_por_proceso->pid == proceso->numero)
                {
                    ya_tiene_una_instancia = j;
                    instancias_por_proceso->instancias++;
                }
            }
            if (ya_tiene_una_instancia == -1)
            {
                t_instancias_por_procesos *instancias_proceso = malloc(sizeof(t_instancias_por_procesos));
                instancias_proceso->pid = proceso->numero;
                instancias_proceso->instancias = 1;
                list_add(recurso_buscado->procesos_asignados, instancias_proceso);
            }
            recurso_buscado->instancias--;

            // El proceso que estaba esperando deja de estar bloqueado y entra en la cola de READY
            for (int j = 0; j < list_size(lista_bloqueados); j++)
            {
                t_pcb *pcb = (t_pcb *)list_get(lista_bloqueados, j);
                if (pcb->pid == proceso->numero)
                {
                    pcb->estado = READY;
                    list_remove(lista_bloqueados, j);
                    queue_push(cola_ready, pcb);
                    log_info(logger, "PID: %i - Estado Anterior: BLOCKED - Estado Actual: READY", pcb->pid);
                    mostrar_cola_ready();
                    sem_post(&sem_proceso_ready);
                }
            }
            free(proceso);
        }
    }

    free(pcb->cpu_registers);
    free(pcb);

    // Se habilita un espacio en el grado de multiprogramación
    sem_wait(&mutex_cola_new);
    if (!list_is_empty(cola_new->elements) && grado_multiprogramacion_activo <= grado_multiprogramacion_max)
    {
        sem_post(&sem_multiprogramacion);
    }
    else
    {
        if(grado_multiprogramacion_activo > 0)
        {
            grado_multiprogramacion_activo--;
        }
    }
    sem_post(&mutex_cola_new);
}

void detener_planificacion()
{
    if (planificar)
    {
        planificar = false;
        sem_wait(&sem_planificacion);
    }
    else
    {
        printf("Error: La planificación ya está pausada");
    }
}

void iniciar_planificacion()
{
    if(!planificar)
    {
        planificar = true;
        sem_post(&sem_planificacion);
    }
    else
    {
        printf("Error: La planificación ya está en marcha");
    }
}

void comprobar_planificacion()
{
    // Si la planificación está pausada (con DETENER_PLANIFICACION) se "bloquea" hasta que se ponga en marcha (con INICIAR_PLANIFICACION)
    sem_wait(&sem_planificacion);
    sem_post(&sem_planificacion);
}

void modificar_grado_multiprogramacion(int nuevo_valor)
{
    sem_wait(&mutex_cola_new);
    if (nuevo_valor > grado_multiprogramacion_max && !list_is_empty(cola_new->elements))
    {
        if (nuevo_valor - grado_multiprogramacion_max < list_size(cola_new->elements))
        {
            for (int i = 0; i < (nuevo_valor - grado_multiprogramacion_max); i++)
            {
                sem_post(&sem_multiprogramacion);
            }
        }
        else
        {
            for (int i = 0; i < list_size(cola_new->elements); i++)
            {
                sem_post(&sem_multiprogramacion);
            }
        } 
    }
    sem_post(&mutex_cola_new);

    log_trace(logger, "El grado de multiprogramación anterior era %i y se cambió a %i", grado_multiprogramacion_max, nuevo_valor);
    grado_multiprogramacion_max = nuevo_valor;
}

void mostrar_cola_ready()
{
    char *pids = string_new();
    if (list_is_empty(cola_ready->elements))
    {
        string_append(&pids, "Vacía");
    }
    else
    {
        for (int i = 0; i < list_size(cola_ready->elements); i++)
        {
            t_pcb *pcb = list_get(cola_ready->elements, i);
            string_append(&pids, string_itoa(pcb->pid));
            if (i != list_size(cola_ready->elements) - 1)
            {
                string_append(&pids, "|");
            }
        }
    }
    log_info(logger, "Cola Ready: [%s]", pids);
}

void procesos_por_estado()
{
    printf("NEW | READY | BLOCKED | EXEC\n");
    
    if(list_size(cola_new->elements) > list_size(cola_ready->elements))
    {
        if(list_size(cola_new->elements) > list_size(lista_bloqueados))
        {
            listar_procesos(list_size(cola_new->elements));
        }
        else
        {
            listar_procesos(list_size(lista_bloqueados));
        }
    }
    else
    {
        if(list_size(cola_ready->elements) > list_size(lista_bloqueados))
        {
            listar_procesos(list_size(cola_ready->elements));
        }
        else
        {
            listar_procesos(list_size(lista_bloqueados));
        }
    }
}

void recursos_e_instancias()
{
    for (int i = 0; i < list_size(lista_recursos); i++)
    {
        t_manejo_de_recurso *recurso = (t_manejo_de_recurso *)list_get(lista_recursos, i);
        printf("%s: %i instancias disponibles\n", recurso->nombre, recurso->instancias);
        for (int j = 0; j < list_size(recurso->procesos_asignados); j++)
        {
            t_instancias_por_procesos *instancias_por_procesos = (t_instancias_por_procesos *)list_get(recurso->procesos_asignados, j);
            printf("    El proceso %i tiene %i instancias asignadas\n",
                instancias_por_procesos->pid, instancias_por_procesos->instancias);
        }
    }
}

void listar_procesos(int cantidad_filas)
{
    char pid_exec[10];
    if(cantidad_filas == 0)
    {
        if(proceso_en_ejecucion)
        {
            strcpy(pid_exec, string_itoa_hasta_tres_cifras(pcb_ejecutandose->pid));
            printf("    |       |         |  %s\n", pid_exec);
        }
        else
        {
            printf("    |       |         |       (No hay ningún proceso)\n");
        }
    }
    else
    {
        char pid_new[10];
        char pid_ready[10];
        char pid_blocked[10];
        for (int i = 0; i < cantidad_filas; i++)
        {
            if(list_size(cola_new->elements) > i)
            {
                t_pcb *proceso_new = list_get(cola_new->elements, i);
                strcpy(pid_new, string_itoa_hasta_tres_cifras(proceso_new->pid));
            }
            else
            {
                strcpy(pid_new, "   ");
            }

            if(list_size(cola_ready->elements) > i)
            {
                t_pcb *proceso_ready = list_get(cola_ready->elements, i);
                strcpy(pid_ready, string_itoa_hasta_tres_cifras(proceso_ready->pid));
            }
            else
            {
                strcpy(pid_ready, "   ");
            }

            if(list_size(lista_bloqueados) > i)
            {
                t_pcb *proceso_blocked = list_get(lista_bloqueados, i);
                strcpy(pid_blocked, string_itoa_hasta_tres_cifras(proceso_blocked->pid));
            }
            else
            {
                strcpy(pid_blocked, "   ");
            }

            if(i == 0 && proceso_en_ejecucion)
            {
                strcpy(pid_exec, string_itoa_hasta_tres_cifras(pcb_ejecutandose->pid));
                printf("%s |   %s |     %s |  %s\n", pid_new, pid_ready, pid_blocked, pid_exec);
            }
            else
            {
                printf("%s |   %s |     %s |     \n", pid_new, pid_ready, pid_blocked);
            }
        }
    }
}

void script(char *path)
{
    char *linea = NULL;
    size_t size = 0;
    FILE *archivo = fopen(path, "r");

    if (!archivo)
    {
        log_error(logger, "No se pudo abrir el archivo del script: %s", path);
        exit(EXIT_FAILURE);
    }

    while (getline(&linea, &size, archivo) != -1)
    {
        char comando[50];
        sscanf(linea, "%s", comando);

        if (strcmp("INICIAR_PROCESO", comando) == 0)
        {
            char path_instrucciones[300];
            sscanf(linea, "%s %s", comando, path_instrucciones);
            log_trace(logger, "El proceso que quiero ejecutar es: %s",path_instrucciones);
            crear_pcb(path_instrucciones);
        }
        else if(strcmp("FINALIZAR_PROCESO", comando) == 0)
        {
            int pid;
            sscanf(linea, "%s %i", comando, &pid);

            encontrar_y_eliminar_proceso(pid);
        }
        else if(strcmp("DETENER_PLANIFICACION", comando) == 0)
        {
            detener_planificacion();
        }
        else if(strcmp("INICIAR_PLANIFICACION", comando) == 0)
        {
            iniciar_planificacion();
        }
        else if(strcmp("MULTIPROGRAMACION", comando) == 0)
        {
            int valor;
            sscanf(linea, "%s %i", comando, &valor);

            modificar_grado_multiprogramacion(valor);
        }
        else if(strcmp("PROCESO_ESTADO", comando) == 0)
        {
            procesos_por_estado();
        }
    }

    free(linea);
    fclose(archivo);
}

/*-----------------------------CONSOLA--------------------------------------------------------------------*/

void consola()
{
    char *linea;
    while (1)
    {
        linea = readline("> ");

        char comando[30];
        sscanf(linea, "%s", comando);

        if (strcmp(comando, "COMANDOS") == 0)
        {
            printf("EJECUTAR_SCRIPT [PATH]\n");
            printf("INICIAR_PROCESO [PATH]\n");
            printf("FINALIZAR_PROCESO [PID]\n");
            printf("DETENER_PLANIFICACION\n");
            printf("INICIAR_PLANIFICACION\n");
            printf("MULTIPROGRAMACION [VALOR]\n");
            printf("PROCESO_ESTADO\n");
        }
        else if ((strcmp(comando, "EJECUTAR_SCRIPT") == 0) || (strcmp(comando, "1") == 0))
        {
            char path_script[300];
            sscanf(linea, "%s %s", comando, path_script);

            script(path_script);
        }
        else if ((strcmp(comando, "INICIAR_PROCESO") == 0) || (strcmp(comando, "2") == 0))
        {
            char path_instrucciones[300];
            sscanf(linea, "%s %s", comando, path_instrucciones);

            crear_pcb(path_instrucciones);
        }
        else if ((strcmp(comando, "FINALIZAR_PROCESO") == 0) || (strcmp(comando, "3") == 0))
        {
            int pid;
            sscanf(linea, "%s %i", comando, &pid);

            encontrar_y_eliminar_proceso(pid);
        }
        else if ((strcmp(comando, "DETENER_PLANIFICACION") == 0) || (strcmp(comando, "4") == 0))
        {
            detener_planificacion();
        }
        else if ((strcmp(comando, "INICIAR_PLANIFICACION") == 0) || (strcmp(comando, "5") == 0))
        {
            iniciar_planificacion();
        }
        else if ((strcmp(comando, "MULTIPROGRAMACION") == 0) || (strcmp(comando, "6") == 0))
        {
            int valor;
            sscanf(linea, "%s %i", comando, &valor);

            modificar_grado_multiprogramacion(valor);
        }
        else if ((strcmp(comando, "PROCESO_ESTADO") == 0) || (strcmp(comando, "7") == 0))
        {
            procesos_por_estado();
        }
        else if ((strcmp(comando, "RECURSOS") == 0) || (strcmp(comando, "8") == 0))
        {
            recursos_e_instancias();
        }
        else
        {
            printf("Comando inválido\nEscriba COMANDOS para ver los comandos disponibles\n");
        }

        free(linea);
    }
}