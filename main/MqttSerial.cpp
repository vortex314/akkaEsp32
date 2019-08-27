#include <MqttSerial.h>
#include <Wifi.h>
#include <sys/types.h>
#include <unistd.h>
#include <Config.h>
#include "driver/uart.h"
#include "esp_log.h"
#include <Hardware.h>

#define UART_NUM UART_NUM_0
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t uart0_queue;


// volatile MQTTAsync_token deliveredtoken;
#define BZERO(x) ::memset(&x, 0, sizeof(x))


MqttSerial* MqttSerial::_me;

MqttSerial::MqttSerial( ) :_uart(UART::create(UART_NUM_0,1,3)) {
	_connected = false;
	config.setNameSpace("mqtt");
	_me=this;
}

MqttSerial::~MqttSerial() {
}

MsgClass MqttSerial::PublishRcvd("mqttPublishRcvd");
MsgClass MqttSerial::Connected("mqttConnected");
MsgClass MqttSerial::Disconnected("mqttDisconnected");
MsgClass MqttSerial::Publish("mqttPublish");
MsgClass MqttSerial::Subscribe("mqttSubscribe");


void MqttSerial::preStart() {
	DEBUG(" MQTT-Serial preStart");
	_uart.setClock(115200);
	_uart.onRxd(onRxd,this);
	_uart.mode("8N1");
	_uart.init();
	logger.setOutput(MqttSerial::uartLogger); // redirect logs to same UART buffer, to avoid mixup of lines
	_connected=false;
	timers().startPeriodicTimer("T1", Msg("connectTimer"), 1000);
	timers().startPeriodicTimer("T2", Msg("checkConnectTimer"), 1000);

	eb.publish(Msg(MqttSerial::Connected).src(self().id()));
	_loopbackTopic =  "dst/";
	_loopbackTopic += context().system().label();
	_loopbackTopic += "/loopback";
	_loopbackCount =0;
}


Receive& MqttSerial::createReceive() {
	return receiveBuilder()

	.match(MqttSerial::Publish, [this](Msg& msg) {
		if ( _connected==false ) return;
		std::string topic;
		std::string message;
		while ( msg.getNext("topic",topic) && msg.getNext("message",message) ) {
			mqttPublish(topic.c_str(),message.c_str());
		}
	})
	.match(MqttSerial::Subscribe, [this](Msg& msg) {
		if ( _connected==false ) return;
		std::string topic;
		while ( msg.getNext("topic",topic)  ) {
			mqttSubscribe(topic.c_str());
		}
	})

	.match(MsgClass::Properties(), [this](Msg& msg) {
		sender().tell(Msg(MsgClass::PropertiesReply())
		              ("clientId",_clientId)
		              ,self());
	})

	.match(MsgClass(H("checkConnectTimer")), [this](Msg& msg) {
		if ( _loopbackCount < -2 ) updateConnected(false);
		_loopbackCount--;
	})

	.match(MsgClass(H("connectTimer")), [this](Msg& msg) {
		if ( _connected ==false )  {
			std::string topics;
			topics = "dst/";
			topics += context().system().label();
			mqttSubscribe(topics.c_str());
			topics += "/#";
			mqttSubscribe(topics.c_str());
		} else {
		}
		mqttPublish(_loopbackTopic.c_str(),"true");
	})

	.build();
}

void MqttSerial::updateConnected(bool connected) {
	if ( _connected ==  connected) return;
	_connected=connected;
	if ( connected ) {
		eb.publish(Msg(MqttSerial::Connected).src(self().id()));
	} else {
		eb.publish(Msg(MqttSerial::Disconnected).src(self().id()));
	}
}

void MqttSerial::handleSerial(uint8_t b) {
	if (( b=='\r' || b=='\n') && ( _line.length()>0) ) {
		_jsonDocument.clear();
		deserializeJson(_jsonDocument,_line);
		if ( _jsonDocument.is<JsonArray>()) {
			JsonArray array = _jsonDocument.as<JsonArray>();
			int cmd=array[1];
			if ( cmd==PUBLISH && array.size()>4) {
				std::string topic=array[2];
				std::string message=array[3];
				uint32_t qos=array[4];
				bool retained =  array[5]==1;
				INFO(" MQTT RXD : %s = %s ",topic.c_str(),message.c_str());
				if ( topic == _loopbackTopic) {
					_loopbackCount=0;
					updateConnected(true);
				} else {
					Msg pub(PublishRcvd);
					pub("topic", topic)("message", message).src(self().id());
					eb.publish(pub);
				}
			}
		} else {
			WARN(" received invalid JSON Array : %s ",_line.c_str());
		}
		_line.clear();
	} else {
		_line+=(char)b;
	}
}

void MqttSerial::onRxd(void* me) {
	Bytes bytes(100);
	MqttSerial* mqttSerial=(MqttSerial*)me;
	while(mqttSerial->_uart.hasData()) {
		mqttSerial->_uart.read(bytes);
		bytes.offset(0);
		while ( bytes.hasData()) mqttSerial->handleSerial(bytes.read());
	}
}

void MqttSerial::uartLogger(char* s,uint32_t length) {
	_me->_uart.write((uint8_t*)s,length);
	_me->_uart.write((uint8_t*)"\r\n",2);
}





void MqttSerial::mqttPublish(const char* topic, const char* message) {
	INFO(" pub %s = %s ",topic,message);
	DynamicJsonDocument doc(2038);
	doc.add("0000");
	doc.add((uint32_t)PUBLISH);
	doc.add(topic);
	doc.add(message);
	doc.add(0);
	doc.add(0);
	std::string line;
	serializeJson(doc,line);
	line +="\r\n";
	_uart.write((uint8_t*)line.c_str(),line.length());
}

void MqttSerial::mqttSubscribe(const char* topic) {
//	INFO(" sub %s  ",topic);
	DynamicJsonDocument doc(2038);
	doc.add("0000");
	doc.add((uint32_t)SUBSCRIBE);
	doc.add(topic);
	std::string line;
	serializeJson(doc,line);
	line+="\r\n";
	_uart.write((uint8_t*)line.c_str(),line.length());
}
