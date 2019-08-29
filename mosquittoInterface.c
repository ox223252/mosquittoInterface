#include "mosquittoInterface.h"
#include <pthread.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "../log/log.h"
#include "../freeOnExit/freeOnExit.h"

// to install mosquitto :
// apt install libmosquitto-dev
// or
// git clone https://github.com/eclipse/mosquitto.git
// make
// make install
//
// install mosquitto for cross compilation
// need openssl
// git clone https://github.com/openssl/openssl.git
// cd openssl/
// ./Configure linux-armv4 --prefix=/usr/arm-linux-gnueabihf CROSS_COMPILE=arm-linux-gnueabihf-
// make 
// make install
// git clone https://github.com/eclipse/mosquitto.git
// cd mosquitto
// make CROSS_COMPILE=arm-linux-gnueabihf- CC=gcc CXX=g++ AR=ar LD=ld
// cp lib/libmosquitto.so.1 /usr/arm-linux-gnueabihf/lib/libmosquitto.so.1
// ln -s /usr/arm-linux-gnueabihf/lib/libmosquitto.so.1 /usr/arm-linux-gnueabihf/lib/libmosquitto.so
// cp lib/mosquitto.h /usr/arm-linux-gnueabihf/include

static bool ackDone = false;

typedef struct
{
	void (*on_message)();
	void * arg;
}
_bigBoy_init_t;

typedef struct
{
	struct mosquitto *mosq;
	const char *topic;
	void *arg;
	uint8_t *stop;
	char* (*callback)( void * arg );
	uint32_t time;
}
_bigBoy_sender_t;

#pragma GCC diagnostic ignored "-Wunused-parameter"
static void disconnectCallback ( struct mosquitto * restrict mosq, void * restrict obj, int result )
{
	if ( !result )
	{
		ackDone = false;
	}
}
static void connectCallback ( struct mosquitto * restrict mosq, void * restrict obj, int result )
{
	if ( !result )
	{
		ackDone = true;
	}
}
static void messageCallback ( struct mosquitto * restrict mosq, void * arg, const struct mosquitto_message * m)
{
	_bigBoy_init_t *s = arg;

	if ( !s )
	{
		return;
	}
	s->on_message ( m->topic, m-> payload, s->arg );
}
#pragma GCC diagnostic pop



int bigBoyMQTT_init ( const MQTT_init_t s, struct mosquitto ** mosq, void (*fnc)(char*,char*,void*), void * arg )
{
	int rt = 0;

	if ( !mosq )
	{
		return ( __LINE__ );
	}

	if ( mosquitto_lib_init( ) )
	{
		return ( __LINE__ );
	}

	_bigBoy_init_t *init = NULL;
	if ( fnc )
	{
		init = malloc ( sizeof ( *init ) );
		if ( !init )
		{
			rt = __LINE__;
			goto lClean;
		}
		setFreeOnExit ( init );
		init->on_message = fnc;
		init->arg = arg;
	}

	if ( s.name )
	{
		*mosq = mosquitto_new ( s.name, true, init );
	}
	else
	{
		*mosq = mosquitto_new ( "bigBoy", true, init );
	}

	if ( !*mosq )
	{
		rt = __LINE__;
		goto lClean;
	}


	mosquitto_connect_callback_set ( *mosq, connectCallback );
	mosquitto_disconnect_callback_set ( *mosq, disconnectCallback );
	mosquitto_message_callback_set( *mosq, messageCallback );

	// set the msg provided to broker in failure case
	if ( s.lastName && 
		s.lastMsg &&
		mosquitto_will_set ( *mosq, s.lastName, strlen( s.lastMsg )+1, s.lastMsg, 0, 0 ) )
	{
		rt = __LINE__;
		goto lDestroy;
	}

	// set ssl security
	if ( s.ca && 
		s.cert && 
		s.key &&
		( mosquitto_tls_opts_set ( *mosq, 1, NULL, NULL ) ||
		mosquitto_tls_set ( *mosq, s.ca, NULL, s.cert, s.key, NULL ) ||
		mosquitto_tls_insecure_set ( *mosq, true ) ) )
	{
		rt = __LINE__;
		goto lDestroy;
	}

	if ( mosquitto_connect ( *mosq, s.host, s.port, 1000000 ) )
	{
		rt = __LINE__;
		goto lDestroy;
	}

	if ( mosquitto_loop_start ( *mosq ) )
	{
		rt = __LINE__;
		goto lDestroy;
	}

	return ( 0 );

lDestroy:
	mosquitto_destroy ( *mosq );
lClean:
	mosquitto_lib_cleanup ( );

	return ( rt );
}


int bigBoyMQTT_stop ( struct mosquitto ** mosq )
{
	if ( !mosq )
	{
		return (  0 );
	}

	mosquitto_loop_stop ( *mosq, true );
	mosquitto_destroy ( *mosq );
	*mosq = NULL;
	mosquitto_lib_cleanup ( );

	return ( 0 );
}

static void * bigBoyMQTT_senderSubRoutine ( void * arg )
{
	_bigBoy_sender_t *s = arg;
	while ( !*s->stop )
	{
		if ( ackDone )
		{
			char *str = s->callback ( s->arg );

			if ( !str )
			{
				continue;
			}

			mosquitto_publish ( s->mosq, NULL, s->topic, strlen ( str ), str, 0, false );
		}
		usleep ( s->time * 1000 );
	}
	return ( NULL );
}

int bigBoyMQTT_sender ( struct mosquitto * mosq, const char* topic, uint8_t * stop, void * data, char *callback( void* arg ), uint32_t time )
{
	_bigBoy_sender_t *s = NULL;

	s = malloc ( sizeof (_bigBoy_sender_t) );
	if ( !s )
	{
		return ( __LINE__ );
	}

	setFreeOnExit ( s );

	s->mosq = mosq;
	s->topic = topic;
	s->stop = stop;
	s->arg = data;
	s->callback = callback;
	s->time = time;

	pthread_t thread;

	if ( pthread_create( &thread, 0, bigBoyMQTT_senderSubRoutine, s ) )
	{
		return ( __LINE__ );
	}
	else
	{
		// setThreadKillOnExit ( thread );
		// setThreadJoinOnExit ( thread );
		setThreadCancelOnExit ( thread );
	}
	return ( 0 );
}

