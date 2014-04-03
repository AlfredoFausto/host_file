Para correr el programa ./sender <broadcast_ipaddress>  5000

#include <stdio.h>      
#include <sys/socket.h> 
#include <arpa/inet.h>  
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>     
#include "client.h"
#include <netdb.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "netlib/tcp.h"
#include "utils/debug.h"
#include "debug.h"
#include <stdarg.h>
#include <strings.h>


void beginProtocol(const int clientSocket,const char *remoteFilename,const char *localFilename);
u_long currentTimeMillis();

int newTCPServerSocket4(const char *ip, const u_short port, const int q_size);
void closeTCPSocket(const int socketFD);
int waitConnection4(int socket, char *clientIP, u_int *clientPort);
int newTCPClientSocket4(const char *ip, const u_short port);

int start_client(const u_short port, const char ip[], const char remoteFilename[], const char localFilename[] );
int sendLine(const int clientSocket, const char* writeBuffer);
void processGET(int socket, const char *localFilename, unsigned long size) ;
int getNewTCPSocket(int addrType);
int buildAddr4(struct sockaddr_in *addr, const char *ip, const u_short port);

int main(int argc, char *argv[])
{
    int bcSock, udpSocket;                        
    struct sockaddr_in broadcastAddr; 
    char *broadcastIP, strbuffer[255], buffer[255];                
    unsigned short broadcastPort;     
    pid_t forkID; //es un int                 
    
    int broadcastPermission;          
    unsigned int sendStringLen; 

	int status;
	int i;
	int found=0,size = 0;
	struct sockaddr_in udpClient,udpServer;
	socklen_t addrlen = sizeof(udpClient);
	char ip[17], *knownHosts[20];
	static char chosenip[17];
	static char filename[255];
	static char localFilename[255];
	//u_short clientPort;

///El siguiente código lo utilizo para obtener la ip de mi propia PC en la interfaz especificada
struct ifaddrs *ifaddr, *ifa;
    int s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) 
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
    {
        if (ifa->ifa_addr == NULL)
            continue;  

        s=getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
	//Para interfaz cableada eth0 y wireless wlan0, para loopback cambiar a lo
        if((strcmp(ifa->ifa_name,"eth0")==0)&&(ifa->ifa_addr->sa_family==AF_INET))
        {
            if (s != 0)
            {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            printf("\tInterface : <%s>\n",ifa->ifa_name );
            printf("\t  Address : <%s>\n", host); 
        }
    }
	freeifaddrs(ifaddr);

/////////////fin de código para encontrar mi propia dirección IP.

    if (argc < 3)                     
    {
        //fprintf(stderr,"Usage:  %s <IP Address> <Port> <Send String>\n", argv[0]);
	fprintf(stderr,"Usage:  %s <IP Address> <Port>\n", argv[0]);
        exit(1);
    }

    broadcastIP = argv[1];            /* First arg:  broadcast IP address */ 
    broadcastPort = atoi(argv[2]);    /* Second arg:  broadcast port */
    sprintf(strbuffer,"WHO IS THERE %i\r\n\r\n",(int)broadcastPort);	
	//Creamos bcSock, el socket para enviar	broadcast
	bcSock = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(bcSock == -1) {
		fprintf(stderr,"Can't create Socket");
		return 1;
	}
  	
	//Creamos el udpSocket para recibir mensajes
	udpSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(udpSocket == -1) {
		fprintf(stderr,"Can't create UDP Socket");
		return 1;
	}

    broadcastPermission = 1;
    status = setsockopt(bcSock, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission, sizeof(broadcastPermission));
    if(status == -1) {
		fprintf(stderr,"Can't set Brodcast Option");
		return 1;
    }

    memset(&broadcastAddr, 0, sizeof(broadcastAddr));   
    broadcastAddr.sin_family = AF_INET;                 
	inet_pton(AF_INET,broadcastIP,&broadcastAddr.sin_addr.s_addr); 
    broadcastAddr.sin_port = htons(broadcastPort);         
	 sendStringLen = strlen(strbuffer);  

    udpServer.sin_family = AF_INET;
    inet_pton(AF_INET,"0.0.0.0",&udpServer.sin_addr.s_addr);
    udpServer.sin_port = htons(5000);
	//con bind asociamos el socket para recibir (udpSocket) a la dirección del udpServer)
    status = bind(udpSocket, (struct sockaddr*)&udpServer,sizeof(udpServer));
	if(status != 0) { 
	  fprintf(stderr,"Can't bind");
    }	
	forkID = fork(); //////////////START FORK
	////Checar donde poner los exit() y donde cerrar los sockets UDP....
	if(forkID == -1) {
			fprintf(stderr,"Cant Fork\n");
		} else if(forkID == 0) {
			//Soy el HIJO
			//printf("Recibimos: OK\n");
		while(1) {
		bzero(buffer,255);
		status = recvfrom(udpSocket, buffer, 255, 0, (struct sockaddr*)&udpClient, &addrlen );
		////ver por donde se recibe los mensajes del sender para en el sender recibir el ME PC2
		inet_ntop(AF_INET,&(udpClient.sin_addr),ip,INET_ADDRSTRLEN);
	//	clientPort = ntohs(udpClient.sin_port);
		////no se esta recibiendo el buffer que manda el listen en ME PC2....checar
		
		if(strcmp(ip,host)!=0){ //nos aseguramos de no imprimir las respuestas de mi propia IP.
		//fprintf(stderr,"Recibimos: %s desde: [%s:%i]\n",buffer,ip,clientPort);
		
		///Creamos la lista de hosts conocidos con un arreglo de apuntadores a char knownHosts
		for(i=0; i < size; i++) {
   			if(strcmp(knownHosts[i],ip)==0) {
      			  found = 1;
			break;
   			}
		} 

			if(!found) {
			   knownHosts[size] = (char *) malloc(16);
 			   strcpy(knownHosts[size],ip);
   			   size++;
			} 
				
		}	
		
		i=0;
		///En cada respuesta imprimimos la lista de hosts conocidos
		fprintf(stderr,"Hosts conocidos por ");
		while(knownHosts[i]!=NULL){
		
		fprintf(stderr,"%s \n",knownHosts[i]);
		i++;	
		}


		udpClient.sin_port = 5000;
		bzero(buffer,255);
   	    fflush(stdout);
	}
	
	close(udpSocket);
			//exit(0);
			return 0;
		} 
	else {
		//	//SOY EL PADRE
		for(i=0; i<2; i++) {  //Este hilo (proceso) sólo manda WHO IS THERE cada 3 segundos
		status = sendto(bcSock,strbuffer,sendStringLen,0,(struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));    
      ///Respuestas cada 3 segundos
        sleep(1);   
    	}
		//exit(0);///
		//close(udpSocket); 

	} /////////////////////FIN FORK
		//exit(0);
//close(udpSocket);
fprintf(stderr,"\nElija una IP\n");
gets(chosenip);
fprintf(stderr,"\nUsted escogió al host: %s\n",chosenip );
get_LIST(5000,chosenip); 
fprintf(stderr,"\nAhora teclea el archivo que deseas obtener\n");
gets(filename);
fprintf(stderr,"\nTeclea con qué nombre quieres guardar el archivo\n");
gets(localFilename);
start_client(5000,chosenip,filename,localFilename);
sleep(15); 


return 0;
} //fin main

int get_LIST(const u_short port, const char ip[]) {

	int socket;
	char *list,*end;
	char *readBuffer = (char *) malloc(255); 
	char *writeBuffer = (char *) malloc(255);
	int readBytes;
	
	socket = newTCPClientSocket4(ip, port); //Creamos el socket del cliente
	
	if(socket == -1) { //Si no se pudo, error, regresamos un 1
		return 1;
	}
	//Si todo salio bien...
	sprintf(writeBuffer,"LIST\r\n\r\n"); //imprimimos en writeBuffer el header LIST
	sendLine(socket,writeBuffer);	 //y lo escribimos en el socket
	
	//ahora leemos la respuesta al LIST
	list = (char *) malloc(254); //list guardará la lista de archivos del cliente
	while((readBytes = read(socket,readBuffer,1))>0) {//leemos byte por byte readBuffer
		list = (char *) realloc(list,strlen(list)+readBytes+1);
		strncat(list,readBuffer,readBytes);//concatenamos en list lo que se haya leido en readBuffer
		end = list+(strlen(list)-4);//colocamos end 4 bytes antes del final de list
		if(strcmp(end,"\r\n\r\n")==0) { //si hemos llegado al final del bufer:
			*end = '\0'; //Colocamos un caracter nulo
			break; //Solo entonces dejamos de leer
		}
	}	
	
	printf("Esta es la lista de archivos del host:\n"); 	
	printf("%s\n",list); //entonces imprimimos la lista entera
	
	//Liberamos la memoria de los punteros. 
	free(list);
	free(readBuffer);
	free(writeBuffer);
	///Y cerramos el socket...
	//debug(2,"Close connection (%s:%u)",ip,port);
	closeTCPSocket(socket);
		
	return 0;
}

int sendLine(const int clientSocket, const char* writeBuffer) {
	u_int writeBytes = 0;
	   //Esta funcion escribe en el socket ClientSocket lo que esté escrito en el buffer writeBuffer
	do { 
	writeBytes += write(clientSocket,writeBuffer+writeBytes,strlen(writeBuffer)-writeBytes);
	} while(writeBytes < strlen(writeBuffer));
	//debug(4,"---> %s",writeBuffer);	

	return writeBytes;
}


//////////COMIENZAN FUNCIONES DE TCP
int newTCPServerSocket4(const char *ip, const u_short port, const int q_size) {
	int socketFD;
	int status;
	int localerror;
	struct sockaddr_in addr;

	if(!buildAddr4(&addr,ip,port)) return -1;
	
	if(!(socketFD = getNewTCPSocket(PF_INET))==-1) return -1;
	
	status = bind(socketFD, (struct sockaddr*)&addr, sizeof(addr));
	if(status != 0) {
		localerror = errno;
		fprintf(stderr,"Error: Can't bind port %s:%i (%s)\n",ip,port,strerror(localerror));
		return -1;
	}
	
	status = listen(socketFD,q_size);
	if(status != 0) {
		localerror = errno;
		fprintf(stderr,"Error: Can't change socket mode to listen (%s)\n",strerror(localerror));
		return -1;
	}
	
	//debug(4,"Socket on %s:%u created",ip,port);
	
	return socketFD;
}

int buildAddr4(struct sockaddr_in *addr, const char *ip, const u_short port) {
	
	int status;
	int localerror;

	bzero(addr, sizeof(addr));
	addr->sin_family = AF_INET;
	status = inet_pton(AF_INET,ip,&(addr->sin_addr.s_addr));
	if(status == 0) {
		fprintf(stderr,"Invalid IPv4 Address\n");
		return 0;
	} else if(status == -1) {
		localerror = errno;
		fprintf(stderr,"Error on IP Address (%s)\n",strerror(localerror));
		return 0;
	}
	
	addr->sin_port = htons(port);
	
	return 1;	
}

void closeTCPSocket(const int socketFD) {
	close(socketFD);
	//debug(6,"Socket(%i) closed",socketFD);
	return;
}

int getNewTCPSocket(const int addrType) {
	int socketFD;
	int localerror;
	
	socketFD = socket(addrType,SOCK_STREAM,0);
	if(socketFD == -1) {
		localerror = errno;
		fprintf(stderr,"Can't create socket (%s)\n",strerror(localerror));
		return -1;
	}
	
	return socketFD;
}

int waitConnection4(int socket, char *clientIP, u_int *clientPort) {
	int client;
	struct sockaddr_in addrClient;
	socklen_t addrLen;
	char ip[INET_ADDRSTRLEN]={0};
	int localerror;
	
	addrLen = sizeof(addrClient);
	
	bzero(&addrClient, sizeof(addrClient));
	client = accept(socket, (struct sockaddr *)&addrClient,&addrLen);
	if(client == -1) {
		localerror = errno;
		fprintf(stderr,"Can't retrive client Socket (%s)\n",strerror(localerror));
		return -1;
	}
	
	if(clientIP!=NULL) {
		inet_ntop(AF_INET,&(addrClient.sin_addr),ip,INET_ADDRSTRLEN);
		strcpy(clientIP,ip);		
	}
	
	if(clientPort!=NULL) {
		*clientPort = ntohs(addrClient.sin_port);
	}
	
	return client;
}

int newTCPClientSocket4(const char *ip, const u_short port) {
	int clientSocket;
	int status;
	int localerror;
	struct sockaddr_in addr;
	
	if(!buildAddr4(&addr,ip,port)) return -1;
	
	if((clientSocket = getNewTCPSocket(PF_INET))==-1) return -1;
	
	status = connect(clientSocket, (struct sockaddr*)&addr, sizeof(addr));
	if(status == -1) {
		localerror = errno;
		fprintf(stderr,"Can't connect to %s:%i (%s)",ip,port,strerror(localerror));
		return -1;
	}
	
	//debug(3,"Connected to %s:%i",ip,port);
	fprintf(stderr,"Connected to %s:%i\n",ip,port);
	
	return clientSocket;
}
//////////FINALIZAN FUNCIONES DE TCP

int start_client(const u_short port, const char ip[], const char remoteFilename[], const char localFilename[]) {  //Esta funcion inicia el cliente si no hay errores al crear el socket del cliente 

	int socket;
	socket = newTCPClientSocket4(ip, port);
	
	if(socket == -1) { //Si hubo un error al crear el socket, regresa un 1
		return 1;
	}
	//si no hubo errores, iniciamos el protocolo 
	beginProtocol(socket,remoteFilename,localFilename);
	closeTCPSocket(socket); //cerramos el socket
	//debug(2,"Close connection (%s:%u)",ip,port);
	return 0;
			
}

void beginProtocol(const int socket,const char *remoteFilename,const char *localFilename) {

	char writeBuffer[1024];
	char readBuffer[10240];
	int readBytes;
	char *commandStr;
	//char *freecommandStr;
	char *end;
	char *token;
	//Para iniciar, mandamos un GET con el archivo especificado en remoteFilename
	sprintf(writeBuffer,"GET %s\r\n\r\n",remoteFilename);
	sendLine(socket,writeBuffer);
	
	//leemos la respuesta al GET
	commandStr = (char *) calloc(1,254);
	while((readBytes = read(socket,readBuffer,1))>0) { //leemos byte por byte readBuffer
		commandStr = (char *) realloc(commandStr,strlen(commandStr)+readBytes+1);
		strncat(commandStr,readBuffer,readBytes); //concatenamos en commandStr lo que se haya leido
		end = commandStr+(strlen(commandStr)-4);//colocamos end 4 bytes antes del final de commandStr
		if(strcmp(end,"\r\n\r\n")==0) {//si hemos llegado al final del bufer:
			*end = '\0'; //Colocamos un caracter nulo
			break; //Solo entonces dejamos de leer
		}
	}
	
	//freecommandStr = commandStr;
	//debug(4,"Received: %s\n",commandStr); //Imprimimos el header que recibimos
	token = strsep(&commandStr,"\r"); // = strtok()
	commandStr++; 
	if(strcmp(token,"NOT_FOUND")==0) { //Si el server envió un header NOT_FOUND...
		fprintf(stderr,"ERROR: File not Found (%s)",token); //mostramos el error en pantalla
		return;
	} else if(strcmp(token,"OK")==0) { //Si se recibió un OK del server, 
		token = strstr(commandStr,":"); //separa hasta los : y en los sig. 2 Bytes... 
		processGET(socket,localFilename,strtol(token+2,NULL,10)); //Convierte la cadena a un long
	} else {
		fprintf(stderr,"ERROR UNKOWN ANSWER (%s)",token);
	}
	
	sendLine(socket,"QUIT\r\n\r\n");
	//free(freecommandStr);
}

void processGET(int socket, const char *localFilename, unsigned long size) {
	
	u_int writeBytes = 0,readBytes = 0;
	char *readBuffer;
	u_long totalReadBytes = 0;
	int fileDesc,localError;
	u_long inicio,fin;
		//Mostramos el GET con el nombre del archivo local y su tamaño en bytes
	fprintf(stderr,"Doing GET for: %s (%li Bytes)",localFilename,size); //abrimos el archivo recibido
	fileDesc = open(localFilename,O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
	if(fileDesc == -1) {
		//Error al crear el archivo local.
		localError = errno;
		fprintf(stderr,"Can't create Local File (%s)",strerror(localError));
		return;
	}
	//Si no hubo error en la transferencia del archivo, lo escribimos en un archivo después de leerlo 
	readBuffer = (char *) malloc(102400); 
	inicio = currentTimeMillis();	//comienza la cuenta de milisegundos
	while(totalReadBytes < size) {
		readBytes = read(socket,readBuffer,102400);
		if(readBytes > 0) {
			writeBytes = 0;
			while(writeBytes < readBytes) {
				writeBytes += write(fileDesc,readBuffer+writeBytes,readBytes-writeBytes);
			}
		}
		totalReadBytes += readBytes;
		fin = currentTimeMillis(); //termina la cuenta
		if(fin>inicio) //Imprimimos la informacion de la transferencia del archivo
			fprintf(stderr,"Get %lu/%lu ( %0.0f%% ) speed %lukbps",totalReadBytes,size,(((float)totalReadBytes/(float)size)*100),(totalReadBytes/(fin-inicio)*1000/1024));		
	}	
	
	free(readBuffer); //liberamos el bufer
	close(fileDesc); //y cerramos el archivo

}

u_long currentTimeMillis() { //esta funcion devuelve los milisegundos transcurridos en un intervalo
    u_long t;
    struct timeval tv;
    gettimeofday(&tv, 0);
    t = (tv.tv_sec*1000)+tv.tv_usec;
    return t;
}
