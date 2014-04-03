/**
  @file tcp.h
  @brief Funciones generales para trabajar con un Socket de tipo TCP

  @author Alvaro Parres
  @date Feb/2013

*/

#ifndef TCP_H
#define TCP_H

int newTCPServerSocket4(const char *ip, const u_short port, const int q_size=5);
void closeTCPSocket(const int socketFD);
int waitConnection4(int socket, char *clientIP, u_int *clientPort);
int newTCPClientSocket4(const char *ip, const u_short port);

#endif
