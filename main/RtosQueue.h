#ifndef RTOSQUEUE_H
#define RTOSQUEUE_H
#include "Erc.h"
#include <Xdr.h>
#ifdef ESP32_IDF
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#endif
#ifdef ESP_OPEN_RTOS
#include <FreeRTOS.h>
#include <queue.h>
#endif

class RtosQueue {

	private:
		QueueHandle_t _queue;
	public:
		RtosQueue(uint32_t size) ;
		virtual ~RtosQueue() ;
		int enqueue(Xdr& );
		int dequeue(Xdr&);
		bool hasData(uint32_t sz=1) ;
		bool hasSpace(uint32_t) ;
		int wait(uint32_t delay);


};

#endif // RTOSQUEUE_H
