#ifndef CONTROLLER_H
#define CONTROLLER_H
#include <Hardware.h>
#include <Log.h>
#include <Akka.h>
#include <Bridge.h>
#include <Led.h>
#include <Pot.h>
#define MEDIAN_SAMPLES 7



class Controller : public Actor {
		ActorRef& _bridge;
		Label _measureTimer;
		Led _led_right;
		Led _led_left;
		Pot _pot_left;
		Pot _pot_right;
		uint32_t _potLeft;
		uint32_t _potRight;
		DigitalIn& _leftSwitch;
		DigitalIn& _rightSwitch;
	public:
		static MsgClass LedCommand;
		static MsgClass LedLeft;
		static MsgClass LedRight;

		Controller(ActorRef& publisher);
		virtual ~Controller();
		int init();
		void handleRxd();
		void preStart();
		Receive& createReceive();
		void start();
		void run();
};


/*
* Algorithm from N. Wirth’s book, implementation by N. Devillard.
* This code in public domain.
*/
/*---------------------------------------------------------------------------
Function : kth_smallest()
In : array of elements, # of elements in the array, rank k
Out : one element
Job : find the kth smallest element in the array
Notice : use the median() macro defined below to get the median.
Reference:
Author: Wirth, Niklaus
Title: Algorithms + data structures = programs
Publisher: Englewood Cliffs: Prentice-Hall, 1976
Physical description: 366 p.
Series: Prentice-Hall Series in Automatic Computation
---------------------------------------------------------------------------*/



#define ELEM_SWAP(a,b) { register T t=(a);(a)=(b);(b)=t; }
template <typename T >T kth_smallest(T a[], int n, int k) {
	register int i,j,l,m ;
	register T x ;
	l=0 ;
	m=n-1 ;
	while (l<m) {
		x=a[k] ;
		i=l ;
		j=m ;
		do {
			while (a[i]<x) i++ ;
			while (x<a[j]) j-- ;
			if (i<=j) {
				ELEM_SWAP(a[i],a[j]) ;
				i++ ;
				j-- ;
			}
		} while (i<=j) ;
		if (j<k) l=i ;
		if (k<i) m=j ;
	}
	return a[k] ;
}
#define median(a,n) kth_smallest(a,n,(((n)&1)?((n)/2):(((n)/2)-1)))


#endif // CONTROLLER_H
