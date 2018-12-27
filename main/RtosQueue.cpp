#include "RtosQueue.h"

RtosQueue::RtosQueue(uint32_t size) {
	INFO(" RtosQueue created ");
	_queue = xQueueCreate(size,sizeof(Xdr*));

}

RtosQueue::~RtosQueue() {
}


int RtosQueue::enqueue(Xdr& xdr) {
	Xdr* nx = new Xdr(xdr.size());
	*nx=xdr;
	if ( xQueueSend(_queue,&nx,1)!=pdTRUE ) {
		WARN("enqueue failed");
		return ENOENT;
	}
	return 0;
}
int RtosQueue::dequeue(Xdr& xdr) {
	Xdr* px;
	if ( xQueueReceive(_queue,&px,1)!=pdTRUE) {
		WARN("dequeue failed");
		return ENOENT;
	}
	xdr = *px; // copy data
//	INFO(" size : %d/%d vs %d/%d ",px->size(),px->capacity(),xdr.size(),xdr.capacity());
	delete px;
	return 0;
}

int RtosQueue::wait(uint32_t time) {
	Xdr* px;
	if ( xQueuePeek(_queue,&px,time/portTICK_PERIOD_MS)!= pdTRUE) {
		return ETIMEDOUT;
	}
	return 0;
}


bool RtosQueue::hasData(uint32_t count) {
	return uxQueueMessagesWaiting(_queue) >= count;
}
