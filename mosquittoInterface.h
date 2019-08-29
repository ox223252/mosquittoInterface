#ifndef __BIGBOYSERVER_H__
#define __BIGBOYSERVER_H__

#include <mosquitto.h>

#include <pthread.h>
#include <stdint.h>

typedef struct
{
	char * name;
	char * host;
	uint16_t port;

	char * ca;
	char * cert;
	char * key;

	char * lastName;
	char * lastMsg;
}
MQTT_init_t;

int bigBoyMQTT_init ( const MQTT_init_t s, struct mosquitto ** mosq, void (*fnc)(char* topic, char* msg, void * arg), void * arg);
int bigBoyMQTT_stop ( struct mosquitto ** mosq );
int bigBoyMQTT_sender ( struct mosquitto * mosq, const char* topic, uint8_t * stop, void * data, char *callback( void* arg ), uint32_t time );

#endif