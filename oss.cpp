//Written by Yosef Alqufidi
//Date 4/29/25
//updated from project 4


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

using namespace std;

//PCB tracking children

#define PROCESS_TABLE 20
#define MAX_RESOURCES 5
#define INSTANCES_PER_RESOURCE 10

enum RequestType { REQUEST = 0, RELEASE = 1, RELEASE_ALL = 2 };

//struct for PCB
struct PCB{
	int occupied;
	pid_t pid;
	int startSeconds;
	int startNano;
	int messagesSent;
	int alloc[MAX_RESOURCES];
};

//struct for clock
struct ClockDigi{
	int sysClockS;
	int sysClockNano;
};

//struct for messages
struct Message{
	long mtype;
	pid_t pid;
	int type;
	int resID;
};

struct Resource{
	int total;
	int available;
	pid_t waitingQueue[PROCESS_TABLE];
	int waitCount;
};

PCB processTable[PROCESS_TABLE];
Resource resources[MAX_RESOURCES];
//clock ID
int shmid;
//shared memory ptr
ClockDigi* clockVal = nullptr;
//message ID
int msgid;

ofstream logFile;

int findIndex(pid_t pid){
	for(int i=0;i<PROCESS_TABLE;i++){ 
		if(processTable[i].occupied && processTable[i].pid == pid) 
			return i;
}
	return -1;
	
}

//print process table to screen and logfile
void printProcessTable(){
	cout<<"Process Table:-------------------------------------------------------------------------------\n";
	cout<<"Entry\tOccupied\tPID\tStartS\tStartN\tMessagesSent\n";
	logFile<<"Process Table::-------------------------------------------------------------------------------\n";
	logFile << "Entry\tOccupied\tPID\tStartS\tStartN\tMessagesSent\n";
    for (int i = 0; i < PROCESS_TABLE; i++) {
	    if(!processTable[i].occupied) continue;
        cout << i << "\t" << processTable[i].occupied << "\t\t"
             << processTable[i].pid << "\t"
             << processTable[i].startSeconds << "\t"
             << processTable[i].startNano << "\t"
             << processTable[i].messagesSent << "\n";
        logFile << i << "\t" << processTable[i].occupied << "\t\t"
                << processTable[i].pid << "\t"
                << processTable[i].startSeconds << "\t"
                << processTable[i].startNano << "\t"
                << processTable[i].messagesSent << "\n";
	for(int r = 0; r < MAX_RESOURCES; r++){
		cout << " " << processTable[i].alloc[r];
		logFile << " " << processTable[i].alloc[r];

    }
    cout.flush();
    logFile.flush();
}
}

void printResourceTable(){
	cout << "Resource Table:------------------------------------------------------\n";
    logFile << "Resource Table:------------------------------------------------------\n";
    for(int r = 0; r < MAX_RESOURCES; ++r) {
        cout << "R" << r << ": available=" << resources[r].available
             << " waiting=" << resources[r].waitCount << "\n";
        logFile << "R" << r << ": available=" << resources[r].available
                << " waiting=" << resources[r].waitCount << "\n";
    }
}

void cleanup(int) {
    //send kill signal to all children based on their PIDs in process table
for(int i = 0; i < PROCESS_TABLE; i++){
	if(processTable[i].occupied) kill(processTable[i].pid, SIGKILL);
	
}
//free up shared memory
if(clockVal) shmdt(clockVal);
shmctl(shmid, IPC_RMID, NULL);
msgctl(msgid, IPC_RMID, NULL);
if(logFile.is_open()) logFile.close();
    exit(1);
}


//adding command line args for parse
int main( int argc, char *argv[]){
	
	int n_case = 0;
	int s_case = 0;
//	int t_case = 0;
	int i_case = 0;
	string logFileName;
	bool n_var = false, s_var = false, i_var = false, f_var = false;
	int opt;

//setting up the parameters for h,n,s,t,i, and f
	while ((opt = getopt(argc, argv, "hn:s:i:f: ")) != -1) {
		switch (opt){
			case 'h':
			cout<< "This is the help menu\n";
			cout<< "-h: shows help\n";
			cout<< "-n: processes\n";
			cout<< "-s: simultaneous\n";
//			cout<< "-t: iterations\n";
			cout<< "-f: logfile\n";
			cout<< "To run try ./oss -n 1 -s 1 -t 1 -i 100 -f logfile.txt\n";

	return EXIT_SUCCESS;

			case 'n':
				n_case = atoi(optarg);
				n_var = true;
				break;

			case 's':
				s_case = atoi(optarg);
				s_var = true;
				break;

//			case 't':
//				t_case = atoi(optarg);
//				t_var = true;
//				break;

			case 'i':
				i_case = atoi(optarg);
				i_var = true;
				break;
			case 'f':
                		logFileName = optarg;
             			f_var = true;
                		break;

			default:
				cout<< "invalid";

			return EXIT_FAILURE;
		}
	}

//only allow all three to be used together and not by itself 	
	if(!n_var || !s_var || !i_var || !f_var){
		cout<<"ERROR: You cannot do one alone please do -n, -s,-i and -f together.\n";
			return EXIT_FAILURE;
	}
//error checking for logfile
	logFile.open(logFileName);
	if(!logFile){
		cout<<"ERROR: could not open" << logFileName << "\n";
		return EXIT_FAILURE;
	}

//shared memory for clock using key to verify

key_t key = 6321;

shmid = shmget(key, sizeof(ClockDigi), IPC_CREAT | 0666);
	clockVal = (ClockDigi*)shmat(shmid, NULL, 0);
	clockVal->sysClocks = 0;
	clockVal->sysClockNano = 0;

for(int i = 0; i < PROCESS_TABLE; ++i) {
        processTable[i].occupied = 0;
        processTable[i].pid = 0;
        processTable[i].startSeconds = 0;
        processTable[i].startNano = 0;
        processTable[i].messagesSent = 0;
        for(int r = 0; r < MAX_RESOURCES; ++r) processTable[i].alloc[r] = 0;
    }
//init resources
    for(int r = 0; r < MAX_RESOURCES; ++r) {
        resources[r].available = INSTANCES_PER_RESOURCE;
        resources[r].waitCount = 0;
    }
