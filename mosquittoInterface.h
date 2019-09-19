#ifndef __BIGBOYSERVER_H__
#define __BIGBOYSERVER_H__

#include <mosquitto.h>

#include <pthread.h>
#include <stdint.h>


/// \struct MQTT_init_t
typedef struct
{
	char * name; ///< name of topic
	char * host; ///< broker hostname : localhost, 192.168.1.1, hostname.local
	uint16_t port; ///< broker port : 1883, 8883

	char * ca; ///< Authority Certification
	char * cert; ///< certificat file
	char * key; ///< key file

	char * lastName; ///< topic name were the disconnection message wille be 
	    ///< sent
	char * lastMsg; ///< last massage sent on disconnection
}
MQTT_init_t;

////////////////////////////////////////////////////////////////////////////////
/// \fn int bigBoyMQTT_init ( const MQTT_init_t s, struct mosquitto ** mosq,
///     void (*fnc)(char* topic, char* msg, void * arg), void * arg);
/// \param [ in ] s : init struct to feed mosquitto client
/// \param [ out ] mosq : mosq struct use later to subscribe or publish
/// \param [ in ] fnc : function used as receive callback
/// \param [ in ] arg : arg for receive callback
/// \brief init mosquitto client
/// \return 0 if OK else see errno for more details
////////////////////////////////////////////////////////////////////////////////
int bigBoyMQTT_init ( const MQTT_init_t s, struct mosquitto ** mosq,
	void (*fnc)(char* topic, char* msg, void * arg), void * arg);

////////////////////////////////////////////////////////////////////////////////
/// \fn int bigBoyMQTT_stop ( struct mosquitto ** mosq );
/// \param [ in ] mosq : struct provided by bigBoyMQTT_init
/// \brief clean mosquitto
/// \return 0
////////////////////////////////////////////////////////////////////////////////
int bigBoyMQTT_stop ( struct mosquitto ** mosq );

////////////////////////////////////////////////////////////////////////////////
/// \fn int bigBoyMQTT_sender ( struct mosquitto * mosq, const char* topic,
///     uint8_t *stop, void *data, char *callback( void* arg ), uint32_t time );
/// \param [ in ] mosq : struct provided by bigBoyMQTT_init
/// \param [ in ] topic : topic name where function will publish data
/// \param [ in ] stop : var used to stop the sender thread
/// \param [ in ] data : callback data argument
/// \param [ in ] callback : function used to provide string to the sender
/// \param [ time ] time : cycle time (ms)
/// \brief this function will create a thread that will call every X ms the 
///     callback. the callback should return a regular string. this string will
///     be sent to topic.
/// \return 0 if OK else 
////////////////////////////////////////////////////////////////////////////////
int bigBoyMQTT_sender ( struct mosquitto * mosq, const char* topic, 
	uint8_t * stop, void * data, char *callback( void* arg ), uint32_t time );

extern int mosquitto_publish(struct mosquitto *mosq,int *mid,const char *topic,int payloadlen,const void *payload, int qos, bool retain );
extern int mosquitto_subscribe(struct mosquitto *mosq,int *mid,const char *sub,int qos );

#endif