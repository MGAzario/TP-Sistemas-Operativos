// ---------------------------------------------    Instrucciones CPU    ---------------------------------------------

// void seleccionarOperacion(t_list* instruccion, t_reg* reg)
// {
// 	int codigoInstruccion= (int) listPop(instruccion);

// 	switch (codigoInstruccion){
// 		case SET:
// 			ejecutarSET(instruccion, reg);
// 			break;
// 		case SUM:
// 			ejecutarSUM(instruccion, reg);
// 			break;
// 		case SUB:
// 			ejecutarSUB(instruccion, reg);
// 			break;
// 		case JNZ:
// 			ejecutarJNZ(instruccion, reg);
// 			break;
// 		case IO_GEN_SLEEP:
// 			ejecutarIO_GEN_SLEEP(instruccion, reg);
// 			break;
// 		case EXIT:
// 			ejecutarEXIT(instruccion,reg);
// 	}

// }

// int isUint8(int regCod)
// {
// 	int ret = 0;
// 	if((AX <= regCod) && (regCod <= DX))
// 		ret = 1;
// 	return ret;
// }

// void setRegistro(t_reg* reg, int codReg, uint32_t valor)
// {
// 	switch (codReg)
// 	{
// 		case AX:
// 			reg->AX = valor;
// 			break;
// 		case BX:
// 			reg->BX = valor;
// 			break;
// 		case CX:
// 			reg->CX = valor;
// 			break;
// 		case DX:
// 			reg->DX = valor;
// 			break;
// 		case EAX:
// 			reg->EAX = valor;
// 			break;
// 		case EBX:
// 			reg->EBX = valor;
// 			break;
// 		case ECX:
// 			reg->ECX = valor;
// 			break;
// 		case EDX:
// 			reg->EDX = valor;
// 			break;
	
// 	}
// }

// uint32_t obtenerRegistro(t_reg* reg, int codReg)
// {
// 	uint32_t ret = 0;
// 	switch (codReg)
// 	{
// 		case AX:
// 			ret = reg->AX;
// 			break;
// 		case BX:
// 			ret = reg->BX;
// 			break;
// 		case CX:
// 			ret = reg->CX;
// 			break;
// 		case DX:
// 			ret = reg->DX;
// 			break;
// 		case EAX:
// 			ret = reg->EAX;
// 			break;
// 		case EBX:
// 			ret = reg->EBX;
// 			break;
// 		case ECX:
// 			ret = reg->ECX;
// 			break;
// 		case EDX:
// 			ret = reg->EDX;
// 			break;	
// 	}
// 	return ret;
// }

// void ejecutarSET(t_list* instruccion, t_reg* reg)
// {
// 	int codReg = (int) listPop(instruccion);
// 	uint32_t valor = (uint32_t) listPop(instruccion);
// 	setRegistro(reg, codReg, valor);
// 	free (codReg);
// 	free(valor);
// }

// void ejecutarSUM(t_list* instruccion, t_reg* reg)
// {
// 	int codReg1 = (int) listPop(instruccion);
// 	int codReg2 = (int) listPop(instruccion);
// 	setRegistro(reg, codReg1, obtenerRegistro(reg, codReg1) + obtenerRegistro(reg, codReg2));
// 	free (codReg1);
// 	free(codReg2);
// }

// void ejecutarSUB(t_list* instruccion, t_reg* reg)
// {
// 	int codReg1 = (int) listPop(instruccion);
// 	int codReg2 = (int) listPop(instruccion);
// 	setRegistro(reg, codReg1, obtenerRegistro(reg, codReg1) - obtenerRegistro(reg, codReg2));
// 	free (codReg1);
// 	free(codReg2);
// }

// void ejecutarJNZ(t_list* instruccion, t_reg* reg)
// {
// 	int codReg = (int) listPop(instruccion);
// 	uint32_t valor = (uint32_t) listPop(instruccion); 
// 	if(obtenerRegistro(reg, codReg) != 0)
// 		reg->PC = valor;
// 	free (codReg);
// 	free(valor);
// }

// void ejecutarIO_GEN_SLEEP(t_list* instruccion, t_reg* reg)
// {
// 	mensajeInterrupt = (char*)listPop(instruccion);
	
// 	int codReg2 = (int) listPop(instruccion);

// 	sem_post(&semDispatch);


// 	free (codReg1);
// 	free(codReg2);

// }
// void ejecutarEXIT(t_list* instruccion, t_reg* reg){
// 	sem_post(&semInterrupt); //entiendo que es asi, no estoy seguro porque no se si la interrupcion que sigue es la que corresponde
// }

// ---------------------------------------------    Kernel server    ---------------------------------------------

// void* server(void*)
// {
// 	int socketKernel = iniciarServidor(puertoKernel, logger);
// 	int i = 0;

// 	pthread_t ioThread;
// 	while(true)
// 	{
// 		// TODO: RAZONAR SI ES NECESARIO AGREGAR UN SEMAFORO CON MAX_CLIENTES PARA QUE EL WHILE TRUE NO SEA UNA ESPERA ACTIVA
// 		if(i = checkSem() >= 0)
// 		{
// 			clientes[i].socket = esperarCliente(socketKernel, logger);
// 			pthread_create(&ioThread, NULL, (void*)gestionarIO, (void*)i);			//MUTEX¿?
// 		}
// 	}
    
//     liberarConexion(socketKernel);

//     return 0;
// }

// int checkSem(void)
// {
// 	int i;
// 	int valor = 0;
// 	for(i = 0; ((i<MAX_CLIENTES) && (valor==0)); i++)
// 	{
// 		sem_getvalue(&(clientes[i].semCli),&valor);
// 	}
// 	if(valor == 0)
// 		i = -1;
// 	return i;
// }

// void gestionarIO(void* num)
// {	
// 	int i = (int) num;
// 	clientes[i].nombre = recibirMensaje(clientes[i].socket, logger);
// 	sem_wait(&(clientes[i].semCli));
// 	free(clientes[i].nombre);
//     liberarConexion(clientes[i].socket);
// }

// ---------------------------------------------    Interfaz genérica    ---------------------------------------------

// include <entradasalida.h>

// t_log* logger;
// t_config* config;
// char* ipKernel;
// char* puertoKernel;
// char* ipMemoria;
// char* puertoMemoria;
// char* tipoInterfaz;
// int tiempoUnidadTrabajo;

// int main(int argc, char* argv[]) 
// {

//     logger = iniciarLogger("entradasalida");
//     /*                                  UTILIZAR CUANDO SE SEPA CÓMO INGRESAR PARÁMETROS
//     if(argc != 3)
//     {
//         log_error(logger,"No se ingresaron suficientes parámetros");
//         return -1;
//     }
//     log_destroy(logger);
    
//     char* nombre = (char*) malloc(strlen(argv[1]) + 1);
//     char* pathConf = (char*) malloc(strlen(argv[2]) + 1 + strlen(".config"));
//     */
//     char nombre[] = "iOPrueba";
//     char pathConf []= "generica";
    
//     iniciarLogger(nombre);

//     strcat(pathConf,".config");
	
// 	config = config_create(pathConf);
// 	if(pathConf == NULL)
// 	{
// 		log_error(logger , "%s.c>iniciar_config||No se pudo crear nuevoConfig", nombre);
// 		// TODO: Diseñar e implementar manejo de errores
// 	}

//     tipoInterfaz = config_get_string_value(config, "TIPO_INTERFAZ");

//     seleccionarInterfaz(tipoInterfaz, nombre);

//     terminarPrograma();
//     return 0;
// }

// void terminarPrograma()
// {
// 	config_destroy(config);
// 	log_destroy(logger);
// }

// void seleccionarInterfaz(char* tipoInterfaz, char* nombre)
// {
//     if(strcmp(tipoInterfaz, "Generica") == 0)
//         interfazGenerica(nombre);
// }

// void interfazGenerica(char* nombre)
// {
//     ipKernel = config_get_string_value(config, "IP_KERNEL");
//     puertoKernel = config_get_string_value(config, "PUERTO_KERNEL");
//     tiempoUnidadTrabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");

//     int conexion = crearConexion(ipKernel, puertoKernel);

//     enviarMensaje(nombre, conexion);
//     //free(nombre);                 DESCOMENTAR CUANDO SE UTILICEN PARAMETROS
//     while(true)
//     {
//         char* respuesta = recibirMensaje(conexion, logger);
//         char** decodificada = string_split(respuesta, ',');

//         if(strcmp(decodificada[0], "IO_GEN_SLEEP") == 0)
//         {
//             int multiplicador = atoi(decodificada[1]);
//             usleep(multiplicador * 1000 * tiempoUnidadTrabajo);
//             enviarMensaje("FIN", conexion);
//         }
//         free(respuesta);
//     }
// }

// // ---------------------------------------------    RR    ---------------------------------------------

// void roundRobin(t_list* lista, int* quantum)
// {
// 	pthread_t quantumThread;

// 	pthread_create(&quantumThread, NULL, (void*) quantumCount, (void*) quantum);
// 	sem_wait(&semRR);

// 	void* element = listPop(lista);
// 	list_add(lista,element);
// }

// void quantumCount(int* quantum)
// {
// 	usleep(*quantum * 1000);
// 	sem_post(&semRR);
// }