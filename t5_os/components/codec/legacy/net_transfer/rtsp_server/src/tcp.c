/*
 * @Author: cijliu
 * @Date: 2021-02-11 14:17:59
 * @LastEditTime: 2021-02-23 18:01:54
 */
#include "include/net.h"
int tcp_server_init(tcp_t *tcp, int port)
{
    memset(tcp, 0, sizeof(tcp_t));
    tcp->sock = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp->sock == -1) {
		printf("create socket failed : %d\n", errno);
		return -1;
	}
    memset(&tcp->addr, 0, sizeof(struct sockaddr_in));
	tcp->addr.sin_family = AF_INET;
	tcp->addr.sin_addr.s_addr = htonl(INADDR_ANY);
	tcp->addr.sin_port = htons(port);

    int opt = 1;
    setsockopt(tcp->sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
	int ret = bind(tcp->sock, (struct sockaddr*)&tcp->addr, sizeof(struct sockaddr_in));
	if (ret) {
		printf("bind socket to address failed : %d\n", errno);
		close(tcp->sock);
		return -1;
	}
    ret = listen(tcp->sock, TCP_MAX_CLIENT); //XXX
	if (ret) {
		printf("listen socket failed : %d\n", errno);
		close(tcp->sock);
		return -1;
	}
    tcp->port = port;
    return 0;
}

int tcp_server_wait_client(tcp_t *tcp)
{
    int i = 0;
    socklen_t addrlen = sizeof(tcp->addr);
    while(tcp->client[i] != 0 && i < TCP_MAX_CLIENT)i++;
    tcp->client[i] = accept(tcp->sock, (struct sockaddr*)&tcp->addr, &addrlen);
    return tcp->client[i];
    
}

int tcp_server_close_client(tcp_t *tcp, int client)
{
    int i;
    while(tcp->client[i] != client && i < TCP_MAX_CLIENT) i++;
    close(tcp->client[i]);
    tcp->client[i] = 0;
    return 0;
}

int tcp_server_send_msg(tcp_t *tcp, int client, unsigned char *data, int len)
{
    int i = 0;
    while(tcp->client[i] != client && i < TCP_MAX_CLIENT) i++;
    if(i >= TCP_MAX_CLIENT) {
        return -1;
    }
    return send(client, data, len, 0);
}

int tcp_server_receive_msg(tcp_t *tcp, int client, unsigned char *data, int len)
{
    int i = 0;
    while(tcp->client[i] != client && i < TCP_MAX_CLIENT) i++;
    if(i >= TCP_MAX_CLIENT) {
        return -1;
    }
    return read(client, data, len);
}

int tcp_server_deinit(tcp_t *tcp)
{
    int i;
    for(i=0; i < TCP_MAX_CLIENT; i++){
        if(tcp->client[i] != 0){
            close(tcp->client[i]);
            tcp->client[i] = 0;
        }
    }
    close(tcp->sock);
    return 0;
}

