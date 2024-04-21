#include "hello.h"

void decir_hola(char* quien) {
    log_info(logger, "Hola desde %s!!\n", quien);
}
