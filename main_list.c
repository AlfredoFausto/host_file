#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "defaults.h"
#include "utils/debug.h"
#include "client.h"

char ip[19] = "";
u_short port = CONFIG_DEFAULT_PORT;
int debugLevel = CONFIG_DEFAULT_VERBOSE;

int processArguments(int argc, char* argv[]);
void printHelp(const char *cmd, const short error, const char *msg="");

int main(int argc, char *argv[]) {	
	
	printf("--Cliente LIST del File Transfer\n");
	if(!processArguments(argc, argv)) return -1;
	
	get_LIST(port,ip);
}

int processArguments(int argc, char* argv[]) {

	int i;
	u_int temp;
	
	if(argc < 2) {
		printHelp(argv[0],true,"Missing Arguments");
		return false;
	}
		
	for(i=1; i<argc-3; i++) {
		if(strcmp(argv[i],"-h")==0) {
			printHelp(argv[0],false);
			return true;
		}	
		
		if(strcmp(argv[i],"-p")==0) {
			temp = strtoimax(argv[++i], NULL, 10);
			if(temp == 0 || temp < MINPORT || temp > MAXPORT) {
				printHelp(argv[0],true,"Port out of range\n");
				return false;
			}
			port = temp;
		} else if(strstr(argv[i],"-v")!=NULL) {
			debugLevel = strlen(argv[i])-1;
		} else {
			printHelp(argv[0],true,"Unkown option\n");
			return false;
		}	
	}
	
	strncpy(ip,argv[i],18);
	
	debug(2,"%s","Configuraciones:");
	debug(2,"Puerto:\t\t%u",port);
	debug(2,"IP Server:\t%s",ip);		
	return true;
}

void printHelp(const char *cmd, const short error, const char *msg) {
	
	if(error) {
		fprintf(stderr,"Error: %s\n\n",msg);
	}
	
	printf("Use:\t%s [options] IP_ADDR\n", cmd);
	printf("\n");
	printf("Options:\n");
	printf("\t-p PORT_NUMBER\t Set port number to work on (Default %u)\n",CONFIG_DEFAULT_PORT);
	printf("\t-v[vvvv]\t Increments the verbosity of the server (Default %u)\n",CONFIG_DEFAULT_VERBOSE);
	printf("\t-h\t Prints this help message\n");
	printf("\n");
}
