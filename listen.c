/*  PROYECTO HOST DISCOVER + FILE TRANSFER - LISTENER (receptor) 
Para correr el programa escribir en la CLI  ./listen 
Jorge Humberto Arrezola Bautista  678608
Mauricio Javier Gonzalez Salinas 672475
Programación de Sockets.
Prof. Alvaro Parres.
*/

#include <stdio.h>      
#include <sys/socket.h> 
#include <arpa/inet.h>  
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include "netlib/tcp.h"
#include "utils/debug.h"
#define BUFFERSIZE  102400
#define	CONFIG_LISENT_IFACE			"0.0.0.0" /*< Interfaz para escucar */
#define CONFIG_MAX_CLIENT			5		/*< Maximo de Conexiones Simultaneas */
#define CONFIG_DEFAULT_PORT			5000	/*< Puerto de trabajo */

#define CONFIG_DEFAULT_BASEDIR		"./"

#define CONFIG_DEFAULT_VERBOSE		1		/*< Nivel de Verbosity  */

#define MINPORT						1
#define MAXPORT						65000


void startProtocolFork(const int clientSocket,const char baseDir[]);
void processGET(const int clientSocket, const char  baseDir[], const char *fileName);
void processLIST(const int clientSocket, const char baseDir[]);
int sendLine(const int clientSocket, const char* writeBuffer);
int start_server(const u_short port, const char baseDir[]);
//Global Vars
u_short port = CONFIG_DEFAULT_PORT;
int debugLevel = CONFIG_DEFAULT_VERBOSE;
char baseDir[255] = CONFIG_DEFAULT_BASEDIR;

int main(int argc, char* argv[]) {
	
	int udpSocket,udpSocket2;
	struct sockaddr_in udpServer, udpClient;
	int localerror;
	socklen_t addrlen = sizeof(udpClient);
	char buffer[255];
	char ip[17];
	//u_short clientPort;
	int forkId;

//start_server(port,baseDir);
	int status;

	forkId = fork();

	if(forkId == 0) {
	        start_server(port,baseDir);
		return 0;
	} 

        //Creamos el Socket para recibir mensajes
	udpSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(udpSocket == -1) {
		fprintf(stderr,"Can't create UDP Socket");
		return 1;
	}
	
	//Creamos el Socket para enviar mensajes
	udpSocket2 = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(udpSocket == -1) {
		fprintf(stderr,"Can't create UDP Socket");
		return 1;
	}	
	
    udpServer.sin_family = AF_INET;
    inet_pton(AF_INET,"0.0.0.0",&udpServer.sin_addr.s_addr);
    udpServer.sin_port = htons(5000);
	//con bind asociamos el socket para recibir (udpSocket) a la dirección del udpServer)
    status = bind(udpSocket, (struct sockaddr*)&udpServer,sizeof(udpServer));
	//Validamos cualquier error al hacer el bind
    if(status != 0) { 
	  fprintf(stderr,"Can't bind");
    }

	/////este hilo se encarga de escuchar las respuestas de los hosts (listeners)
	while(1) {
		bzero(buffer,255);
		status = recvfrom(udpSocket, buffer, 255, 0, (struct sockaddr*)&udpClient, &addrlen );
		////ver por donde se recibe los mensajes del sender para en el sender recibir el ME PC2
		inet_ntop(AF_INET,&(udpClient.sin_addr),ip,INET_ADDRSTRLEN);
		//clientPort = ntohs(udpClient.sin_port);
		//printf("Recibimos: [%s:%i] %s\n",ip,clientPort,buffer);
					
		udpClient.sin_port =  htons(5000);
		///Mandamos como respuesta un ME PC2" al sender.
		status = sendto(udpSocket2,"ME PC2\r\n\r\n",strlen("ME PC2\r\n\r\n"),0,(struct sockaddr*)&udpClient, addrlen);
		if(status==-1){
                localerror = errno;
                printf("%s",strerror(localerror));
		}
   	    fflush(stdout);
	}
	
	close(udpSocket);
	
	return 0;
	
}

int start_server(const u_short port, const char baseDir[]) {
	
    int clientes = 0,serverSocket,clientSocket;
    char clientIP[18];
    u_int clientPort;
    int forkID;
	
    serverSocket = newTCPServerSocket4(CONFIG_LISENT_IFACE,port,CONFIG_MAX_CLIENT); //Creamos el socket para el server
	
	if(serverSocket == -1) {
		return 0;
	}
	//Dejamos el server escuchando para conexiones activas
	bzero(clientIP,sizeof(clientIP));
	clientPort = 0;
	while(1){
		
	debug(5,"%s","Waiting for a Client...");
	clientSocket = waitConnection4(serverSocket,clientIP,&clientPort);
	clientes++;
	debug(4,"Tenemos en linea %i clientes\n",clientes);		
	debug(4,"Connected Client %s:%u",clientIP,clientPort);

	forkID = fork(); //////////////START FORK
		
		if(forkID == -1) {
			fprintf(stderr,"Cant Fork\n");
		} else if(forkID == 0) {
			//Soy el HIJO
			closeTCPSocket(serverSocket);
			startProtocolFork(clientSocket,baseDir);
			clientes--;
			closeTCPSocket(clientSocket);
			return 0;
		} else {
		//	//SOY EL PADRE
			closeTCPSocket(clientSocket);
		} /////////////////////FIN FORK
	

	} //fin de while(true)
	return 1;
	
}

void startProtocolFork(const int clientSocket,const char baseDir[]) {
///Esta es la funcion que maneja el protocolo de comunicacion del File Transfer
	char readBuffer[1024];
	int readBytes;
	char *commandStr,*end,*token;
	

	while(1) {  //El ciclo se repite siempre hasta que se cumple la condicion de paro en el break;
		commandStr = (char *) calloc(255,1);
		while((readBytes = read(clientSocket,readBuffer,1))>0) { //leemos byte por byte readBuffer
		commandStr = (char *) realloc(commandStr,strlen(commandStr)+readBytes+1);
		strncat(commandStr,readBuffer,readBytes); //concatenamos en commandStr lo que se haya leido en readBuffer
		end = commandStr+(strlen(commandStr)-4);//colocamos end 4 bytes antes del final de commandStr
			if(strcmp(end,"\r\n\r\n")==0) {//si hemos llegado al final del bufer:
				*end = '\0';  //Colocamos un caracter nulo
				break; //Solo entonces dejamos de leer
			}
		}

	debug(4,"<--- %s",commandStr);
	token = strtok(commandStr," \t\b"); //colocamos el token despues del espacio en blanco
		if(strcmp(token,"GET")==0) { //si la palabra que sigue es un GET:
			token = strtok(NULL," \t\b"); //Checamos si el token coincide con NULL
			if(token == NULL) { //Si es igual a NULL, significa que falta el nombre del archivo
			sendLine(clientSocket, "ERROR (Missing FileName)\r\n\r\n");
			} else {//Si no es NULL, entonces procesamos el GET 
				processGET(clientSocket,baseDir, token);
			}
		} else if(strcmp(token,"LIST")==0) { //Si se recibió un encabezado LIST, lo procesamos con processLIST()
			processLIST(clientSocket,baseDir);
		} else if(strcmp(token,"QUIT")==0) { //Si se recibió un QUIT, se termina la conexión
					break;
		} else {//si no se recibe una cabecera conocida, manda UNKOWN METHOD
			sendLine(clientSocket, "UNKOWN METHOD\r\n\r\n");
		}

		if(commandStr != NULL) free(commandStr);
	}

}
 
void processGET(const int clientSocket, const char  baseDir[], const char *fileName) {
	//funcion que procesa los Headers GET
	char *writeBuffer = (char *) malloc(255);
	u_int writeBytes = 0,readBytes = 0;
	char *readBuffer,*fullPath; 
	int fileDesc;
	int localError;
	struct stat fileStatus;
	
	fullPath = (char *) malloc(strlen(baseDir)+strlen(fileName)+1);
	strcpy(fullPath,baseDir);//copiamos el ./ a fullpath
	strcat(fullPath,fileName); //concatenamos el nombre del archivo
	fileDesc = open(fullPath,O_RDONLY);
	free(fullPath);
	///Si sucede algun error al abrir el archivo...
	if(fileDesc == -1) {
		localError = errno;
		debug(1,"Can't open Requested File (%s)",strerror(localError));
		sprintf(writeBuffer,"NOT_FOUND %s\r\n\r\n",fileName);
		sendLine(clientSocket, writeBuffer);
		free(writeBuffer);
		return;
	} 
	//Si no hubo ningun error y el archivo fue encontrado...
	bzero(writeBuffer,255);
	sprintf(writeBuffer,"OK\r\n");		;
        sendLine(clientSocket, writeBuffer); //mandamos un OK al cliente
        fstat(fileDesc, &fileStatus); //con esta funcion obtenemos el tmaño del archivo referenciado por el file descriptor fileDesc
	sprintf(writeBuffer,"Size: %u\r\n",(u_int)fileStatus.st_size); //copiamos al bufer la cabecera Size
	sendLine(clientSocket,writeBuffer); //mandamos el encabezado Size: 
	sendLine(clientSocket,"\r\n");	//despues mandamos una linea en blanco
	
	readBuffer = (char *) malloc(102400);	//Despues quedamos a la escucha de respuesta del cliente
	while((readBytes = read(fileDesc,readBuffer,102400))>0) {
		writeBytes = 0;
		while(writeBytes < readBytes) {
			writeBytes += write(clientSocket,readBuffer+writeBytes,readBytes-writeBytes);
		}
	}
	//Liberamos la memoria ocupada por los búfers.
	free(readBuffer);
	free(writeBuffer);
	close(fileDesc);  //cerramos el archivo en fileDesc
}

int sendLine(const int clientSocket, const char* writeBuffer) {
	u_int writeBytes = 0;
       
        //Esta funcion escribe en el socket ClientSocket lo que esté escrito en el buffer writeBuffer
	do { 
	    writeBytes += write(clientSocket,writeBuffer+writeBytes,strlen(writeBuffer)-writeBytes);
	} while(writeBytes < strlen(writeBuffer));

	debug(4,"---> %s",writeBuffer);	

	return writeBytes;
}

void processLIST(const int clientSocket, const char baseDir[]) {
	
	DIR *dir;
	struct dirent *ent;
	char writeBuffer[1024];

	if ((dir = opendir (baseDir)) != NULL) {
		while ((ent = readdir (dir)) != NULL) {
			if(strcmp(ent->d_name,".")==0) continue;
			if(strcmp(ent->d_name,"..")==0) continue;			
			sprintf(writeBuffer,"%s\r\n",ent->d_name);
			sendLine(clientSocket,writeBuffer);
		}
		closedir (dir);
		sendLine(clientSocket,"\r\n");
	} else {
		sendLine(clientSocket, "INTENRAL ERROR ( Can't open directory)\r\n\r\n");
	}
		
}

