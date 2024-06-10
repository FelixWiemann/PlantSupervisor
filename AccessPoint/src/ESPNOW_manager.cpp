
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_arp.h>
#include <arpa/inet.h>
#include <assert.h>

#include "ESPNOW_manager.h"

#include "ESPNOW_types.h"
#include <thread>

#define MAC_2_MSBytes(MAC)  MAC == NULL ? 0 : (MAC[0] << 8) | MAC[1]
#define MAC_4_LSBytes(MAC)  MAC == NULL ? 0 : (((((MAC[2] << 8) | MAC[3]) << 8) | MAC[4]) << 8) | MAC[5]

void ESPNOW_manager::set_interface(char* interface) {
	this->interface = (char*) malloc(strlen(interface)*sizeof(char));	
	strcpy(this->interface, interface);
}

void ESPNOW_manager::set_recv_callback(void (*callback)(uint8_t src_mac[6], uint8_t *data, int len)) {
	recv_thread_params.callback = callback;
}

void ESPNOW_manager::unset_filter() {
	if(this->bpf.filter != NULL) {
		free(this->bpf.filter);
		this->bpf.filter = NULL;
	}
	this->bpf.len = 0;
}

void ESPNOW_manager::set_filter(uint8_t *src_mac, uint8_t *dst_mac) {
	//sudo tcpdump -i wlp5s0 'type 0 subtype 0xd0 and wlan[24:4]=0x7f18fe34 and wlan[32]=221 and wlan[33:4]&0xffffff = 0x18fe34 and wlan[37]=0x4 and wlan dst 11:22:33:44:55:66 and wlan src 77:88:99:aa:bb:cc' -dd
	unset_filter();

	this->bpf.len = 13;

	struct sock_filter myFilter[13] = {
		{ 0x30, 0, 0, 0x00000003 },
		{ 0x64, 0, 0, 0x00000008 },
		{ 0x7, 0, 0, 0x00000000 },
		{ 0x30, 0, 0, 0x00000002 },
		{ 0x4c, 0, 0, 0x00000000 },
		{ 0x7, 0, 0, 0x00000000 },
		{ 0x50, 0, 0, 0x00000000 },
		{ 0x54, 0, 0, 0x000000fc },
		{ 0x15, 0, 3, 0x000000d0 },
		{ 0x40, 0, 0, 0x00000018 },
		{ 0x15, 0, 1, 0x7f18fe34 },
		{ 0x6, 0, 0, 0x00040000 },
		{ 0x6, 0, 0, 0x00000000 }
	};

	this->bpf.filter = (sock_filter*) malloc(sizeof(sock_filter)*this->bpf.len);
	memcpy(this->bpf.filter, myFilter, sizeof(struct sock_filter) * this->bpf.len);
}


void ESPNOW_manager::start() {
	struct sockaddr_ll s_dest_addr;
    struct ifreq ifr;
	
    int fd, 			//file descriptor
		ioctl_errno,	//ioctl errno
		bind_errno,		//bind errno
		filter_errno,	//attach filter errno
		priority_errno;	//Set priority errno

	bzero(&s_dest_addr, sizeof(s_dest_addr));
    bzero(&ifr, sizeof(ifr));
	
	
    fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    assert(fd != -1);

    strncpy((char *)ifr.ifr_name, this->interface, IFNAMSIZ); //interface

    ioctl_errno = ioctl(fd, SIOCGIFINDEX, &ifr);
    assert(ioctl_errno >= 0);	//abort if error

    s_dest_addr.sll_family = PF_PACKET;
    s_dest_addr.sll_protocol = htons(ETH_P_ALL);
    s_dest_addr.sll_ifindex = ifr.ifr_ifindex;
    
    bind_errno = bind(fd, (struct sockaddr *)&s_dest_addr, sizeof(s_dest_addr));
    assert(bind_errno >= 0);	//abort if error
	
	
	if(bpf.len > 0) {
		filter_errno = setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, &(this->bpf), sizeof(bpf));
		assert(filter_errno >= 0);
	}


	priority_errno = setsockopt(fd, SOL_SOCKET, SO_PRIORITY, &(this->socket_priority), sizeof(this->socket_priority));
	assert(priority_errno ==0);

	this->sock_fd = fd;

	this->recv_thread_params.sock_fd = this->sock_fd;

	pthread_create (&recv_thd_id, NULL, &(ESPNOW_manager::sock_recv_thread), &recv_thread_params);
    
}

void ESPNOW_manager::stop() {
	printf("end socket receive");
	if(recv_thd_id) {
		pthread_cancel(recv_thd_id);
		pthread_join(recv_thd_id, NULL);
	}
	if (this->sock_fd > 0)
    {
        close(this->sock_fd);
    }
}

void ESPNOW_manager::end() {
	stop();

}

void* ESPNOW_manager::sock_recv_thread (void *p_arg)
{
	int raw_bytes_len;
	uint8_t raw_bytes[LEN_RAWBYTES_MAX];

	uint8_t* res_mac;
	uint8_t* res_payload;
	int res_len;

	struct thread_args params = * ((struct thread_args *)p_arg);

	if(params.callback == NULL) {
		printf ("No callback for receive, receive thread exited\n");
    	return EXIT_SUCCESS;
	};
	printf("start socket receive\n");
	while(1)
    {	
		raw_bytes_len = recvfrom (params.sock_fd, raw_bytes, LEN_RAWBYTES_MAX, MSG_TRUNC, NULL, 0);

		if( -1 == raw_bytes_len )
		{
			perror ("Socket receive failed");
			printf("\n");
		}
		else if( raw_bytes_len < 0 )
		{
			perror ("Socket receive, error ");
			printf("\n");
		}
		else
		{
			res_mac = ESPNOW_packet::get_src_mac(raw_bytes,raw_bytes_len);
			res_payload = ESPNOW_packet::get_payload(raw_bytes, raw_bytes_len);
			res_len = ESPNOW_packet::get_payload_len(raw_bytes, raw_bytes_len);
			if(res_mac != NULL && res_payload != NULL && res_len > 0) {
				params.callback(res_mac, res_payload, res_len);
				// reset data after receiving
				for (int i = 0; i < LEN_RAWBYTES_MAX; i++) {
					raw_bytes [i] = 0xff;
				}
			}
		}
    }

    printf ("Receive thread exited \n");
    return EXIT_SUCCESS;
}


/*
int ESPNOW_manager::send(ESPNOW_packet p) {
	uint8_t raw_bytes[LEN_RAWBYTES_MAX];
	int len = p.toBytes(raw_bytes, LEN_RAWBYTES_MAX);

	return sendto(this->sock_fd, raw_bytes, len, 0, NULL, 0);
}
*/


int ESPNOW_manager::send(uint8_t *payload, int len) {
	uint8_t raw_bytes[LEN_RAWBYTES_MAX];

	//Not the most fastest way to do this : 
	//	copy the payload in the packet array and then copy it back into the buffer...
	this->mypacket.wlan.actionframe.content.set_length(len); 
	memcpy(this->mypacket.wlan.actionframe.content.payload, payload, len);

	int raw_len = mypacket.toBytes(raw_bytes, LEN_RAWBYTES_MAX);

	return sendto(this->sock_fd, raw_bytes, raw_len, 0, NULL, 0);
}

int ESPNOW_manager::send() {
	uint8_t raw_bytes[LEN_RAWBYTES_MAX];

	int raw_len = mypacket.toBytes(raw_bytes, LEN_RAWBYTES_MAX);

	return sendto(this->sock_fd, raw_bytes, raw_len, 0, NULL, 0);
}

