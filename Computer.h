#include <iostream>
#include <tuple>
#include <cmath>
#include <string>
#include <chrono>
#include <vector>
#include <limits>
#include <queue>
#include <fstream>
#include <thread> // Include thread library for multithreading
#include <mutex>  // Include mutex for thread synchronization
#include <atomic> // Include atomic for termination flag
#include <semaphore.h> // Include semaphore for synchronization
#include <sys/mman.h> // For mmap, munmap
#include <fcntl.h>    // For shm_open, O_CREAT, O_RDWR
#include <unistd.h>   // For ftruncate
#include <cstring>    // For memcpy
#include "Aircraft.h"
#include <sstream>

//m1
#define shared_name "/radar_shm"
#define sem_name "/radar_semaphore"

using namespace std;
using namespace std::chrono;

//m2
mutex air_mutex;
sem_t* sem_plane;

// Semaphore names
const char* AIRCRAFT_SEMAPHORE_NAME = "/aircraft_semaphore";
const char* ALERTS_SEMAPHORE_NAME = "/alerts_semaphore";
const char* COMMUNICATION_SEMAPHORE_NAME = "/communication_semaphore";
const char* DATADISPLAY_SEMAPHORE_NAME = "/datadisplay_semaphore";

// Shared memory names
const char* AIRCRAFT_SHARED_MEMORY_NAME = "/AircraftData";
const char* ALERTS_SHARED_MEMORY_NAME = "/AlertsData";
const char* AUGMENTED_INFO_MEMORY_NAME ="/AugmentedData";

int alertsShmFd;        // File descriptor for alerts shared memory
void* alertsShmPtr;     // Pointer to alerts shared memory
size_t alertsShmSize;   // Size of the alerts shared memory

// Shared memory and semaphore names for operator commands
const char* SHARED_MEMORY_LOGS = "/shm_logs";
const char* SEMAPHORE_LOGS = "/logs/semaphore";

const char* SHARED_MEMORY_TERMINATION = "/shm_term";
const char* SEMAPHORE_TERMINATION = "/term_semaphore";

const int SHM_SIZE = 4096; // Shared memory size

// Shared memory and semaphore names for communication
const char* SHARED_MEMORY_COMMUNICATION = "/shm_communication";
const char* SEMAPHORE_COMMUNICATION = "/comm_semaphore";

const int COMM_SHM_SIZE = 4096; // Shared memory size for communication

sem_t* sem_logs;       // Semaphore for operator commands
sem_t* sem_term;       // Semaphore for termination signal
void* shm_ptr_logs;    // Pointer to shared memory for operator commands
void* shm_ptr_term;    // Pointer to shared memory for termination signal
int shm_fd_logs;       // File descriptor for shared memory (logs)
int shm_fd_term;       // File descriptor for shared memory (termination)

sem_t* sem_comm;       // Semaphore for communication
void* shm_ptr_comm;    // Pointer to shared memory for communication
int shm_fd_comm;       // File descriptor for shared memory (communication)


sem_t* sem_display;          // Semaphore for data display synchronization
void* shm_ptr_alerts;        // Pointer to shared memory for alerts
void* shm_ptr_aircrafts;     // Pointer to shared memory for aircraft data
int shm_fd_alerts;           // File descriptor for shared memory (alerts)
int shm_fd_aircrafts;        // File descriptor for shared memory (aircraft data)
mutex alertsMutex;           // Mutex for protecting alerts shared memory
mutex aircraftsMutex;        // Mutex for protecting aircraft data shared memory

sem_t* sem_augmentedInfo;       // Semaphore for augmented information
void* shm_ptr_augmentedInfo;    // Pointer to shared memory for augmented information
int shm_fd_augmentedInfo;       // File descriptor for shared memory (augmented information)


struct SharedAircraft {
    int aircraftID;
    double positionX, positionY, positionZ;
    double speedX, speedY, speedZ;
    int startTime;
};


int shm_fdd;
SharedAircraft* sharedAircraftList;

struct Alert {
    double time;
    string message;

    bool operator<(const Alert& other) const {
        return time > other.time; // Higher priority for more imminent alerts
    }
};

class Computer {
public:
    // Constructor
    Computer() : terminate(false), logFile("history.txt", ios::out | ios::app) {
        if (!logFile.is_open()) {
            cerr << "Error opening log file." << endl;
        } else {
            logFile << "Computer object created." << endl;
        }

        cout << "Computer object created." << endl;

        // Initialize semaphores
        radarSemaphore = sem_open(AIRCRAFT_SEMAPHORE_NAME, O_CREAT, 0666, 1);
        if (radarSemaphore == SEM_FAILED) {
            perror("Error opening radar semaphore");
            exit(1);
        }

        alertsSemaphore = sem_open(ALERTS_SEMAPHORE_NAME, O_CREAT, 0666, 1);
        if (alertsSemaphore == SEM_FAILED) {
            perror("Error opening alerts semaphore");
            cleanupSemaphores();
            exit(1);
        }

        communicationSemaphore = sem_open(COMMUNICATION_SEMAPHORE_NAME, O_CREAT, 0666, 1);
        if (communicationSemaphore == SEM_FAILED) {
            perror("Error opening communication semaphore");
            cleanupSemaphores();
            exit(1);
        }

        dataDisplaySemaphore = sem_open(DATADISPLAY_SEMAPHORE_NAME, O_CREAT, 0666, 1);
        if (dataDisplaySemaphore == SEM_FAILED) {
            perror("Error opening data display semaphore");
            cleanupSemaphores();
            exit(1);
        }

        // Initialize shared memory for aircraft data
        aircraftShmFd = shm_open(AIRCRAFT_SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
        if (aircraftShmFd == -1) {
            perror("Error creating shared memory for aircraft data");
            cleanupSemaphores();
            exit(1);
        }

        shm_size = 2056;
        if (ftruncate(aircraftShmFd, shm_size) == -1) {
            perror("Error setting shared memory size");
            cleanupSemaphores();
            exit(1);
        }

        aircraftShmPtr = mmap(0, shm_size, PROT_WRITE, MAP_SHARED, aircraftShmFd, 0);
        if (aircraftShmPtr == MAP_FAILED) {
            perror("Error mapping shared memory");
            cleanupSemaphores();
            exit(1);
        }

        memset(aircraftShmPtr, 0, shm_size);

        // Initialize shared memory and semaphore for operator commands
        shm_fd_logs = shm_open(SHARED_MEMORY_LOGS, O_CREAT | O_RDWR, 0666);
        if (shm_fd_logs == -1) {
            perror("shm_open() for logs failed");
            exit(1);
        }

        if (ftruncate(shm_fd_logs, SHM_SIZE) == -1) {
            perror("ftruncate() for logs failed");
            exit(1);
        }

        shm_ptr_logs = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_logs, 0);
        if (shm_ptr_logs == MAP_FAILED) {
            perror("mmap() for logs failed");
            exit(1);
        }

        sem_logs = sem_open(SEMAPHORE_LOGS, O_CREAT, 0666, 1);
        if (sem_logs == SEM_FAILED) {
            perror("sem_open() for logs failed");
            exit(1);
        }

        // Initialize shared memory and semaphore for termination signal
        shm_fd_term = shm_open(SHARED_MEMORY_TERMINATION, O_CREAT | O_RDWR, 0666);
        if (shm_fd_term == -1) {
            perror("shm_open() for termination failed");
            exit(1);
        }

        if (ftruncate(shm_fd_term, SHM_SIZE) == -1) {
            perror("ftruncate() for termination failed");
            exit(1);
        }

        shm_ptr_term = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_term, 0);
        if (shm_ptr_term == MAP_FAILED) {
            perror("mmap() for termination failed");
            exit(1);
        }

        sem_term = sem_open(SEMAPHORE_TERMINATION, O_CREAT, 0666, 1);
        if (sem_term == SEM_FAILED) {
            perror("sem_open() for termination failed");
            exit(1);
        }
        // Initialize shared memory and semaphore for communication
        shm_fd_comm = shm_open(SHARED_MEMORY_COMMUNICATION, O_CREAT | O_RDWR, 0666);
        if (shm_fd_comm == -1) {
            perror("shm_open() for communication failed");
            exit(1);
        }

        if (ftruncate(shm_fd_comm, COMM_SHM_SIZE) == -1) {
            perror("ftruncate() for communication failed");
            exit(1);
        }

        shm_ptr_comm = mmap(0, COMM_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_comm, 0);
        if (shm_ptr_comm == MAP_FAILED) {
            perror("mmap() for communication failed");
            exit(1);
        }

        sem_comm = sem_open(SEMAPHORE_COMMUNICATION, O_CREAT, 0666, 1);
        if (sem_comm == SEM_FAILED) {
            perror("sem_open() for communication failed");
            exit(1);
        }
        // Initialize shared memory for alerts
        shm_fd_alerts = shm_open(ALERTS_SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
        if (shm_fd_alerts == -1) {
            perror("shm_open() for alerts failed");
            exit(1);
        }

        if (ftruncate(shm_fd_alerts, SHM_SIZE) == -1) {
            perror("ftruncate() for alerts failed");
            exit(1);
        }

        shm_ptr_alerts = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_alerts, 0);
        if (shm_ptr_alerts == MAP_FAILED) {
            perror("mmap() for alerts failed");
            exit(1);
        }

        // Initialize shared memory for aircraft data
        shm_fd_aircrafts = shm_open(AIRCRAFT_SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
        if (shm_fd_aircrafts == -1) {
            perror("shm_open() for aircraft data failed");
            exit(1);
        }

        if (ftruncate(shm_fd_aircrafts, SHM_SIZE) == -1) {
            perror("ftruncate() for aircraft data failed");
            exit(1);
        }

        shm_ptr_aircrafts = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_aircrafts, 0);
        if (shm_ptr_aircrafts == MAP_FAILED) {
            perror("mmap() for aircraft data failed");
            exit(1);
        }

        // Initialize semaphore for data display synchronization
        sem_display = sem_open(DATADISPLAY_SEMAPHORE_NAME, O_CREAT, 0666, 1);
        if (sem_display == SEM_FAILED) {
            perror("sem_open() for data display failed");
            exit(1);
        }
        shm_fd_augmentedInfo = shm_open(AUGMENTED_INFO_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
        if (shm_fd_augmentedInfo == -1) {
            perror("shm_open() for augmented information failed");
            exit(1);
        }

        if (ftruncate(shm_fd_augmentedInfo, SHM_SIZE) == -1) {
            perror("ftruncate() for augmented information failed");
            exit(1);
        }

        shm_ptr_augmentedInfo = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_augmentedInfo, 0);
        if (shm_ptr_augmentedInfo == MAP_FAILED) {
            perror("mmap() for augmented information failed");
            exit(1);
        }

        sem_augmentedInfo = sem_open("/augmented_info_semaphore", O_CREAT, 0666, 1);
        if (sem_augmentedInfo == SEM_FAILED) {
            perror("sem_open() for augmented information failed");
            exit(1);
        }
    }

    // Destructor
    ~Computer() {
        if (logFile.is_open()) {
            logFile << "Computer object destroyed." << endl;
            logFile.close();
        }

        cleanupSemaphores();
        if (shm_ptr_logs) {
            munmap(shm_ptr_logs, SHM_SIZE);
        }
        if (shm_fd_logs != -1) {
            close(shm_fd_logs);
        }
        shm_unlink(SHARED_MEMORY_LOGS);

        if (sem_logs) {
            sem_close(sem_logs);
            sem_unlink(SEMAPHORE_LOGS);
        }

        if (shm_ptr_term) {
            munmap(shm_ptr_term, SHM_SIZE);
        }
        if (shm_fd_term != -1) {
            close(shm_fd_term);
        }
        shm_unlink(SHARED_MEMORY_TERMINATION);

        if (sem_term) {
            sem_close(sem_term);
            sem_unlink(SEMAPHORE_TERMINATION);
        }
        if (shm_ptr_comm) {
            munmap(shm_ptr_comm, COMM_SHM_SIZE);
        }
        if (shm_fd_comm != -1) {
            close(shm_fd_comm);
        }
        shm_unlink(SHARED_MEMORY_COMMUNICATION);

        if (sem_comm) {
            sem_close(sem_comm);
            sem_unlink(SEMAPHORE_COMMUNICATION);
        }
        if (shm_ptr_augmentedInfo) {
            munmap(shm_ptr_augmentedInfo, SHM_SIZE);
        }
        if (shm_fd_augmentedInfo != -1) {
            close(shm_fd_augmentedInfo);
        }
        shm_unlink(AUGMENTED_INFO_MEMORY_NAME);

        if (sem_augmentedInfo) {
            sem_close(sem_augmentedInfo);
            sem_unlink("/augmented_info_semaphore");
        }
    }

    // Main function for the Computer class
    void run() {
        thread radarThread(&Computer::updateFromRadar, this);
        thread violationThread(&Computer::checkViolationsAndAlerts, this);
        thread loggingThread(&Computer::logAircraftData, this);
        thread operatorThread(&Computer::processOperatorCommands, this);
        thread aircraftThread(&Computer::aircraftDataThread, this);
        thread alertsThread(&Computer::alertsDataThread, this);
        thread displayThread(&Computer::dataDisplayThread, this);


        radarThread.join();
        violationThread.join();
        loggingThread.join();
        operatorThread.join();
        aircraftThread.join();
        alertsThread.join();
        displayThread.join();
    }

private:
    vector<Aircraft> aircrafts;

    priority_queue<Alert> alerts;
    mutex alertMutex;
    atomic<bool> terminate;
    sem_t* radarSemaphore;
    sem_t* alertsSemaphore;
    sem_t* communicationSemaphore;
    sem_t* dataDisplaySemaphore;
    int aircraftShmFd;
    void* aircraftShmPtr;
    size_t shm_size;
    ofstream logFile;

    void cleanupSemaphores() {
        if (radarSemaphore) sem_close(radarSemaphore);
        if (alertsSemaphore) sem_close(alertsSemaphore);
        if (communicationSemaphore) sem_close(communicationSemaphore);
        if (dataDisplaySemaphore) sem_close(dataDisplaySemaphore);

        sem_unlink(AIRCRAFT_SEMAPHORE_NAME);
        sem_unlink(ALERTS_SEMAPHORE_NAME);
        sem_unlink(COMMUNICATION_SEMAPHORE_NAME);
        sem_unlink(DATADISPLAY_SEMAPHORE_NAME);

        if (aircraftShmPtr) {
            munmap(aircraftShmPtr, shm_size);
        }
        if (aircraftShmFd != -1) {
            close(aircraftShmFd);
        }
        shm_unlink(AIRCRAFT_SHARED_MEMORY_NAME);
    }

    void updateFromRadar() {
        cout << "Initializing radar shared memory and semaphore..." << endl;

        // Open the semaphore
        sem_plane = sem_open(sem_name, O_CREAT, 0777, 1);
        if (sem_plane == SEM_FAILED) {
            perror("Failed to open radar semaphore");
            exit(EXIT_FAILURE);
        }

        // Open the shared memory
        shm_fdd = shm_open(shared_name, O_RDWR, 0777);
        if (shm_fdd == -1) {
            perror("Failed to open radar shared memory");
            sem_close(sem_plane);
            exit(EXIT_FAILURE);
        }

        // Map the shared memory
        sharedAircraftList = (SharedAircraft*)mmap(0, sizeof(SharedAircraft) * 10, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fdd, 0);
        if (sharedAircraftList == MAP_FAILED) {
            perror("Failed to map radar shared memory");
            sem_close(sem_plane);
            close(shm_fdd);
            exit(EXIT_FAILURE);
        }

        cout << "Radar shared memory and semaphore initialized successfully." << endl;

        // Periodically update the aircrafts vector with radar data
        while (!terminate) {
            this_thread::sleep_for(chrono::seconds(5)); // Update every 5 seconds

            sem_wait(sem_plane); // Lock the semaphore

            {
                lock_guard<mutex> lock(air_mutex); // Protect access to the aircrafts vector
                lock_guard<mutex> aircraftlock(aircraftsMutex);
                // Clear the current aircraft list
                aircrafts.clear();

                // Read data from shared memory and populate the aircrafts vector
                for (int i = 0; i < 8; i++) { // Assuming a maximum of 8 aircrafts
                    SharedAircraft& radarAircraft = sharedAircraftList[i];

                    // Skip empty or uninitialized entries
                    if (radarAircraft.aircraftID == 0) {
                        continue;
                    }

                    // Convert SharedAircraft to Aircraft and add to the vector
                    aircrafts.emplace_back(
                        radarAircraft.startTime,
                        radarAircraft.aircraftID,
                        radarAircraft.positionX,
                        radarAircraft.positionY,
                        radarAircraft.positionZ,
                        radarAircraft.speedX,
                        radarAircraft.speedY,
                        radarAircraft.speedZ,
						0
                    );
                }
            }

            sem_post(sem_plane); // Unlock the semaphore

            cout << "Aircraft data updated from radar." << endl;
        }

        // Cleanup shared memory and semaphore
        munmap(sharedAircraftList, sizeof(SharedAircraft) * 10);
        close(shm_fdd);
        sem_close(sem_plane);
    }

    void checkViolations() {
        while (!terminate) {
            this_thread::sleep_for(chrono::seconds(5));

            lock_guard<mutex> lock(alertMutex);
            for (size_t i = 0; i < aircrafts.size(); ++i) {
                for (size_t j = i + 1; j < aircrafts.size(); ++j) {
                    Aircraft a1 = aircrafts[i];
                    Aircraft a2 = aircrafts[j];

                    if (violationCheck(&a1, &a2)) {
                        alerts.push({0, "Separation violation detected between " + to_string(a1.getAircraftID()) + " and " + to_string(a2.getAircraftID())});
                    }
                }
            }

            sem_wait(dataDisplaySemaphore);
            while (!alerts.empty()) {
                Alert alert = alerts.top();
                alerts.pop();
                cout << "Sending alert to data display: " << alert.message << endl;
            }
            sem_post(dataDisplaySemaphore);
        }
    }

    //just adds data into log. use this for the history
    void logAircraftData() {
        while (!terminate) {
            this_thread::sleep_for(chrono::seconds(20));

            lock_guard<mutex> lock(alertMutex);
            logFile << "Logging aircraft data..." << endl;
            for (auto& aircraft : aircrafts) {
                logFile << "Aircraft ID: " << aircraft.getAircraftID()
                        << ", Position: (" << aircraft.getPositionX() << ", "
                        << aircraft.getPositionY() << ", " << aircraft.getPositionZ() << ")"
						<< ", Velocity: (" << aircraft.getSpeedX() << ", "
                        << aircraft.getSpeedY() << ", " << aircraft.getSpeedZ() << ")" << endl;
            }
        }
    }

    void processOperatorCommands() {
            while (!terminate) {
                this_thread::sleep_for(chrono::seconds(1)); // Periodic task

                sem_wait(sem_logs); // Lock semaphore for logs

                // Read the command from shared memory
                string command(static_cast<char*>(shm_ptr_logs));

                if (!command.empty()) {
                    cout << "Processing operator command: " << command << endl;

                    // Parse the command
                    istringstream iss(command);
                    string timestamp, commandType, aircraftID;
                    iss >> timestamp >> commandType >> aircraftID;

                    string communicationMessage;

                    if (commandType == "Speed_Change") {
                        int newSpeedX, newSpeedY, newSpeedZ;
                        iss >> newSpeedX >> newSpeedY >> newSpeedZ;
                        communicationMessage = "Speed_Change " + aircraftID + " " +
                                               to_string(newSpeedX) + " " +
                                               to_string(newSpeedY) + " " +
                                               to_string(newSpeedZ);

                        sendToCommunication(aircraftID, communicationMessage);

                    } else if (commandType == "Augmented_Information") {
                        //todo: add augmented info method
                    }

                    // Clear the shared memory for logs after processing the command
                               memset(shm_ptr_logs, 0, SHM_SIZE);
                               cout << "Shared memory for logs cleared after processing command." << endl;
                }

                sem_post(sem_logs); // Unlock semaphore for logs
            }
        }

    //protected send() function.
    void sendToCommunication(const string& aircraft, const string& command) {
        sem_wait(sem_comm);
        // Write the message to shared memory
            char* mem = static_cast<char*>(shm_ptr_comm);
            memset(mem, 0, COMM_SHM_SIZE); // Clear the shared memory

            // Ensure we don't exceed the shared memory size
            if (command.size() >= COMM_SHM_SIZE) {
                cerr << "Shared memory full, unable to write communication message." << endl;
                sem_post(sem_comm); // Unlock semaphore
                return;
            }

            memcpy(mem, command.c_str(), command.size());

        sem_post(sem_comm);
    }

    void checkViolationsAndAlerts() {
        while (!terminate) {
            this_thread::sleep_for(chrono::seconds(3)); // Periodic task every 5 seconds

            lock_guard<mutex> lock(alertMutex); // Protect access to the alerts queue
            lock_guard<mutex> aircraftslock(aircraftsMutex);

            for (size_t i = 0; i < aircrafts.size(); ++i) {
                for (size_t j = i + 1; j < aircrafts.size(); ++j) {
                    Aircraft& a1 = aircrafts[i];
                    Aircraft& a2 = aircrafts[j];

                    // Check for separation violations
                    if (violationCheck(&a1, &a2)) {
                        alerts.push({0, "Separation violation detected between " + to_string(a1.getAircraftID()) + " and " + to_string(a2.getAircraftID())});
                        a1.setIsViolation(1);
                        a2.setIsViolation(1);
                        cout << "Violation found. Taking necessary actions." << endl;
                    }
                    else{

                    // Check for potential collisions
                    auto [collisionDetected, collisionTime] = collisionCheck(&a1, &a2);
                    if (collisionDetected) {
                        alerts.push({collisionTime, "Collision will occur in " + to_string(collisionTime) + " seconds between " + to_string(a1.getAircraftID()) + " and " + to_string(a2.getAircraftID())});
                        a1.setIsViolation(1);
                        a2.setIsViolation(1);
                    }

                    }
                }
            }

            // Send alerts to data display
            sendAlertsToDataDisplay();
        }
    }
//    void sendAircrafts() {
//    	sem_wait(dataDisplaySemaphore);
//    	//TODO: ADD SHARED MEMORY PORTION HERE
//
//    }
    void sendAircrafts() {
        sem_wait(sem_display); // Lock semaphore for aircraft data
        lock_guard<mutex> lock(aircraftsMutex); // Protect access to aircraft shared memory

        // Clear the shared memory for aircraft data
        memset(shm_ptr_aircrafts, 0, SHM_SIZE);

        // Write aircraft data to shared memory
        char* mem = static_cast<char*>(shm_ptr_aircrafts);
        for (auto& aircraft : aircrafts) {
            string data = to_string(aircraft.getAircraftID()) + " " +
                          to_string(aircraft.getPositionX()) + " " +
                          to_string(aircraft.getPositionY()) + " " +
                          to_string(aircraft.getPositionZ()) + " " +
						  to_string(aircraft.getIsViolation()) +"\n";

            // Ensure we don't exceed the shared memory size
            if (mem + data.size() > static_cast<char*>(shm_ptr_aircrafts) + SHM_SIZE) {
                cerr << "Shared memory full, unable to write more aircraft data." << endl;
                break;
            }

            memcpy(mem, data.c_str(), data.size());
            mem += data.size();
        }

        sem_post(dataDisplaySemaphore); // Unlock semaphore for aircraft data
    }

    void sendAlertsToDataDisplay() {
        sem_wait(dataDisplaySemaphore); // Lock semaphore for data display

        lock_guard<mutex> lock(alertsMutex); // Protect access to alerts shared memory

        // Clear the shared memory for alerts
        memset(shm_ptr_alerts, 0, SHM_SIZE);

        // Write alert data to shared memory
        char* mem = static_cast<char*>(shm_ptr_alerts);
        while (!alerts.empty()) {
            Alert alert = alerts.top();
            alerts.pop();

            string data = "ALERT: Time: " + to_string(alert.time) +
                          ", Message: " + alert.message + "\n";

            // Ensure we don't exceed the shared memory size
            if (mem + data.size() > static_cast<char*>(shm_ptr_alerts) + SHM_SIZE) {
                cerr << "Shared memory full, unable to write more alert data." << endl;
                break;
            }

            memcpy(mem, data.c_str(), data.size());
            mem += data.size();
        }

        sem_post(dataDisplaySemaphore); // Unlock semaphore for data display
    }

    void augmentInformation(int aircraftID) {
        sem_wait(sem_augmentedInfo); // Lock semaphore for augmented info

        lock_guard<mutex> lock(aircraftsMutex); // Protect access to the aircrafts vector

        // Search for the aircraft in the vector
        auto it = find_if(aircrafts.begin(), aircrafts.end(), [aircraftID](Aircraft& aircraft) {
            return aircraft.getAircraftID() == aircraftID;
        });

        if (it == aircrafts.end()) {
            cerr << "Aircraft with ID " << aircraftID << " not found." << endl;
            sem_post(sem_augmentedInfo); // Unlock semaphore
            return;
        }

        // Get the aircraft's information
        Aircraft& aircraft = *it;
        string data = to_string(aircraft.getAircraftID()) + " " +
                to_string(aircraft.getPositionX()) + " " +
                to_string(aircraft.getPositionY()) + " " +
                to_string(aircraft.getPositionZ()) + " " +
				to_string(aircraft.getSpeedX()) + " " +
                to_string(aircraft.getSpeedY()) + " " +
                to_string(aircraft.getSpeedZ());

        // Write the data to shared memory
        char* mem = static_cast<char*>(shm_ptr_augmentedInfo);
        memset(mem, 0, SHM_SIZE); // Clear the shared memory

        // Ensure we don't exceed the shared memory size
        if (data.size() >= SHM_SIZE) {
            cerr << "Shared memory full, unable to write augmented info." << endl;
            sem_post(sem_augmentedInfo); // Unlock semaphore
            return;
        }

        memcpy(mem, data.c_str(), data.size());

        sem_post(sem_augmentedInfo); // Unlock semaphore
        cout << "Augmented info for aircraft " << aircraftID << " sent to shared memory." << endl;
    }

//     void sendAlert(const string& message) {
//        cout << "ALERT: " << message << endl;
//        logMessage("ALERT: " + message); // Log the alert
//
//        lock_guard<mutex> lock(alertsMutex); // Protect access to alerts shared memory
//
//        // Append the alert message to shared memory
//        char* mem = static_cast<char*>(shm_ptr_alerts);
//
//        // Find the end of the current data in shared memory
//        while (*mem != '\0' && mem < static_cast<char*>(shm_ptr_alerts) + SHM_SIZE) {
//            mem++;
//        }
//
//        string data = "ALERT: " + message + "\n";
//
//        // Ensure we don't exceed the shared memory size
//        if (mem + data.size() > static_cast<char*>(shm_ptr_alerts) + SHM_SIZE) {
//            cerr << "Shared memory full, unable to write alert." << endl;
//            return;
//        }
//
//        memcpy(mem, data.c_str(), data.size());
//    }

    bool violationCheck(Aircraft* a1, Aircraft* a2) {
        double dx = abs(a1->getPositionX() - a2->getPositionX());
        double dy = abs(a1->getPositionY() - a2->getPositionY());
        double dz = abs(a1->getPositionZ() - a2->getPositionZ());

        return ((dx < 3000 || dy < 3000) && dz < 1000);
    }

    tuple<bool, double> collisionCheck(Aircraft* a1, Aircraft* a2) {
        const int maxTime = 120;
        const double horizontalThreshold = 3000.0;
        const double verticalThreshold = 1000.0;

        double dx0 = a1->getPositionX() - a2->getPositionX();
        double dy0 = a1->getPositionY() - a2->getPositionY();
        double dz0 = a1->getPositionZ() - a2->getPositionZ();
        double dvx = a1->getSpeedX() - a2->getSpeedX();
        double dvy = a1->getSpeedY() - a2->getSpeedY();
        double dvz = a1->getSpeedZ() - a2->getSpeedZ();

        double A = dvx * dvx + dvy * dvy;
        double B = 2 * (dx0 * dvx + dy0 * dvy);
        double C = dx0 * dx0 + dy0 * dy0;

        double t_h_first = numeric_limits<double>::infinity();
        double t_v_first = numeric_limits<double>::infinity();

        bool horizontalValid = (A == 0) ? (sqrt(C) <= horizontalThreshold) : solveQuadraticFirstTime(A, B, C, horizontalThreshold, t_h_first);
        bool verticalValid = solveLinearFirstTime(dz0, dvz, verticalThreshold, t_v_first);

        if (!horizontalValid || !verticalValid) {
            return make_tuple(false, -1.0);
        }

        double t_first_alert = max(t_h_first, t_v_first);

        if (t_first_alert <= maxTime) {
            return make_tuple(true, t_first_alert);
        }

        return make_tuple(false, -1.0);
    }

    bool solveQuadraticFirstTime(double A, double B, double C, double R, double& t_first) {
        C -= R * R;

        double discriminant = B * B - 4 * A * C;
        if (discriminant < 0) return false;

        double sqrtD = sqrt(discriminant);
        double t1 = (-B - sqrtD) / (2 * A);
        double t2 = (-B + sqrtD) / (2 * A);

        if (t1 >= 0) t_first = t1;
        else if (t2 >= 0) t_first = t2;
        else return false;

        return true;
    }

    bool solveLinearFirstTime(double dz0, double dvz, double R, double& t_first) {
        if (dvz == 0) {
            if (abs(dz0) <= R) {
                t_first = 0;
                return true;
            } else {
                return false;
            }
        }

        double t1 = (R - dz0) / dvz;
        double t2 = (-R - dz0) / dvz;

        if (t1 > t2) swap(t1, t2);

        if (t1 >= 0) t_first = t1;
        else if (t2 >= 0) t_first = t2;
        else return false;

        return true;
    }

    void logMessage(const string& message) {
        if (logFile.is_open()) {
            logFile << message << endl;
        }
    }
    void aircraftDataThread() {
        while (!terminate) {
            this_thread::sleep_for(chrono::seconds(5)); // Update every 5 seconds
            sendAircrafts();
            cout << "Aircraft data sent to shared memory." << endl;
        }
    }
    void alertsDataThread() {
        while (!terminate) {
            this_thread::sleep_for(chrono::seconds(5)); // Update every 5 seconds
            sendAlertsToDataDisplay();
            cout << "Alerts sent to shared memory." << endl;
        }
    }
    void dataDisplayThread() {
        while (!terminate) {
            sem_wait(dataDisplaySemaphore); // Wait for data to be updated
            cout << "Data display system is reading updated data." << endl;
            sem_post(dataDisplaySemaphore); // Allow further updates
            this_thread::sleep_for(chrono::seconds(5)); // Simulate data display processing
        }
    }




public:
//    void printData() {
//    cout << "Semaphore opened successfully." << endl;
//	sem_plane=sem_open(sem_name, O_CREAT, 0777,1);
//	if(sem_plane==SEM_FAILED){
//		perror("Failed to open");
//		exit(EXIT_FAILURE);
//	}
//	cout << "Semaphore opened successfully." << endl;
//
//    lock_guard<mutex> lock(air_mutex);
//
//    shm_fdd=shm_open(shared_name,O_RDWR, 0777);
//    if(shm_fdd==-1){
//    	perror("failed");
//    	sem_post(sem_plane);
//    	exit(EXIT_FAILURE);
//    }
//
//    sharedAircraftList=(SharedAircraft*)mmap(0,sizeof(SharedAircraft)*10, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fdd, 0);
//    if(sharedAircraftList==MAP_FAILED){
//    	perror("Failed to map");
//    	sem_post(sem_plane);
//    	exit(EXIT_FAILURE);
//    }
//    while(true){
//    	sem_wait(sem_plane);
//    	for (int i = 0; i < 8; i++) {
//        SharedAircraft& aircraft = sharedAircraftList[i];
//
//        cout << "ID: " << aircraft.aircraftID
//             << " | X: " << aircraft.positionX
//             << " | Y: " << aircraft.positionY
//             << " | Z: " << aircraft.positionZ << endl;
//    	}
//
//
//    	close(shm_fdd);
//    	sem_post(sem_plane);
//    	sleep(1);
//    }
//    	cout << endl;
//    }
};
