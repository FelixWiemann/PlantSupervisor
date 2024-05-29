/*
Etienne Arlaud
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>

#include <assert.h>
#include <unistd.h>
#include <sys/time.h>

#include <thread>

#include "ESPNOW_manager.h"
#include "ESPNOW_types.h"
#include "ic.h"

using namespace std;

static uint8_t my_mac[6] = {0xF8, 0x1A, 0x67, 0xb7, 0xEB, 0x0B};
static uint8_t dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t ESP_mac[6] = {0xB4,0xE6,0x2D,0xB5,0x9F,0x85};

ESPNOW_manager *handler;

uint8_t payload[127];

char addr[100] = {0};
char buck[100] = {0};
char usr[100] = {0};
char tkn[100] = {0};
int port = 0;

void callback(uint8_t src_mac[6], uint8_t *data, int len) {
	handler->mypacket.wlan.actionframe.content.length = 127 + 5;
	memcpy(handler->mypacket.wlan.actionframe.content.payload, data, len);
	char out[20];
    sprintf(out, "humidity,sensor=%d", src_mac[0]);
	ic_measure(out);
	ic_long("value", *(data+2));
	ic_measureend();
	ic_push();
}

void readCfg() {
	FILE* fd = fopen("conf", "r");
	if (fd == NULL) {
		throw std::invalid_argument( "conf not found" );
	}

    char arg[100] = {0};
    char val[100] = {0};
	
	while (fscanf(fd, "%s %s\n", arg, val) == 2) {
		if ( strcmp("ADDRESS", arg) == 0) {
			strcpy(addr, val);
		}
		if ( strcmp("PORT", arg) == 0) {
			port = strtol(val, NULL, 10);
		}
		if ( strcmp("BUCKET", arg) == 0) {
			strcpy(buck, val);
		}
		if ( strcmp("USER", arg) == 0) {
			strcpy(usr, val);
		}
		if ( strcmp("TOKEN", arg) == 0) {
			strcpy(tkn, val);
		}
	}
}

int main(int argc, char **argv) {
	setvbuf(stdout, NULL, _IOLBF, 0);
	
	assert(argc > 1);
	nice(-20);
	readCfg();	

	ic_influx_database(addr, port, buck);
	ic_influx_userpw(usr,tkn);
	ic_tags("");

	handler = new ESPNOW_manager(argv[1], DATARATE_1Mbps, CHANNEL_freq_1, my_mac, dest_mac, false);
	handler->set_filter(ESP_mac, dest_mac);
	handler->set_recv_callback(&callback);
	handler->start();
	printf("started handler\n");
	while(1) {
		sleep(1000);
	}

	handler->end();
}


// wlp5s0