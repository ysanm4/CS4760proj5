//Written by Yosef Alqufidi
//Date 4/15/25
//updated from project 3


#include <iostream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <csignal>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <ctime>
#include <string>
#include <sys/msg.h>
#include <queue>

using namespace std;

//PCB tracking children

#define PROCESS_TABLE 20
#define MAX_PROCESSES 100

//based time quantum
const long long BASE_QUANTUM = 10000000LL;
//struct for PCB
struct PCB{
	int occupied;
	pid_t pid;
	int startSeconds;
	int startNano;
	int serviceTimeSeconds;
	int serviceTimeNano;
	int eventWaitSec;
	int eventWaitNano;
	int blocked;
};

//struct for clock
struct ClockDigi{
	int sysClockS;
	int sysClockNano;
};

//struct for messages
struct Message{
	long mtype;
	int data;
};

PCB processTable[PROCESS_TABLE];
//clock ID
int shmid;
//shared memory ptr
ClockDigi* clockVal = nullptr;
//message ID
int msgid;

ofstream logFile;

//multi-level feedback queue from highest to lowest priority level
queue<int> readyQueue1;
queue<int> readyQueue2;
queue<int> readyQueue3;
//blocked queue
queue<int> blockedQueue;


//print process table to screen and logfile
void printProcessTable(){
	cout<<"Process Table:-------------------------------------------------------------------------------\n";
	cout<<"Entry\tOccupied\tPID\tStartS\tStartN\tService\tBlocked\n";
	logFile<<"Process Table::-------------------------------------------------------------------------------\n";
	logFile << "Entry\tOccupied\tPID\tStartS\tStartN\tService\tBlocked\n";
    for (int i = 0; i < PROCESS_TABLE; i++) {
        cout << i << "\t" << processTable[i].occupied << "\t\t"
             << processTable[i].pid << "\t" 
             << processTable[i].startSeconds << "\t" 
             << processTable[i].startNano << "\t" 
             << processTable[i].serviceTimeSeconds << ":" 
             << processTable[i].serviceTimeNano << "\t\t" 
             << processTable[i].blocked << "\n";
        logFile << i << "\t" << processTable[i].occupied << "\t\t"
                << processTable[i].pid << "\t" 
                << processTable[i].startSeconds << "\t" 
                << processTable[i].startNano << "\t" 
                << processTable[i].serviceTimeSeconds << ":" 
                << processTable[i].serviceTimeNano << "\t\t" 
                << processTable[i].blocked << "\n";
    }

//multilevel feedback queue    
    cout<<" Ready queue 1: ";
    logFile<<" Ready queue 1: ";
    {
    queue<int> tmp = readyQueue1;
    while(!tmp.empty()){
	    cout<<tmp.front()<<" ";
	    logFile << tmp.front() << " ";
        tmp.pop();
    }
}

    cout<<"\nReady queue 2:";
    logFile<<"\nReady queue 2:";
{
    queue<int> tmp = readyQueue2;
    while(!tmp.empty()){
            cout<<tmp.front()<<" ";
            logFile << tmp.front() << " ";
        tmp.pop();
    }
}

    cout<<"\nReady queue 3:";
    logFile<<"\nReady queue 3:";
{
    queue<int> tmp = readyQueue3;
    while(!tmp.empty()){
            cout<<tmp.front()<<" ";
            logFile << tmp.front() << " ";
        tmp.pop();
    }
}

   
    cout<<"\nBlocked queue:";
    logFile<<"\nBlocked queue:";
{
    queue<int> tmp = blockedQueue;
    while(!tmp.empty()){
            cout<<tmp.front()<<" ";
            logFile << tmp.front() << " ";
        tmp.pop();
    }

}
}

void signal_handler(int sig) {
    //send kill signal to all children based on their PIDs in process table
for(int i = 0; i < PROCESS_TABLE; i++){
	if(processTable[i].occupied){
		kill(processTable[i].pid, SIGKILL);
	}
}
//free up shared memory
if(clockVal != nullptr){
	shmdt(clockVal);
}
shmctl(shmid, IPC_RMID, NULL);
msgctl(msgid, IPC_RMID, NULL);
if(logFile.is_open()){
	logFile.close();
}
    exit(1);
}


//adding command line args for parse
int main( int argc, char *argv[]){
	
	int n_case = 7;
	int s_case = 2;
	int t_case = 4;
	int i_case = 100;
	string logFileName = "ossLog.txt";
//	bool n_var = false, s_var = false, t_var = false, i_var = false, f_var = false;
	int opt;

//setting up the parameters for h,n,s,t,i, and f
	while ((opt = getopt(argc, argv, "hn:s:t:i:f: ")) != -1) {
		switch (opt){
			case 'h':
			cout<< "This is the help menu\n";
			cout<< "-h: shows help\n";
			cout<< "-n: processes\n";
			cout<< "-s: simultaneous\n";
			cout<< "-t: iterations\n";
			cout<< "-f: logfile\n";
			cout<< "To run try ./oss -n 1 -s 1 -t 1 -i 100 -f logfile.txt\n";
			cout<< "Or just simply run ./oss and it will auto populate\n";

	return EXIT_SUCCESS;

			case 'n':
				n_case = atoi(optarg);
//				n_var = true;
				break;

			case 's':
				s_case = atoi(optarg);
				(void)s_case;
//				s_var = true;
				break;

			case 't':
				t_case = atoi(optarg);
//				t_var = true;
				break;

			case 'i':
				i_case = atoi(optarg);
//				i_var = true;
				break;
			case 'f':
                		logFileName = optarg;
//             			f_var = true;
                		break;

			default:
				cout<< "invalid";

			return EXIT_FAILURE;
		}
	}

//only allow all three to be used together and not by itself 	
//	if(!n_var || !s_var || !t_var || !i_var || !f_var){
//		cout<<"ERROR: You cannot do one alone please do -n, -s, -t,-i and -f together.\n";
//			return EXIT_FAILURE;
//	}
//error checking for logfile
	logFile.open(logFileName.c_str());
	if(!logFile){
		cout<<"ERROR: could not open" << logFileName << "\n";
		return EXIT_FAILURE;
	}

//shared memory for clock using key to verify

key_t key = 6321;

shmid = shmget(key, sizeof(ClockDigi), IPC_CREAT | 0666);
	if(shmid < 0){
		perror("shmget");
		return EXIT_FAILURE;
}

clockVal = (ClockDigi*) shmat(shmid, nullptr, 0);
	if(clockVal ==(void*) -1){
		perror("shmat");
		return EXIT_FAILURE;
	}
//lets initialize the clock

clockVal->sysClockS = 0;
clockVal->sysClockNano = 0;

//lets now initialize the process table

for(int i = 0; i < PROCESS_TABLE; i++){
	processTable[i].occupied = 0;
	processTable[i].pid = 0;
	processTable[i].startSeconds = 0;
	processTable[i].startNano = 0;
	processTable[i].serviceTimeSeconds = 0;
	processTable[i].serviceTimeNano = 0;
        processTable[i].eventWaitSec = 0;
        processTable[i].eventWaitNano = 0;
        processTable[i].blocked = 0;
}

//message queues

key_t msgKey = 6321;

msgid = msgget(msgKey, IPC_CREAT | 0666);
	if(msgid < 0){
		perror("msgget");
		return EXIT_FAILURE;
	}


//signal and alarm to terminate after 3 real life seconds

signal(SIGALRM, signal_handler);
signal(SIGINT, signal_handler);
alarm(3);
srand(time(NULL));

//counters 
//launched child
int laun = 0;
//running child
int runn = 0;
//tracking process launch time 
long long lastPrintTime = 0;
long long nextLaunchTime = (long long)(rand() % (i_case * 1000000));
long long currentTime = 0;
long long totalServiceTime = 0;
long long totalIdleTime = 0;
long long lastLaunchedTime = 0;
(void)lastLaunchedTime;

//clock simulation -----------------------------------------------------------------------------------------------------------------
//launch intervals


//increment the clock by 10000ms
while(laun < n_case || runn > 0){
	long long overhead = 10000;
	long long totalNano = (long long)clockVal->sysClockS * 1000000000LL + clockVal->sysClockNano + overhead;
	clockVal->sysClockS = totalNano / 1000000000LL;
	clockVal->sysClockNano = totalNano % 1000000000LL;
	currentTime = (long long)clockVal->sysClockS * 1000000000LL + clockVal->sysClockNano;

//blocked queue wait time
	int blockedCount = blockedQueue.size();
	for(int i = 0; i < blockedCount; i++){
		int idx = blockedQueue.front();
		blockedQueue.pop();
		long long eventTime = (long long)processTable[idx].eventWaitSec * 1000000000LL + processTable[idx].eventWaitNano;

	if(currentTime >= eventTime){
		processTable[idx].blocked = 0;
		readyQueue1.push(idx);
		cout<<"OSS: Unblocking process at PCB "<< idx <<"\n";
		logFile << "OSS: Unblocking process at PCB " << idx << "\n";
            } else {
                blockedQueue.push(idx);
            }
        }

//launch new process when PCB free
	if(laun < n_case && currentTime >= nextLaunchTime){
	int freeIndex = -1;
	for(int i = 0; i < PROCESS_TABLE; i++){
		if(!processTable[i].occupied){
			freeIndex = i;
			break;
		}
	}
//term bound	
	if(freeIndex != -1){
		int childOffsetSec = (rand() % t_case) + 1;
		int childOffsetNano = rand() % 1000000000;
//fork		
            pid_t pid = fork();
            if (pid < 0) {
                cout<< "fork failed\n";
                signal_handler(SIGALRM);
            } else if (pid == 0) {
//exec                
                string secStr = to_string(childOffsetSec);
                string nanoStr = to_string(childOffsetNano);
                execlp("./worker", "worker", secStr.c_str(), nanoStr.c_str(), (char*)NULL);
                cout<< "exec failed\n";
                exit(EXIT_FAILURE);
            } else {
                
                    processTable[freeIndex].occupied = 1;
                    processTable[freeIndex].pid = pid;
                    processTable[freeIndex].startSeconds = clockVal->sysClockS;
                    processTable[freeIndex].startNano = clockVal->sysClockNano;
                    processTable[freeIndex].serviceTimeSeconds = 0;
                    processTable[freeIndex].serviceTimeNano = 0;
                    processTable[freeIndex].blocked = 0;
		    readyQueue1.push(freeIndex);
		laun++;
		runn++;
		lastLaunchedTime = currentTime;
		cout<<"OSS: launched new process, PCB "<< freeIndex <<"\n";
		logFile<<"OSS: launched new process, PCB "<< freeIndex <<"\n";
            }
        }
	nextLaunchTime = currentTime + (rand() % (i_case * 1000000));
    }

//selecting process from multilevel feedback queue
int scheduledIndex = -1;
int level = 0;
long long timeQuantum = 0;
if(!readyQueue1.empty()){
	scheduledIndex = readyQueue1.front();
	readyQueue1.pop();
	level = 1;
	timeQuantum = BASE_QUANTUM;
}
else if(!readyQueue2.empty()){
        scheduledIndex = readyQueue2.front();
        readyQueue2.pop();
        level = 2;
        timeQuantum = 2 * BASE_QUANTUM;
}
else if(!readyQueue3.empty()){
        scheduledIndex = readyQueue3.front();
        readyQueue3.pop();
        level = 3;
        timeQuantum = 4 * BASE_QUANTUM;
}else{
	long long idleIncrement = 100000000LL;
	totalIdleTime += idleIncrement;
	totalNano = (long long)clockVal->sysClockS * 1000000000LL + clockVal->sysClockNano + idleIncrement;
	clockVal->sysClockS = totalNano / 1000000000LL;
	clockVal->sysClockNano = totalNano % 1000000000LL;
	continue;
}

//scheduling overhead
overhead = 10000;
totalNano = (long long)clockVal->sysClockS * 1000000000LL + clockVal->sysClockNano + overhead;

clockVal->sysClockS = totalNano / 1000000000LL;
clockVal->sysClockNano = totalNano % 1000000000LL;
currentTime = (long long)clockVal->sysClockS * 1000000000LL + clockVal->sysClockNano;

//send message to process
Message schedMsg;
schedMsg.mtype = processTable[scheduledIndex].pid;
schedMsg.data = timeQuantum;
if(msgsnd(msgid, &schedMsg, sizeof(schedMsg.data), 0) == -1){
	perror("msgsnd");
}else{
	cout<<"OSS: scheduled PCB "<< scheduledIndex
	<<" PID "<< processTable[scheduledIndex].pid << " with quantum " << timeQuantum << " ns at time " << clockVal->sysClockS << ":" << clockVal->sysClockNano << "\n";
	 logFile<<"OSS: scheduled PCB "<< scheduledIndex
        <<" PID "<< processTable[scheduledIndex].pid << " with quantum " << timeQuantum << " ns at time " << clockVal->sysClockS << ":" << clockVal->sysClockNano << "\n";
}

//message response from process
Message response;
if(msgrcv(msgid, &response, sizeof(response.data), processTable[scheduledIndex].pid, 0) == -1){
	perror("msgrcv");
}else{
	int respTime = response.data;
	processTable[scheduledIndex].serviceTimeNano += abs(respTime);
	while(processTable[scheduledIndex].serviceTimeNano >= 1000000000){
		processTable[scheduledIndex].serviceTimeNano -= 1000000000;
		processTable[scheduledIndex].serviceTimeSeconds++;
	}
	totalServiceTime += abs(respTime);

	if(respTime == timeQuantum){
		cout<<"OSS: PCB "<< scheduledIndex <<" used full quantum\n";
		logFile<<"OSS: PCB "<< scheduledIndex <<" used full quantum\n";

//demoteing process, 1 to 2, 2 to 3
if(level == 1){
	readyQueue2.push(scheduledIndex);
}else if(level == 2){
	readyQueue3.push(scheduledIndex);
}else{
	readyQueue3.push(scheduledIndex);
}
}
	else if(respTime > 0 && respTime < timeQuantum){
		cout<<"Oss: PCB "<< scheduledIndex <<" preempted I/O "<< respTime <<"ns\n";
		logFile<<"Oss: PCB "<< scheduledIndex <<" preempted I/O "<< respTime <<"ns\n";
		int waitSec = rand() % 6;
		int waitNano = rand() % 1000;
		long long totalWaitNano = clockVal->sysClockNano + waitNano;
		processTable[scheduledIndex].eventWaitSec = clockVal->sysClockS + waitSec + (totalWaitNano / 1000000000);
		processTable[scheduledIndex].eventWaitNano = totalWaitNano % 1000000000;
                processTable[scheduledIndex].blocked = 1;
                blockedQueue.push(scheduledIndex);
            } else if (respTime < 0) {
                // Process terminated; absolute value is time used.
                cout<<"OSS: PCB "<< scheduledIndex <<" terminated after using "<< -respTime <<"ns\n";
                logFile <<"OSS: PCB "<< scheduledIndex << " terminated after using "<< -respTime <<" ns\n";
                processTable[scheduledIndex].occupied = 0;
                processTable[scheduledIndex].pid = 0;
                processTable[scheduledIndex].startSeconds = 0;
                processTable[scheduledIndex].startNano = 0;
                processTable[scheduledIndex].serviceTimeSeconds = 0;
                processTable[scheduledIndex].serviceTimeNano = 0;
                processTable[scheduledIndex].eventWaitSec = 0;
                processTable[scheduledIndex].eventWaitNano = 0;
                processTable[scheduledIndex].blocked = 0;
                runn--;
            }
        }
//pring every half second
if(currentTime - lastPrintTime >= 500000000LL){
	printProcessTable();
	lastPrintTime = currentTime;
	}
}

	cout<<"Simulation complete. Total processes launched: " << laun
         <<"Total service time: "<< totalServiceTime << "ns. Total idle time:"<< totalIdleTime <<"ns\n";

	logFile << "Simulation complete. Total processes launched: " << laun
            <<"Total service time:"<< totalServiceTime <<"ns. Total idle time:"<< totalIdleTime <<"ns\n";

//cleanup and close
    shmdt(clockVal);
    shmctl(shmid, IPC_RMID, NULL);
    msgctl(msgid, IPC_RMID, NULL);
    logFile.close();

    return EXIT_SUCCESS;
}

