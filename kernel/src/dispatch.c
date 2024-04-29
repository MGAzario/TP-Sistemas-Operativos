#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_cliente.h>
#include <utils/utils_server.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <kernel.h>

// Requisito Checkpoint: Es capaz de enviar un proceso a la CPU
/*Una vez seleccionado el siguiente proceso a ejecutar, se lo transicionará al estado EXEC y se enviará su Contexto de Ejecución al CPU
a través del puerto de dispatch, quedando a la espera de recibir dicho contexto actualizado después de la ejecución,
junto con un motivo de desalojo por el cual fue desplazado a manejar.*/
extern t_config *config;
extern t_log *logger;