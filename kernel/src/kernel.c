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

t_dictionary *dic_recursos;

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

char *algoritmo_planificacion;

/*-----------------------------SEMAFOROS--------------------------------------------------------------------*/
sem_t sem_nuevo_pcb;
sem_t sem_proceso_ready;
sem_t sem_round_robin;
sem_t sem_vrr_block;

/*-----------------------------HILOS--------------------------------------------------------------------*/
pthread_t hilo_planificador_largo_plazo;
pthread_t hilo_planificador_corto_plazo;
pthread_t hilo_recibir_entradasalida;
pthread_t hilo_entradasalida[100];
pthread_t hilo_quantum_vrr;

int main(int argc, char *argv[])
{
    sem_init(&sem_nuevo_pcb, 0, 0);
    sem_init(&sem_proceso_ready, 0, 0);
    sem_init(&sem_round_robin, 0, 1);

    crear_logger();
    crear_config();

    decir_hola("Kernel");

    crear_diccionario();

    // El grado de multiprogramacion va a ser una variable global, que van a manejar entre CPU y Kernel
    grado_multiprogramacion_max = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");
    // El grado activo empieza en 0 y se ira incrementando
    grado_multiprogramacion_activo = 0;

    // Creo las colas que voy a usar para guardar mis PCBs
    cola_new = queue_create();
    cola_ready = queue_create();
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
    config = config_create("./kernel.config");
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

void crear_diccionario()
{
    char **recursos;
    char **instancias_recursos;
    recursos = config_get_array_value(config, "RECURSOS");
    instancias_recursos = config_get_array_value(config, "INSTANCIAS_RECURSOS");

    dic_recursos = dictionary_create();
    int i = 0;
    int i_rec;
    while (recursos[i] != NULL)
    {
        i_rec = atoi(instancias_recursos[i]);
        dictionary_put(dic_recursos, recursos[i], (int)i_rec);
        i++;
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

void manejo_interfaces(t_interfaz *interfaz)
{
    while (1)
    {
        switch (interfaz->tipo)
        {
        case GENERICA:
            fin_sleep(interfaz);
            break;
        case STDIN:
            log_error(logger, "Falta implementar");
            fin_io_read(interfaz);
            break;
        case STDOUT:
            log_error(logger, "Falta implementar");
            break;
        case DialFS:
            log_error(logger, "Falta implementar");
            break;
        }
    }
}

void cargar_interfaz_recibida(t_interfaz *interfaz, int socket_entradasalida, char *nombre, tipo_interfaz tipo)
{
    interfaz->socket = socket_entradasalida;
    interfaz->nombre = nombre;
    interfaz->tipo = tipo;
    interfaz->ocupada = false;
    list_add(lista_interfaces, interfaz);
}

void fin_sleep(t_interfaz *interfaz)
{
    
    op_code cod_op = recibir_operacion(interfaz->socket);
    if (cod_op == DESCONEXION)
    {
        log_warning(logger, "Se desconectó la interfaz %s", interfaz->nombre);
        // TODO: Liberar estructuras
    }
    else if (cod_op != FIN_SLEEP)
    {
        log_error(logger, "El Kernel esperaba recibir el aviso de fin de sleep pero recibió otra cosa");
    }
    desbloquear_proceso_io(interfaz);
}

void fin_io_read(t_interfaz *interfaz)
{
    op_code cod_op = recibir_operacion(interfaz->socket);
    if (cod_op == DESCONEXION)
    {
        log_warning(logger, "Se desconectó la interfaz %s", interfaz->nombre);
        // TODO: Liberar estructuras
    }
    else if (cod_op != FIN_IO_READ)
    {
        log_error(logger, "El Kernel esperaba recibir el aviso de fin de IO_READ pero recibió otra cosa");
        return; // Abortamos la función si no recibimos el código esperado
    }
    desbloquear_proceso_io(interfaz);
}
// Genero desbloquear procesos IO para no repetir codigo, los desbloqueos van a ser siempre iguales para todas las interfaces
void desbloquear_proceso_io(t_interfaz *interfaz)
{
    // La interfaz ya no está más ocupada
    interfaz->ocupada = false;
    // Recibir el PCB que se desbloqueará
    t_pcb *pcb_a_desbloquear = recibir_pcb(interfaz->socket);

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
            }

            sem_post(&sem_proceso_ready);
            interfaz->ocupada = false;
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
    log_trace(logger, "Recibí OK de la memoria luego de pedirle crear un proceso");

    // El PCB se agrega a la cola de los procesos NEW
    queue_push(cola_new, pcb);
    log_info(logger, "Se crea el proceso %d en NEW", ultimo_pid);
    log_debug(logger, "Hay %i procesos en NEW", queue_size(cola_new));
    // Despertar el mover procesos a ready
    sem_post(&sem_nuevo_pcb);
}

void encontrar_y_eliminar_proceso(int pid_a_eliminar)
{
    // Buscamos en EXEC
    if (pcb_ejecutandose->pid == pid_a_eliminar)
    {
        enviar_interrupcion(socket_cpu_interrupt, pcb_ejecutandose, FINALIZAR_PROCESO);
        return;
    }

    // Buscamos en la cola de NEW
    for (int i = 0; i < list_size(cola_new->elements); i++)
    {
        t_pcb *pcb = (t_pcb *)list_get(cola_new->elements, i);
        if (pcb->pid == pid_a_eliminar)
        {
            list_remove(cola_new->elements, i);
            eliminar_proceso(pcb);
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
            return;
        }
    }

    // TODO: Buscar también en la cola (o colas) de bloqueados
}

/*En caso de que el grado de multiprogramación lo permita, los procesos creados podrán pasar de la cola de NEW a la cola de READY,
caso contrario, se quedarán a la espera de que finalicen procesos que se encuentran en ejecución*/
void *planificador_largo_plazo()
{
    while (1)
    {
        // Esperar hasta que se cree un nuevo PCB
        log_trace(logger, "PLP: Esperando nuevo proceso");
        sem_wait(&sem_nuevo_pcb);
        log_trace(logger, "PLP: Llegó un nuevo proceso");

        // Verificar si el grado de multiprogramación lo permite
        // Mover procesos de la cola de NEW a la cola de READY
        if (grado_multiprogramacion_activo < grado_multiprogramacion_max)
        {
            log_trace(logger, "El grado de multiprogramación permite que un proceso entre a ready");
            // Tomar el primer proceso de la cola de NEW
            t_pcb *proceso_nuevo = queue_pop(cola_new);

            // Cambiar el estado del proceso a READY
            proceso_nuevo->estado = READY;

            // Agregar el proceso a la cola de READY
            queue_push(cola_ready, proceso_nuevo);
            sem_post(&sem_proceso_ready);
            log_debug(logger, "Hay %i procesos en READY", queue_size(cola_ready));
            // Aumentamos el grado de multiprogramacion activo
            // TODO poner MUTEX
            grado_multiprogramacion_activo++;

            // Reducir la cantidad de procesos en la cola de NEW
            log_trace(logger, "PLP: El proceso fue enviado a ready");
        }
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

        // Guardamos registro en el Kernel de qué proceso está ejecutándose
        pcb_ejecutandose = proceso_a_ejecutar;

        // Espero a que el CPU me devuelva el proceso
        esperar_cpu();
    }
}

// Función para planificar los procesos usando Round Robin

void planificar_round_robin()
{
    pthread_t quantum_thread;

    sem_wait(&sem_round_robin);
    // desalojo de CPU
    //  pensar si sería mejor un semáforo que controle las colas TEORÍA DE SINCRO

    // Ahora mismo, hasta que no se termine el quantum, si un proceso finaliza, el siguiente no se ejecuta.
    if (!queue_is_empty(cola_ready))
    {
        // Obtener el proceso listo para ejecutarse de la cola
        t_pcb *proceso_a_ejecutar = queue_pop(cola_ready);

        // wait(sem_mutex_interrupt)
        enviar_interrupcion(socket_cpu_interrupt, pcb_ejecutandose, FIN_DE_QUANTUM);
        esperar_cpu();
        // sem_post(sem_mutex_interrupt)

        // Cambiar el estado del proceso a EXEC
        proceso_a_ejecutar->estado = EXEC;

        enviar_pcb(socket_cpu_dispatch, proceso_a_ejecutar);

        pthread_create(&quantum_thread, NULL, (void *)quantum_count,NULL);

        pcb_ejecutandose = proceso_a_ejecutar;

        // Si el proceso que envie tiene quantum, voy a chequear cuando tengo que decirle al CPU que corte
        // Tambien lo podriamos hacer del lado de CPU, pero funcionalmente no estaria OK.
        // TODO, desalojar de CPU con interrupcion. ¿Ya está hecho?
    }
    // mutex en la cola de interrupt
}

void planificar_vrr()
{
    t_pcb *proceso_a_ejecutar;

    sem_wait(&sem_round_robin);

    if (!queue_is_empty(cola_ready) || !queue_is_empty(cola_prio))
    {
        // Obtener el proceso listo para ejecutarse de la cola

        if (!queue_is_empty(cola_prio))
        {
            proceso_a_ejecutar = queue_pop(cola_prio);
        }
        else
        {
            proceso_a_ejecutar = queue_pop(cola_ready);
        }
    }
    // sem_mutex_interrupt
    // sem_wait(sem_mutex_interrupt);
    enviar_interrupcion(socket_cpu_interrupt, pcb_ejecutandose, FIN_DE_QUANTUM);
    esperar_cpu();
    // sem_post(sem_mutex_interrupt);
    //  Cambiar el estado del proceso a EXEC
    proceso_a_ejecutar->estado = EXEC;

    enviar_pcb(socket_cpu_dispatch, proceso_a_ejecutar);
    cron_quant_vrr = temporal_create();
    pthread_create(&hilo_quantum_vrr, NULL, (void *)quantum_count,NULL);
    pcb_ejecutandose = proceso_a_ejecutar;
}

void quantum_block()
{
    sem_wait(&sem_vrr_block);
    pcb_ejecutandose->quantum = (int)temporal_gettime(cron_quant_vrr); // agregarlo a todas las io
    temporal_destroy(cron_quant_vrr);
    pthread_kill(hilo_quantum_vrr, SIGKILL);
    sem_post(&sem_round_robin);
}

// en caso de que muera el proceso, se resetea cron_quant_vrr y se mata a quantumcount y se agrega a queue_prio
//  valor del timer en el pcb, se manda sempostvrr

void quantum_count()
{
    usleep(quantum * 1000);
    if (strcmp(algoritmo_planificacion, "VRR") == 0)
    {
        temporal_destroy(cron_quant_vrr);
    }
    sem_post(&sem_round_robin);
}

void esperar_cpu()
{
    log_trace(logger, "PCP: Esperando respuesta del CPU");
    op_code cod_op = recibir_operacion(socket_cpu_dispatch);
    log_trace(logger, "PCP: Llegó una respuesta del CPU");
    
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
            log_warning(logger, "La interfaz no existe. Se mandará el proceso a EXIT");
            eliminar_proceso(sleep->pcb);
        }

        // Si la interfaz no es del tipo "Interfaz Genérica" mandamos el proceso a EXIT
        if (interfaz_sleep->tipo != GENERICA)
        {
            log_warning(logger, "La interfaz no admite la operación solicitada. Se mandará el proceso a EXIT");
            eliminar_proceso(sleep->pcb);
        }

        sleep->pcb->estado = BLOCKED;
        list_add(lista_bloqueados, sleep->pcb);
        if (strcmp(algoritmo_planificacion, "VRR") == 0)
        {
            sem_post(&sem_vrr_block);
        }
        // Verficamos si la interfaz está ocupada
        if (interfaz_sleep->ocupada == false)
        {
            enviar_sleep(interfaz_sleep->socket, sleep->pcb, sleep->nombre_interfaz, sleep->unidades_de_trabajo);
        }
        else
        {
            log_error(logger, "La interfaz estaba ocupada pero falta implementar el comportamiento"); // TODO
        }

        interfaz_sleep->ocupada = true;

        // free(sleep->pcb->cpu_registers);
        // free(sleep->pcb);
        // free(sleep);

        break;
    case INSTRUCCION_EXIT:
        log_debug(logger, "El CPU informa que le llegó una instrucción EXIT");
        t_pcb *pcb_exit = recibir_pcb(socket_cpu_dispatch);
        log_info(logger, "Finaliza el proceso %i - Motivo: EXIT", pcb_exit->pid);
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
        }
        if (interrupcion->motivo == FIN_DE_QUANTUM)
        {
            if (strcmp(algoritmo_planificacion, "RR") == 0)
            {
                log_debug(logger, "Este motivo es FIN_DE_QUANTUM con RR, entonces se agrega al final de la cola el proceso");
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
        // TODO PROCESAR WAIT Y SIGNAL DE CPU
        // if(interrupcion->motivo == WAIT){}
        // if(interrupcion->motivo == SIGNAL){}
        break;
    case WAIT:
        t_recurso *recurso_wait = recibir_recurso(socket_cpu_dispatch);
        t_pcb *pcb_wait = recurso_wait->pcb;
        char *recurso_solicitado = recurso_wait->nombre;
        if (dictionary_has_key(dic_recursos, recurso_solicitado))
        {
            int cant_inst = dictionary_get(dic_recursos, recurso_solicitado);
            cant_inst--;
            dictionary_put(dic_recursos,recurso_solicitado,cant_inst);
            if (cant_inst < 0)
            {
                bloquear_proceso(pcb_wait);
            }
        }
        else
        {
            // enviar proceso a exit
            eliminar_proceso(pcb_wait);
        }
        break;
    case SIGNAL:
        t_recurso *recurso_signal = recibir_recurso(socket_cpu_dispatch);
        t_pcb *pcb_signal = recurso_signal->pcb;
        char *recurso_liberado = recurso_signal->nombre;
        if (dictionary_has_key(dic_recursos, recurso_liberado))
        {
            int cant_inst = dictionary_get(dic_recursos, recurso_liberado);
            cant_inst++;
            dictionary_put(dic_recursos, recurso_liberado,cant_inst);
            if (cant_inst >= 0)
            {
                desbloquear_proceso(pcb_signal);
            }
        }
        else
        {
            eliminar_proceso(pcb_signal);
        }
        break;
    default:
        log_warning(logger, "Mensaje desconocido del CPU: %i", cod_op);
        break;
    }
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

    // Se habilita un espacio en el grado de multiprogramación
    // TODO poner MUTEX
    grado_multiprogramacion_activo--;
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
            char path[300];
            sscanf(linea, "%s %s", comando, path);

            printf("Falta implementar\n"); // TODO
        }
        else if ((strcmp(comando, "INICIAR_PROCESO") == 0) || (strcmp(comando, "2") == 0))
        {
            char path[300];
            sscanf(linea, "%s %s", comando, path);

            crear_pcb(path);
        }
        else if ((strcmp(comando, "FINALIZAR_PROCESO") == 0) || (strcmp(comando, "3") == 0))
        {
            int pid;
            sscanf(linea, "%s %i", comando, &pid);

            encontrar_y_eliminar_proceso(pid);
        }
        else if ((strcmp(comando, "DETENER_PLANIFICACION") == 0) || (strcmp(comando, "4") == 0))
        {
            printf("Falta implementar\n"); // TODO
        }
        else if ((strcmp(comando, "INICIAR_PLANIFICACION") == 0) || (strcmp(comando, "5") == 0))
        {
            printf("Falta implementar\n"); // TODO
        }
        else if ((strcmp(comando, "MULTIPROGRAMACION") == 0) || (strcmp(comando, "6") == 0))
        {
            int valor;
            sscanf(linea, "%s %i", comando, &valor);

            printf("Falta implementar\n"); // TODO
        }
        else if ((strcmp(comando, "PROCESO_ESTADO") == 0) || (strcmp(comando, "7") == 0))
        {
            printf("Falta implementar\n"); // TODO
        }
        else
        {
            printf("Comando inválido\nEscriba COMANDOS para ver los comandos disponibles\n");
        }

        free(linea);
    }
}