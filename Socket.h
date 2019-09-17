#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "userlib_type.h"

#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class Socket{
public:
	Socket();
	virtual ~Socket();

public:
	int Create(const char *host,int sock_type);
	int Create(int family,int sock_type);
	int Bind(unsigned int client_port);
	int Connect(unsigned int server_port, int timeout);
	int Listen(int backlog);
	int Accept();
	int Receive(unsigned char* pBuffer, int nLen);
	int Send(const void * pBuffer, int nLen);
	int RecvFrom(char * pBuffer,int nLen,unsigned short *nPort,char *pszIp);
	int SendTo(char * pBuffer,int nLen,const char* pszHost, unsigned short nPort);
	void set_socket_nonblock(int type);
	int set_buf_size();
	void Colse();
	int get_sock_name(unsigned short *nPort,char *pszIp);
protected:
	void SetSocketTimeOut(int nTimeOut);
public:
	int m_nSocket;
	string client_ip;
protected:
	int m_IsV6;
	struct sockaddr_in bind_addrv4;
	struct sockaddr_in6 bind_addrv6;
	struct sockaddr_in	m_addrV4;
	struct sockaddr_in6 m_addrV6;
};

#endif