#include <utils/hello.h>

void decir_hola(char* quien) {
    t_log* hola_logger = log_create("./hola.log","LOG_HOLA",true,LOG_LEVEL_INFO);
    if(hola_logger == NULL){
		perror("Ocurrió un error al leer el archivo de Log de Entrada/Salida");
		abort();
	}
    ("Hola desde %s!!\n", quien);
}
