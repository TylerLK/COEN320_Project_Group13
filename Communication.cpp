#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <atomic>
#include <thread>
#include <semaphore.h>
#include <cstring>
#include <sstream>

using namespace std;

const char*shared_comms = "/shm_communication";
const char*sem_comms = "/comm_semaphore";

int shm_fd_term;
void* shm_ptr_term;
sem_t* sem_term;


const char* shm_termination = "/shm_term";
const char* sem_termination = "/term_semaphore";
const int size3 = 64;



const int max_planes = 10;

struct SharedAircraft {
    int aircraftID;
    double positionX, positionY, positionZ;
    double speedX, speedY, speedZ;
    int startTime;
};


const int size = sizeof(SharedAircraft) *max_planes;
const int size2 =256;

sem_t* sem_comm;
int shm_fd_comm;
void* shm_ptr_comm_2;
SharedAircraft* shm_ptr_comm;

void startCommSharedMemory() { //open shared memory to radar
    shm_fd_comm = shm_open(shared_comms, O_CREAT | O_RDWR, 0777);
    if (shm_fd_comm == -1) {
        perror("shared memory failed");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd_comm, size2 + size) == -1) {
        perror("error with size of shared memory");
        exit(EXIT_FAILURE);
    }

    shm_ptr_comm_2 = mmap(0, size2 + size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_comm, 0);
    if (shm_ptr_comm_2 == MAP_FAILED) {
        perror("Error with shared memory mapping");
        exit(EXIT_FAILURE);
    }

    shm_ptr_comm = (SharedAircraft*)((char*)shm_ptr_comm_2 + size2);
    sem_comm = sem_open(sem_comms, O_CREAT, 0777, 1);
    if (sem_comm == SEM_FAILED) {
        perror("Failed to open semaphore");
        exit(EXIT_FAILURE);
    }

    cout << "Communication shared memory and semaphore successful" << endl;
}

void CommunicationCommand() { //send message from computer to radar for speed change
    char commandBuffer[size2];

    sem_wait(sem_comm);
    memcpy(commandBuffer, shm_ptr_comm_2, size2);
    sem_post(sem_comm);

    string command(commandBuffer);
    if (command.empty()) return;

    istringstream iss(command);



   int newSpeedX, newSpeedY, newSpeedZ, aircraftID;
    iss >> aircraftID>> newSpeedX >> newSpeedY >> newSpeedZ;

        cout << "Speed Change request for aircraft ID recieved: " << aircraftID << endl;



        sem_wait(sem_comm);

        for (int i = 0; i < max_planes; ++i) { //send selected aircraft
            if (shm_ptr_comm[i].aircraftID == aircraftID) {
            	shm_ptr_comm[i].aircraftID = aircraftID;
                shm_ptr_comm[i].speedX = newSpeedX;
                shm_ptr_comm[i].speedY = newSpeedY;
                shm_ptr_comm[i].speedZ = newSpeedZ;
                cout << "Speed updated: (" << newSpeedX << ", " << newSpeedY << ", " << newSpeedZ << ")" << endl;
                break;
            }
        }


        memset(shm_ptr_comm_2, 0, size2);
        sem_post(sem_comm);

}

bool checkTermination() {
    int terminated = 0;

    sem_wait(sem_term);
    memcpy(&terminated, shm_ptr_term, sizeof(int));
    sem_post(sem_term);

    return terminated == 1;
}

void monitorTermination() { //closes all shared memory upon request and sends acknowledgment
    while (true) {
        if (checkTermination()) {
            cout << "Termination signal received. Terminating communications subsystem." << endl;
            sem_wait(sem_term);
                strncpy((char*)shm_ptr_term, "Communications", size3 - 1);
                ((char*)shm_ptr_term)[size3 - 1] = '\0';
                sem_post(sem_term);

            munmap(shm_ptr_term, size3);
            close(shm_fd_term);
            sem_close(sem_term);

            munmap(shm_ptr_comm_2, size2 + size);
                       close(shm_fd_comm);
                       sem_close(sem_comm);


                       shm_unlink(shared_comms);
                       sem_unlink(sem_comms);


            exit(0);
        }

        sleep(1);
    }
}
void startTerminationMonitor() { //opens termination shared memory
    shm_fd_term = shm_open(shm_termination, O_CREAT | O_RDWR, 0777);
    if (shm_fd_term == -1) {
        perror("error opening");
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
        perror("error opening");
        exit(1);
    }
}




int main() {
    startCommSharedMemory();
    startTerminationMonitor();

    thread monitor(monitorTermination);

    while (true) {
        CommunicationCommand();
        sleep(1);
    }
    monitor.join();

    return 0;
}
