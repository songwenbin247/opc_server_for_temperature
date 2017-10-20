#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <time.h>

#include "mbus.h"
#define pr() //printf("%s %s %d\n", __FILE__, __func__, __LINE__)
extern struct mbus_tcp_func tcp_func;

static int _check_para(struct send_info *info)
{
	if (info->sbuf[6] == (char)0xf1){
		return 0;
	}	
	if (info->sbuf[7] > 6 && info->sbuf[7] < 1){
			printf("Function code :\n");
			printf("1        Read Coil Status\n");
			printf("2        Read Input Status\n");
			printf("3        Read Holding Registers\n");
			printf("4        Read Input Registers\n");
			printf("5        Force Single Coil\n");
			printf("6        Preset Single Register\n");
			return -1;
	}
	
	if(info->sbuf[7] == 5){
		if(info->sbuf[10] && info->sbuf[11]){
			info->sbuf[10] = 0xff;
			info->sbuf[11] = 0x0;
		}
	}	
	return 0;
}

static int _create_sk_cli(char *addr, char *port)
{
	int skfd;
	int ret;
	struct addrinfo hints;
	struct addrinfo *res;
	struct addrinfo *p;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;

	ret = getaddrinfo(addr, port, &hints, &res);
	if(ret != 0){
		printf("<Modbus Tcp Master> getaddrinfo : %s\n", gai_strerror(ret));
		return -1;
	}

	for(p = res; p != NULL; p = p->ai_next){
		skfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(skfd == -1){
			continue;
		}

		ret = connect(skfd, p->ai_addr, p->ai_addrlen);
		if(ret == -1){
			close(skfd);
			continue;
		}
		break;
	}

	if(p == NULL){
		if (skfd > 0)
			close(skfd);
		printf("<Modbus Tcp Master> Fail to connect\n");
 		return -1;
	}

	freeaddrinfo(res);
	
	return skfd;
}


static struct send_info *create_connect(char *ip, char *port)
{
	struct send_info *info = malloc(sizeof(struct send_info));
	if (!info){
		return NULL;
	}
	info->ip = ip;
	info->port = port;
	info->skfd = _create_sk_cli(ip, port);
	if(info->skfd < 0){
		free(info);
		return NULL;
	}
	return info;
}

static void del_connect(struct send_info *info)
{
	close(info->skfd);
	free(info);
}

static void set_data(struct send_info *info, char *buf, int len)
{
	info->sbuf[0] = INITTCPTRANSID >> 8;
	info->sbuf[1] = INITTCPTRANSID & 0xFF;
	info->sbuf[2] = TCPMBUSPROTOCOL >> 8;;
	info->sbuf[3] = TCPMBUSPROTOCOL & 0XFF;;
	info->sbuf[4] = (len >> 8) & 0xFF;
	info->sbuf[5] = len & 0xFF;
	memcpy(&(info->sbuf[6]), buf, len);
	info->len = len + 6;
}

static int send_date(struct send_info *info, unsigned char *rbuf, int len)
{
	int ret;
	int wlen;
	int rlen;
	struct tcp_frm_para pa;

	ret = _check_para(info);
	if(ret == -1){
		return ret;
	}

         wlen = send(info->skfd, info->sbuf, info->len, MSG_NOSIGNAL);
	 if(wlen != info->len){
		return -1;
	 }

	 if (info->sbuf[6] == (char)0xF1){
		 return 0;
	 }	

	 rlen = recv(info->skfd, rbuf, len, 0);
	 if(rlen < 1){
		return -1;
	 }

	pa.transID = (info->sbuf[0] << 8) + info->sbuf[1];  
	pa.potoID = (info->sbuf[2] << 8) + info->sbuf[3];  
	pa.msglen = (info->sbuf[4] << 8) + info->sbuf[5];  
	pa.unitID = info->sbuf[6];
	pa.fc = info->sbuf[7];
	pa.straddr = (info->sbuf[8] << 8) + info->sbuf[9];
	pa.act = (info->sbuf[10] << 8) + info->sbuf[11];
	pa.len = (info->sbuf[10] << 8) + info->sbuf[11];

	 ret = tcp_func.chk_dest((struct tcp_frm *)rbuf, &pa);
	 if(ret == -1){
		return -1;
	 }
	
	 ret = tcp_func.resp_parser((unsigned char *)rbuf, &pa, rlen);
	 if(ret == -1){
		return -1;
	 }
	
	return ret == -1 ? -1 : rlen;
}	

int temperature = 0;
long long last_time_in_mill = 0;
#define UP_RATE 1000 // 100ms

int get_temperature(char *ip, char *port)
{
	unsigned char rbuf[32];
	struct send_info *info;
	char obuf[] = {0xf1, 0x01, 0x04, 0x09,0x06, 0xfe};
	char sbuf[] = {0x01, 0x03, 0x00, 0x00,0x00, 0x02};
	unsigned int tem;
	int len;
	struct timeval tv;
	long long time_in_mill;
	
	gettimeofday(&tv, NULL);;
	time_in_mill =  (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
	if (time_in_mill - last_time_in_mill < UP_RATE){
		return temperature;
	}
	
	last_time_in_mill = time_in_mill;

	if ((info = create_connect(ip, port)) == NULL){
		printf("Can't create a connect to %s:%s\n", ip, port);
		return 0;
	}
	
	set_data(info, sbuf, sizeof(sbuf));
	len = send_date(info, rbuf, 32);
       	if (len < 0){
		printf("send error\n");
		return 0;
	}

	tem = (rbuf[9] << 8) + rbuf[10];	
	if (tem == 0)
		tem = (rbuf[11] << 8) + rbuf[12];	

	obuf[2] = (tem % 1000) / 100;
	obuf[3] = ((tem % 100) / 10) | 0x80;
	obuf[4] = (tem % 10);

	set_data(info, obuf, sizeof(obuf));
	len = send_date(info, rbuf, 32);
       	if (len < 0){
		printf("send error\n");
		return 0;
	}
	del_connect(info);
	temperature = tem;
	return tem;
	
}

