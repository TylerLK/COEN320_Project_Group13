#include <iostream>
#include <csignal>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <unistd.h>
#include <cstring>
#include <mutex>
#include <fcntl.h>
#include <sys/mman.h>

using namespace std;

#define shared_name "/aircraft_shm"
int max_planes=10;

time_t programStartTime;

struct SharedAircraft {
    char aircraftID[10];
    double positionX, positionY, positionZ;
    double speedX, speedY, speedZ;
    int startTime;
};

int shm_fd;
SharedAircraft* sharedAircraftList;

mutex air_mutex;

void printData() {
    lock_guard<mutex> lock(air_mutex);
    time_t currentTime = time(nullptr);

    int elapsedProgramTime=currentTime-programStartTime;

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

        aircraft.positionX += aircraft.speedX * elapsedTime;
        aircraft.positionY += aircraft.speedY * elapsedTime;
        aircraft.positionZ += aircraft.speedZ * elapsedTime;

        cout << "ID: " << aircraft.aircraftID
             << " | X: " << aircraft.positionX
             << " | Y: " << aircraft.positionY
             << " | Z: " << aircraft.positionZ << endl;
    }
    cout << endl;
}

void getUserInputAircraft() {
    int numAircrafts;

    while (true) {
        cout << "Enter number of aircrafts (Max " << max_planes << "): ";
        cin >> numAircrafts;

        if (numAircrafts > 0 && numAircrafts <= max_planes)
            break;
        else
            cout << "Invalid input. Please enter between 1 and " << max_planes << endl;
    }

    for (int i = 0; i < numAircrafts; i++) {
        SharedAircraft aircraft;

        cout << "\nEnter Aircraft " << (i + 1) << " ID: ";
        string id;
        cin >> id;
        strncpy(aircraft.aircraftID, id.c_str(), sizeof(aircraft.aircraftID) - 1);
        aircraft.aircraftID[sizeof(aircraft.aircraftID) - 1] = '\0';

        cout << "Enter Initial Position (X Y Z): ";
        cin >> aircraft.positionX >> aircraft.positionY >> aircraft.positionZ;

        cout << "Enter Speed (X Y Z): ";
        cin >> aircraft.speedX >> aircraft.speedY >> aircraft.speedZ;

        cout << "Enter Start Time (seconds): ";
        cin >> aircraft.startTime;

        sharedAircraftList[i] = aircraft;
    }
}


void timerHandler(union sigval sv) {
    printData();
}


void initializeSharedMemory() {
    shm_fd = shm_open(shared_name, O_CREAT | O_RDWR, 1234);
    if (shm_fd == -1) {
        cerr << "Error creating shared memory" << endl;
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, max_planes * sizeof(SharedAircraft)) == -1) {
        cerr << "Error setting shared memory size" << endl;
        exit(EXIT_FAILURE);
    }

    sharedAircraftList = (SharedAircraft*)mmap(0, max_planes * sizeof(SharedAircraft),
                                               PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (sharedAircraftList == MAP_FAILED) {
        cerr << "Error mapping shared memory" << endl;
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < max_planes; i++) {
        sharedAircraftList[i].startTime = -1;
    }
}


void startTimer() {
    timer_t timer_id;
    struct sigevent sev;
    struct itimerspec its;

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = timerHandler;
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

int main() {


    initializeSharedMemory();
    getUserInputAircraft();
    programStartTime = time(nullptr);
    startTimer();

    shm_unlink(shared_name);
    return 0;
}
