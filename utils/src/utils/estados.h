// estados.h

#ifndef ESTADOS_H
#define ESTADOS_H

#include <stdint.h>


typedef enum {
    NEW,
    READY,
    BLOCKED,
    EXEC,
    EXIT
} estado_proceso;

#endif /* ESTADOS_H */