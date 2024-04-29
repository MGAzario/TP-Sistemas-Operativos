#ifndef KERNEL_H
#define KERNEL_H
#include <utils/registros.h>
#include <utils/estados.h>


void procedimiento_de_prueba();
void crear_logger();
void crear_config();
void conectar_memoria();
void conectar_dispatch_cpu(char *ip_cpu);
void conectar_interrupt_cpu(char *ip_cpu);
void recibir_entradasalida();
void crear_pcb(int quantum);
void mover_procesos_ready();
void planificador_corto_plazo();
void planificador_largo_plazo();
void planificar_fifo();
void planificar_round_robin();


#endif /*KERNEL_H*/