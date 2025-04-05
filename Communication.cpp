#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <cstring>
//#include "Aircraft.h"

using namespace std;

const char* shared_comms = "/shm_communication";
const char* sem_comms = "/comm_semaphore";
const int max_planes=10;


struct SharedAircraft { //aircraft object layout
    int aircraftID;
    double positionX, positionY, positionZ;
    double speedX, speedY, speedZ;
    int startTime;
};
const int size = sizeof(SharedAircraft) * max_planes;

sem_t* sem_comm;
int shm_fd_comm;
SharedAircraft* shm_ptr_comm;



void initCommunicationSharedMemory() {

    shm_fd_comm = shm_open(shared_comms, O_CREAT | O_RDWR, 0777);
    if (shm_fd_comm == -1) {
        perror("shared memory failed");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd_comm, size) == -1) {
        perror("error with size of shared memory");
        exit(EXIT_FAILURE);
    }


    shm_ptr_comm = (SharedAircraft*)mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd_comm, 0);
    if (shm_ptr_comm == MAP_FAILED) {
        perror("Error with shared memory mapping");
        exit(EXIT_FAILURE);
    }


    sem_comm = sem_open(sem_comms, O_CREAT, 0777, 1);
    if (sem_comm == SEM_FAILED) {
        perror("Failed to open semaphore");
        exit(EXIT_FAILURE);
    }

    cout << "Communication shared memory and semaphore successful" << endl;
}
void testWriteAircraftData() {
    sem_wait(sem_comm);

    for (int i = 0; i < max_planes; ++i) {
    	shm_ptr_comm[i].aircraftID = 30000003; //test value to tell radar which id to change speed

        shm_ptr_comm[i].speedX = 0;
        shm_ptr_comm[i].speedY = 0;
        shm_ptr_comm[i].speedZ = 0;
    }

    sem_post(sem_comm);
    cout << "Test data written to shared memory" << endl;

sleep(1);
}


int main() {
    initCommunicationSharedMemory();

    while(true){
    testWriteAircraftData();
    }
    return 0;
}
