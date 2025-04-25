//Written by Yosef Alqufidi
//Date 4/24/25
//updated from project 4

#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <ctime>
#include <cstring>

using namespace std;

#define MAX_RESOURCES 5
#define REQUEST 0
#define RELEASE 1
#define RELEASE_ALL 2
#define BOUND_NS 1000000000

//Shared memory clock structure
struct ClockDigi{
	int sysClockS;
	int sysClockNano;
};

struct Request{
	long mtype;
	pid_t pid;
	int type;
	int resID;
};

struct Reply{
	long mtype;
	int granted;
};


//logic for shared memory
int main(){
//shared memory key
key_t shmKey= 6321;

//access to shared memory
int shmid = shmget(shmKey, sizeof(ClockDigi), 0666);
ClockDigi* clockVal = (ClockDigi*)shmat(shmid, NULL, 0);
int msgid = msgget(key, 0666);

pid_t pid = getpid();
srand(pid);
int alloc[MAX_RESOURCES] = {0};

long startTime = (long)clockVal->sysClockS*1000000000LL + clockVal->sysClockNano;
long lastAction = startTime;

while(true){
	long now = (long)clockVal->sysClockS*1000000000LL + clockVal->sysClockNano;
	long delta = now - lastAction;
	long randBound = rand() % BOUND_NS;
	if(delta >= randBound){
		lastAction = now;
		if(now - startTime >= 1000000000LL && (rand()%100)<10){
			for(int r=0;r<MAX_RESOURCES;r++){
				if(alloc[r]>0){ Request req{1,pid,RELEASE, r}; msgsnd(msgid,&req,sizeof(req)-sizeof(long),0); }
                }
			Request req{1,pid,RELEASE_ALL,0}; msgsnd(msgid,&req,sizeof(req)-sizeof(long),0);
			break;
		}

		if((rand()%100)<70){

