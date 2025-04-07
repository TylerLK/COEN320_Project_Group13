#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>

using namespace std;

// Names of all the shared memory used in the system.
const char *SHARED_MEMORY_LOGS = "/shm_logs";
const char *SHARED_MEMORY_TERMINATION = "/shm_term";
const char *SHARED_MEMORY_AIRCRAFT_DATA = "/AircraftData";
const char *SHARED_MEMORY_AUGMENTED_DATA = "/AugmentedData";
const char *SHARED_MEMORY_ALERTS = "/AlertsData";
const char *SHARED_MEMORY_COMMUNICATIONS = "/shm_communication";
const char *SHARED_MEMORY_RADAR = "/radar_shm";

// Names of all the semaphores used in the system.
const char *SEMAPHORE_LOGS = "/logs_semaphore";
const char *SEMAPHORE_TERMINATION = "/term_semaphore";
const char *SEMAPHORE_DATA = "/data_semaphore";
const char *SEMAPHORE_COMMUNICATIONS = "/comm_semaphore";
const char *AIRCRAFT_SEMAPHORE_NAME = "/aircraft_semaphore";
const char *SEMAPHORE_RADAR = "/radar_semaphore";
const char *ALERTS_SEMAPHORE_NAME = "/alerts_semaphore";
const char *COMMUNICATION_SEMAPHORE_NAME = "/communication_semaphore";
const char *DATADISPLAY_SEMAPHORE_NAME = "/datadisplay_semaphore";

int main()
{
    // Unlink shared memory
    if (shm_unlink(SHARED_MEMORY_LOGS) == -1)
    {
        perror("Error unlinking SHARED_MEMORY_LOGS");
    }

    if (shm_unlink(SHARED_MEMORY_TERMINATION) == -1)
    {
        perror("Error unlinking SHARED_MEMORY_TERMINATION");
    }

    if (shm_unlink(SHARED_MEMORY_AIRCRAFT_DATA) == -1)
    {
        perror("Error unlinking SHARED_MEMORY_AIRCRAFT_DATA");
    }

    if (shm_unlink(SHARED_MEMORY_AUGMENTED_DATA) == -1)
    {
        perror("Error unlinking SHARED_MEMORY_AUGMENTED_DATA");
    }

    if (shm_unlink(SHARED_MEMORY_ALERTS) == -1)
    {
        perror("Error unlinking SHARED_MEMORY_ALERTS");
    }

    if (shm_unlink(SHARED_MEMORY_COMMUNICATIONS) == -1)
    {
        perror("Error unlinking SHARED_MEMORY_COMMUNICATIONS");
    }

    if (shm_unlink(SHARED_MEMORY_RADAR) == -1)
    {
        perror("Error unlinking SHARED_MEMORY_RADAR");
    }

    // Unlink semaphores
    if (sem_unlink(SEMAPHORE_LOGS) == -1)
    {
        perror("Error unlinking SEMAPHORE_LOGS");
    }

    if (sem_unlink(SEMAPHORE_TERMINATION) == -1)
    {
        perror("Error unlinking SEMAPHORE_TERMINATION");
    }

    if (sem_unlink(SEMAPHORE_DATA) == -1)
    {
        perror("Error unlinking SEMAPHORE_DATA");
    }

    if (sem_unlink(SEMAPHORE_COMMUNICATIONS) == -1)
    {
        perror("Error unlinking SEMAPHORE_COMMUNICATIONS");
    }

    if (sem_unlink(AIRCRAFT_SEMAPHORE_NAME) == -1)
    {
        perror("Error unlinking AIRCRAFT_SEMAPHORE_NAME");
    }

    if (sem_unlink(SEMAPHORE_RADAR) == -1)
    {
        perror("Error unlinking SEMAPHORE_RADAR");
    }

    if (sem_unlink(ALERTS_SEMAPHORE_NAME) == -1)
    {
        perror("Error unlinking ALERTS_SEMAPHORE_NAME");
    }

    if (sem_unlink(COMMUNICATION_SEMAPHORE_NAME) == -1)
    {
        perror("Error unlinking COMMUNICATION_SEMAPHORE_NAME");
    }

    if (sem_unlink(DATADISPLAY_SEMAPHORE_NAME) == -1)
    {
        perror("Error unlinking DATADISPLAY_SEMAPHORE_NAME");
    }

    cout << "All shared memory and semaphores have been unlinked." << endl;
}