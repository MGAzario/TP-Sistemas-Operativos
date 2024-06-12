#ifndef UTILS_MEMORIA_H
#define UTILS_MEMORIA_H

#include "memoria.h"


t_tabla_de_paginas *buscar_tabla_de_paginas(int pid);
void liberar_ultimo_marco(t_tabla_de_paginas *tabla_de_paginas);
void estado_del_bitarray();
void estado_tabla_de_paginas(t_tabla_de_paginas *tabla_de_paginas);


#endif /*UTILS_MEMORIA_H*/