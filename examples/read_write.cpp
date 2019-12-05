#include <iostream>
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>  //for sleep function
#include <signal.h>
#include <string.h>  //for memset
#include "socketcan_cpp/socketcan_cpp.h"

using namespace std;

scpp::SocketCan socket_can;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


bool forever = true;

void* can_read_thread(void* arg)
{
	scpp::CanFrame fr;
	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	
	printf("init can_read_thread \n");
	
	while(1)
	{
		memset(&fr, 0, sizeof(scpp::CanFrame));
		if (socket_can.read(fr) == scpp::STATUS_OK)
		{
			pthread_mutex_lock(&mutex);
			printf("len %d byte, id: %d, data: %02x %02x %02x %02x %02x %02x %02x %02x  \n", fr.len, fr.id, 
                    fr.data[0], fr.data[1], fr.data[2], fr.data[3],
                    fr.data[4], fr.data[5], fr.data[6], fr.data[7]);
			pthread_mutex_unlock(&mutex);
			pthread_testcancel();

		}
		
		
	}
	
}


void sighandler(int sig)
{
	printf("Signal %d caught...\n", sig);
	forever = false;
}

int main()
{
	pthread_t thread_id;
	pthread_t read_thread;
	int status;

	int i = 1;

	//scpp::SocketCan socket_can;
    if (socket_can.open("can0") == scpp::STATUS_OK)
    {
    	printf("Open CAN Interface - can0 Success!!\n");
    }
    else
    {
    	printf("Cannot open can socket!");
    	return -1;
    }

	if((thread_id = pthread_create(&read_thread, NULL, can_read_thread, NULL)))
	{
		printf("can_read_thread[%zu] Create Failed!\n", thread_id);
	}
	else
		printf("can_read_thread[%zu] Create Success!\n", thread_id);

	sleep(1);

    signal(SIGABRT, &sighandler);
	signal(SIGTERM, &sighandler);
	signal(SIGINT, &sighandler);

	scpp::CanFrame cf_to_write;

	cf_to_write.id = 123;
    cf_to_write.len = 8;

    int j = 0;


	while(forever)
	{

		for (int i = 0; i < 8; ++i)
			cf_to_write.data[i] = j & (254);

		auto write_sc_status = socket_can.write(cf_to_write);

        if (write_sc_status != scpp::STATUS_OK)
            printf("something went wrong on socket write, error code : %d \n", int32_t(write_sc_status));
        else
            printf("Message was written to the socket \n");
            printf("len %d byte, id: %d, data: %02x %02x %02x %02x %02x %02x %02x %02x  \n", cf_to_write.len, cf_to_write.id, 
                    cf_to_write.data[0], cf_to_write.data[1], cf_to_write.data[2], cf_to_write.data[3],
                    cf_to_write.data[4], cf_to_write.data[5], cf_to_write.data[6], cf_to_write.data[7]);
		
		j++;
		usleep(100000);
		
	}

	printf("can_read_thread cancel\n");

	if(pthread_cancel(read_thread)) printf("read_thread_cancel failed\n");
	usleep(100);


	status = pthread_mutex_destroy(&mutex);
	printf("status %d\n", status);
	printf("End!!!\n");

	return 0;


}

