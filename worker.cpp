//Written by Yosef Alqufidi
//Date 4/15/25
//updated from project 3

#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <ctime>
#include <string>

using namespace std;

//message struct
struct Message{
	long mtype;
	int data;
};

//logic for shared memory
int main(int argc, char** argv){
	if(argc !=3){
		cout<<"Usage:"<< argv[0] << " <term_BS> <term_BN>" <<endl;
		return EXIT_FAILURE;
	}


//termination bounds
int termBS = atoi(argv[1]);
int termBN = atoi(argv[2]);

(void)termBS;
(void)termBN;

//message queue
key_t msgKey = 6321;
int msgid = msgget(msgKey, 0666);
if(msgid < 0){
	perror("msgget");
	return EXIT_FAILURE;
}

//RNG 
srand(time(NULL) ^ getpid());

//probabilities
//termination 10%
const double TERM_PROB = 0.1; 
//I/O 20%
const double IO_PROB = 0.2;

//loop for scheduling messages
while(true){
	Message schedMsg;
	if(msgrcv(msgid, &schedMsg, sizeof(schedMsg.data), getpid(), 0) == -1){
		perror("msgrcv");
		break;
	}

//time slice
int quantum = schedMsg.data;
cout<< "WORKER PID: " <<getpid()<< " recived quantum: " << quantum << "ns" <<endl;

int usage = (rand() % (quantum - 1)) + 1;
Message response;
response.mtype = getpid();
double r = (double)rand() / RAND_MAX;
if(r < TERM_PROB){
	response.data = -usage;
	cout<< "WORKER PID: " << getpid()<< " terminating after usage " << usage <<"ns" <<endl;
	if(msgsnd(msgid, &response, sizeof(response.data), 0) == -1){
		perror("msgsnd");
	}
	break;
}
	else if(r < TERM_PROB + IO_PROB){
		response.data = usage;
		cout<< "WORKER PID: " <<getpid()<< " preempted by I/O after " << usage << "ns" <<endl;
	}else{
	response.data = quantum;
		cout<< "WORKER PID: " <<getpid()<< " used full quantum of " << quantum << "ns" <<endl;
	}

//respond to oss
if(msgsnd(msgid, &response, sizeof(response.data), 0) == -1){
	perror("msgsnd");
	}
}
return EXIT_SUCCESS;
}

