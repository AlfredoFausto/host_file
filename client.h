/**
  @author Alvaro Parres
  @date Feb/2013

*/


#ifndef CLIENT_H
#define CLIENT_H

int start_client(const u_short port, const char ip[], const char remoteFilename[], const char localFilename[] );
int get_LIST(const u_short port, const char ip[]);

#endif
