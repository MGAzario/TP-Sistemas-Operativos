// registros.h

#ifndef DISPATCH_H
#define DISPATCH_H

#include <stdint.h>
#include <kernel.h>
#include <utils/utils.h>

int iniciar_dispatch_cpu();
void enviar_pcb(int socket_cliente, PCB *pcb);
void conectar_dispatch_cpu(PCB *proceso_enviar);

#endif /* DISPATCH_H */