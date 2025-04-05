#include <iostream>
#include <csignal>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <cstring>
#include <mutex>
#include <atomic>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <thread>
//#include "Aircraft.h"

using namespace std;



//shared memory and sempahore declaration for radar-computer and communications-radar communication
#define shared_name "/radar_shm"
#define sem_name "/radar_semaphore"
#define shared_comms_name "/shm_communication"
#define sem_comms_name "/comm_semaphore"


int shm_fd_term;
void* shm_ptr_term;
sem_t* sem_term;

const char* shm_termination = "/shm_term";
const char* sem_termination = "/term_semaphore";
const int size3 = 64;

int max_planes=20; //maximum amount of planes allowed

const string filename = "Input_Medium.txt"; //file to be read from either with low, medium, or high traffic

time_t programStartTime; //start time of program

struct SharedAircraft { //aircraft object layout
    int aircraftID;
    double positionX, positionY, positionZ;
    double speedX, speedY, speedZ;
    int startTime;
};

vector<SharedAircraft> aircrafting;

int shm_fd, shm_fdd;

SharedAircraft* sharedAircraftList; //aircraft list

mutex air_mutex, comms_mutex;
sem_t* sem_plane;
sem_t* sem_comms;

void printData() { //function which prints updates and prints aircraft positions
    sem_wait(sem_plane); //semaphore and mutex below for critical section of updating aircrafts
    lock_guard<mutex> lock(air_mutex);
    time_t currentTime = time(nullptr);

    int elapsedProgramTime=currentTime-programStartTime; //elapsed time to determine when to put aircrafts into the system

    cout << "[Time: " << elapsedProgramTime << "s] Updated Positions:" << endl;

    for (int i = 0; i < max_planes; i++) {
        SharedAircraft& aircraft = sharedAircraftList[i];

        if (aircraft.startTime == -1)
            continue;

        if (elapsedProgramTime < aircraft.startTime) {
            cout << "ID: " << aircraft.aircraftID << " | Waiting to start at " << aircraft.startTime << "s" << endl;
            continue;
        }

        double elapsedTime = elapsedProgramTime - aircraft.startTime;

if(aircraft.positionX<100000||aircraft.positionY<100000||aircraft.positionZ<40000){ //only update aircrafts if they are within bounds set by the project
        aircraft.positionX += aircraft.speedX * elapsedTime;
        aircraft.positionY += aircraft.speedY * elapsedTime;
        aircraft.positionZ += aircraft.speedZ * elapsedTime;
}
else {
	sharedAircraftList[i]={};
}

        cout << "ID: " << aircraft.aircraftID
             << " | X: " << aircraft.positionX
             << " | Y: " << aircraft.positionY
             << " | Z: " << aircraft.positionZ << endl;
    }
    cout << endl;
    sem_post(sem_plane);
}

void loadAircraftFromFile() { //load aircrafts from the file chosen into the sharedaircraft list
    ifstream file(filename);
    if (!file) {
        cerr << "Error opening file: " << filename << endl;
        exit(EXIT_FAILURE);
    }


    int i = 0;
    while (file && i < max_planes) {
           SharedAircraft aircraft;
           if (file >> aircraft.aircraftID >> aircraft.positionX >> aircraft.positionY >> aircraft.positionZ
                     >> aircraft.speedX >> aircraft.speedY >> aircraft.speedZ >> aircraft.startTime) {
               sharedAircraftList[i] = aircraft;
               i++;
           } else {

               if (file.eof()) {//fixed issue where duplicate last entry appeared
                   break;
               } else {
                   cerr << "Error reading data!" << endl;
                   file.ignore(numeric_limits<streamsize>::max(), '\n');
               }
           }
       }
    file.close();
}


void timerHandler(union sigval sv) { //function which calls the printData functions
    printData();
}


void initializeSharedMemory() { //initializes shared memory between radar-computer
    shm_fd = shm_open(shared_name, O_CREAT | O_RDWR, 0777);
    if (shm_fd == -1) {
        cerr << "Error creating shared memory" << endl;
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, max_planes * sizeof(SharedAircraft)) == -1) {
        cerr << "Error setting shared memory size" << endl;
        exit(EXIT_FAILURE);
    }

    sharedAircraftList = (SharedAircraft*)mmap(0, max_planes * sizeof(SharedAircraft),PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (sharedAircraftList == MAP_FAILED) {
        cerr << "Error mapping shared memory" << endl;
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < max_planes; i++) {
        sharedAircraftList[i].startTime = -1;
    }
}


void startTimer() { //timer which calls the function to update aircraft data every second
    timer_t timer_id;
    struct sigevent sev;
    struct itimerspec its;

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = timerHandler; //function to call
    sev.sigev_value.sival_ptr = nullptr;
    sev.sigev_notify_attributes = nullptr;

    if (timer_create(CLOCK_REALTIME, &sev, &timer_id) == -1) {
        cerr << "Error creating timer: " << strerror(errno) << endl;
        exit(EXIT_FAILURE);
    }

    its.it_value.tv_sec = 1;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 1;
    its.it_interval.tv_nsec = 0;

    if (timer_settime(timer_id, 0, &its, nullptr) == -1) {
        cerr << "Error setting timer: " << strerror(errno) << endl;
        exit(EXIT_FAILURE);
    }

    while (true) {
        sleep(1);
    }
}
void changeSpeed(int passedID, int speedx, int speedy, int speedz){ //function which updates selected aircraft's speed

for (int i = 0; i < max_planes; i++) {
	SharedAircraft& aircraft = sharedAircraftList[i];

	if(aircraft.aircraftID==passedID){
	aircraft.speedX=speedx;
	aircraft.speedY=speedy;
	aircraft.speedZ=speedz;
}
	else{

	}

}
}

void changeParameters() { //opens shared memory with communications
    sem_comms = sem_open(sem_comms_name, O_CREAT, 0777, 1);
    if (sem_comms == SEM_FAILED) {
        perror("Failed to open semaphore");
        exit(EXIT_FAILURE);
    }

    int shm_fd_comms = shm_open(shared_comms_name, O_CREAT | O_RDWR, 0777);
    if (shm_fd_comms == -1) {
        perror("failed to open shared memory");
        exit(EXIT_FAILURE);
    }

    SharedAircraft* sharedComms = (SharedAircraft*)mmap(0, sizeof(SharedAircraft) * max_planes, PROT_READ, MAP_SHARED, shm_fd_comms, 0);
    if (sharedComms == MAP_FAILED) {
        perror("Failed to map shared memory");
        sem_close(sem_comms);
        close(shm_fd_comms);
        exit(EXIT_FAILURE);
    }

    cout << "Radar: Communications shared memory received" << endl;

    while (true) {
        sem_wait(sem_comms);
        lock_guard<mutex> lock(comms_mutex);

        for (int i = 0; i < max_planes; i++) { //takes request from communications to change speed and calls changespeed function
            SharedAircraft& aircraftComms = sharedComms[i];
            if (aircraftComms.aircraftID == 0) continue;

            changeSpeed(
                aircraftComms.aircraftID,
                aircraftComms.speedX,
                aircraftComms.speedY,
                aircraftComms.speedZ
            );
        }

        sem_post(sem_comms);
        sleep(1); // check for updates every second
    }
}


void startTerminationMonitor() { // opens termination shared memory
    shm_fd_term = shm_open(shm_termination, O_CREAT | O_RDWR, 0777);
    if (shm_fd_term == -1) {
        perror("error with opening");
        exit(1);
    }

    if (ftruncate(shm_fd_term, size3) == -1) {
        perror("error with ftruncate");
        exit(1);
    }

    shm_ptr_term = mmap(0, size3, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_term, 0);
    if (shm_ptr_term == MAP_FAILED) {
        perror("error mapping");
        exit(1);
    }

    sem_term = sem_open(sem_termination, O_CREAT, 0777, 1);
    if (sem_term == SEM_FAILED) {
        perror("error with sempahore");
        exit(1);
    }
}

bool checkTermination() {
    int terminated = 0;

    sem_wait(sem_term);
    memcpy(&terminated, shm_ptr_term, sizeof(int));
    sem_post(sem_term);

    return terminated == 1;
}

void monitorTermination() { //check if termination signal recieved and send termination completion message
    while (true) {
        if (checkTermination()) {
            cout << "Termination signal received. Terminating radar subsystem." << endl;


            sem_wait(sem_term);
                strncpy((char*)shm_ptr_term, "Radar", size3 - 1);
                ((char*)shm_ptr_term)[size3 - 1] = '\0';
                sem_post(sem_term);


            munmap(shm_ptr_term, size3); //close shared memory and semaphores
            close(shm_fd_term);
            sem_close(sem_term);

            munmap(sharedAircraftList, max_planes * sizeof(SharedAircraft));
                       close(shm_fd);
                       sem_close(sem_plane);
                       sem_close(sem_comms);


                       shm_unlink(shared_name);
                       shm_unlink(shm_termination);

            exit(0);
        }

        sleep(1);
    }
}





int main() {



    initializeSharedMemory();
    startTerminationMonitor();
    loadAircraftFromFile();
    programStartTime = time(nullptr);
    thread t1(startTimer); //threads to make updating aircraft psoitions and speed change request run simultaneously
    thread t2(changeParameters);
    thread t3(monitorTermination);


    t1.join();
    t2.join();
t3.join();
    shm_unlink(shared_name);
    return 0;
}
