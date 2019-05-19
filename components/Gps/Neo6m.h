#ifndef NEO6M_H
#define NEO6M_H
#include <Hardware.h>
#include <Log.h>
#include <Akka.h>
#include <Publisher.h>
class Neo6m : public Actor {
		Connector* _connector;
		UART& _uart;
		static void onRxd(void*);
		ActorRef& _publisher;
		Label _measureTimer;
		std::string _line;
	public:
		Neo6m(Connector* connector,ActorRef& mqtt);
		virtual ~Neo6m();
		int init();
		void handleRxd();
		void preStart();
		Receive& createReceive();
};

#endif // NEO6M_H
