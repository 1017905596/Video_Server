#include "Socket.h"

Socket::Socket(){
	m_nSocket = -1;
	m_IsV6 = 0;
}

Socket::~Socket(){
	Colse();
}

void Socket::SetSocketTimeOut(int nTimeOut)
{
	struct timeval tv;
	
	tv.tv_usec = (nTimeOut % 1000) * 1000;
	tv.tv_sec = nTimeOut / 1000;
	setsockopt(m_nSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));
	setsockopt(m_nSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));
}

int Socket::set_buf_size(){
	int buf_size = 128*1024;
	socklen_t buf_len = sizeof(buf_size);

	setsockopt(m_nSocket, SOL_SOCKET, SO_SNDBUF, (char *)&buf_size, buf_len );
	buf_size = 0;
	getsockopt(m_nSocket, SOL_SOCKET, SO_SNDBUF,  (char *)&buf_size, &buf_len );
	user_log_printf("snd_buf len:%d\n",buf_len);
	return 0;
}

void Socket::set_socket_nonblock(int type){
	int opt = 1;
	setsockopt(m_nSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

	int val = fcntl(m_nSocket, F_GETFL, 0 );
	if (val == -1)
		return;
	if( type )
	{
		val |= O_NDELAY;        /* 设置成非阻塞 */
	}
	else
	{
		val &= ~O_NDELAY;        /* 取消非阻塞标志（无论之前设置与否） */
	}
	fcntl(m_nSocket, F_SETFL, val );
}

int Socket::Create(const char *host,int sock_type){
	struct addrinfo hint;
	struct addrinfo *addrRet = NULL;
	m_IsV6 = 0;

	memset(&hint,0,sizeof(hint));
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(host, NULL, &hint, &addrRet) != 0){
		user_log_printf("Create:%s,%s\n", host, gai_strerror(errno));
		return m_nSocket;
	}
	if (addrRet->ai_family == AF_INET6){
		memcpy(&m_addrV6, (struct sockaddr_in6*)addrRet->ai_addr, sizeof(m_addrV6));
		m_nSocket = socket(AF_INET6, sock_type, 0);
		m_IsV6 = 1;
	}else{
		memcpy(&m_addrV4, (struct sockaddr_in*)addrRet->ai_addr, sizeof(m_addrV4));
		m_nSocket = socket(AF_INET, sock_type, 0);
	}
	user_log_printf("host:%s,ip:%s,port:%d\n",host,inet_ntoa(m_addrV4.sin_addr),ntohs(m_addrV4.sin_port));
	freeaddrinfo(addrRet);
	return m_nSocket;
}

int Socket::Create(int family,int sock_type){
	m_nSocket = socket(family, sock_type, 0);

	return m_nSocket;
}

int Socket::Connect(unsigned int server_port, int timeout){
	int ret = -1;
	user_log_printf("Connect:%s:%d \n",inet_ntoa(m_addrV4.sin_addr) , server_port);
	if(m_IsV6){
		m_addrV6.sin6_port = htons(server_port);
		ret = connect(m_nSocket, (struct sockaddr *)&m_addrV6, sizeof(m_addrV6));
	}else{
		m_addrV4.sin_port = htons(server_port);
		ret = connect(m_nSocket,(struct sockaddr *) &m_addrV4,sizeof(m_addrV4));
	}
	SetSocketTimeOut(timeout);
	return ret;
}

int Socket::Listen(int backlog){
	return listen(m_nSocket,backlog);
}

int Socket::Accept(){
	struct sockaddr_in Addr;
	socklen_t nAddrLen = sizeof(Addr);
	memset(&Addr, 0, sizeof(sockaddr_in));
	Addr.sin_family = AF_INET;
	int ret = accept(m_nSocket,(struct sockaddr *)&Addr, &nAddrLen);
	if(ret != -1){
		client_ip.c_str();
		client_ip.append(inet_ntoa(Addr.sin_addr));
		user_log_printf("client:ip:%s,port:%d\n",client_ip.c_str(),ntohs(Addr.sin_port));
	}
	return ret;
}

int Socket::Receive(unsigned char* pBuffer, int nLen){
	return recv(m_nSocket, (char *)pBuffer, nLen, 0);
}

int Socket::Send(const void * pBuffer, int nLen){
	return send(m_nSocket, (char *)pBuffer, nLen, 0);
}

int Socket::RecvFrom(char * pBuffer,int nLen,unsigned short *nPort,char *pszIp){
	int nRet = -1;
	if (m_IsV6){
		struct sockaddr_in6 RecvAddr;
		socklen_t nAddrLen = sizeof(RecvAddr);
		memset(&RecvAddr, 0, sizeof(sockaddr_in6));
		RecvAddr.sin6_family = AF_INET6;
		RecvAddr.sin6_port = 0;
		nRet = recvfrom(m_nSocket, pBuffer, nLen, 0, (struct sockaddr *)&RecvAddr, &nAddrLen);
		if (nRet > 0 && pszIp != NULL){
			inet_ntop(AF_INET6, &RecvAddr.sin6_addr, pszIp, 64);
			*nPort = ntohs(RecvAddr.sin6_port);
		}
	}else{
		struct sockaddr_in RecvAddr;
		socklen_t nAddrLen = sizeof(RecvAddr);
		memset(&RecvAddr, 0, sizeof(sockaddr_in));
		RecvAddr.sin_family = AF_INET;
		RecvAddr.sin_port = 0;
		RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		nRet = recvfrom(m_nSocket, pBuffer, nLen, 0, (struct sockaddr *)&RecvAddr, &nAddrLen);
		if (nRet > 0 && pszIp != NULL){
			strncpy(pszIp, inet_ntoa(RecvAddr.sin_addr),sizeof(struct sockaddr_in));
			*nPort = ntohs(RecvAddr.sin_port);
		}
	}
	return nRet;
}

int Socket::SendTo(char * pBuffer,int nLen,const char* pszHost, unsigned short nPort){
	if (m_IsV6){
		struct sockaddr_in6 RecvAddr;
		memset(&RecvAddr, 0, sizeof(sockaddr_in));
		RecvAddr.sin6_family = AF_INET6;
		RecvAddr.sin6_port = htons(nPort);
		if(inet_pton(AF_INET6, pszHost, &RecvAddr.sin6_addr) < 0)
		{
			return -1;
		}
		return sendto(m_nSocket, pBuffer, nLen, 0, (struct sockaddr *)&RecvAddr, sizeof(RecvAddr));
	}else{
		struct sockaddr_in RecvAddr;
		memset(&RecvAddr, 0, sizeof(sockaddr_in));
		RecvAddr.sin_family = AF_INET;
		RecvAddr.sin_port = htons(nPort);
		if(inet_pton(AF_INET, pszHost, &RecvAddr.sin_addr) < 0)
		{
			return -1;
		}
		return sendto(m_nSocket, pBuffer, nLen, 0, (struct sockaddr *)&RecvAddr, sizeof(RecvAddr));
	}
}

int Socket::Bind(unsigned int client_port){
	if(m_IsV6){
		memset(&bind_addrv6, 0, sizeof(sockaddr_in6));
		bind_addrv6.sin6_family = AF_INET6;
		bind_addrv6.sin6_port = htons(client_port);
		return bind(m_nSocket, (struct sockaddr *)&bind_addrv6, sizeof(bind_addrv6));
	}else{
		memset(&bind_addrv4, 0, sizeof(sockaddr_in));
		bind_addrv4.sin_family = AF_INET;
		bind_addrv4.sin_port = htons(client_port);
		bind_addrv4.sin_addr.s_addr = htonl(INADDR_ANY);
		return bind(m_nSocket,(sockaddr *)&bind_addrv4,sizeof(bind_addrv4));
	}
}

int Socket::get_sock_name(unsigned short *nPort,char *pszIp){

	socklen_t nAddrLen = sizeof(bind_addrv4);
	getsockname(m_nSocket,(struct sockaddr *)&bind_addrv4, &nAddrLen);
	if(pszIp != NULL){
		strncpy(pszIp,inet_ntoa(bind_addrv4.sin_addr),sizeof(struct sockaddr_in));
	}
	if(nPort != NULL){
		*nPort = ntohs(bind_addrv4.sin_port);
	}

	return 0;
}

void Socket::Colse(){
	if (m_nSocket != -1)
	{
		close(m_nSocket);
		m_nSocket = -1;
	}
}

