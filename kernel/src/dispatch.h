// registros.h

#ifndef DISPATCH_H
#define DISPATCH_H

#include <stdint.h>
#include <kernel.h>

void enviar_pcb(int socket_cliente, t_pcb *pcb);

#endif /* DISPATCH_H */