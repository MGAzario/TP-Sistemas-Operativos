#include "hello.h"

void decir_hola(char* quien) {
    log_info(logger, "\nHola desde %s!!\n", quien);
}
