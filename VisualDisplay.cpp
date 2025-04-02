#include <iostream>
#include <string>
#include <cstring>
#include <cmath>  // Used for round();
#include <vector> // Used to store aircraft data.
#include <array>  // Used to store aircraft data.
#include <tuple>
#include <ctime>       // Used to create a timestamp.
#include <semaphore.h> // Used to define semaphores for inter-process synchronization.
#include <fcntl.h>     // Used to open shared memory.
#include <sys/mman.h>  // Used to map shared memory to an address space.
#include <sys/stat.h>  // Used to define file permissions.
#include <pthread.h>   // Used to create pthreads.
#include <unistd.h>    // Used to allow the threads to sleep.

using namespace std;

// Global Variables
bool augmentedAircraftsPresent = false; // Indicates if any augmented information was requested by the operator.
bool violationsPresent = false;         // Indicates if any violations have been detected.
bool terminateNow = false;              // Indicates if the subsystem should be terminating.
const int rows = 50;                    // Number of rows in the airspace grid.
const int columns = 100;                // Number of columns in the airspace grid.
const int SHM_SIZE = 4096;

const char *SHARED_MEMORY_AIRCRAFT_DATA = "/AircraftData";   // Name for the shared memory used to store all the regular aircrafts.
const char *SHARED_MEMORY_AUGMENTED_DATA = "/AugmentedData"; // Name for the shared memory used to store all the augmented aircrafts.
const char *SHARED_MEMORY_ALERTS = "/AlertsData";            // Name for the shared memory used to store all the violations.
const char *SEMAPHORE_DATA = "/data_semaphore";              // Name for the semaphore used to synchronize the Visual Display and the Computer.

const char *SHARED_MEMORY_TERMINATION = "/shm_term";   // Name for the shared memory used to terminate the system.
const char *SEMAPHORE_TERMINATION = "/term_semaphore"; // Name for the semaphore used to synchronize all processes for the termination of the RTOS.

// Containers for different aircraft data.
vector<array<string, 5>> regularAircraftData;   // Holds the current regular aircraft data.
vector<array<string, 8>> augmentedAircraftData; // Holds the current augmented aircraft data.
vector<tuple<int, int>> aircraftGridPositions;  // Holds the positions of the aircrafts in the visual display.
vector<string> violations;                      // Holds a list of violations, both present and future.

// Function Prototypes
void insertBanner(string title);
string getCurrentTimestamp();
vector<tuple<int, int>> calculateAirspacePositions(vector<tuple<int, int>> currentPositions, vector<array<string>> newAircrafts, int arraySize);
void drawAirspace(vector<string> regularAircrafts, vector<string> augmentedAricrafts, vector<tuple<int, int>> gridPositions);
void *aircraftDataHandling(void *arg); // This function will be ran by the thread to print the regular visual display.
void *violationHandling(void *arg);    // This function will be ran by the thread to print the violations.
void *terminationHandling(void *arg);  // This function will be ran by the thread to terminate the system.

int main()
{
    /* TODO:
        1. Display a view of the space every 5 seconds, showing the current position of each aircraft.
        2. Display a notification(s) if a safety violation occurs:
            - Listening for notifications from the Computer Subsystem (Readable Shared Memory).
            - Displaying a visual notification to the operator.
            - (OPTIONAL) Emitting a sonorous alarm.
        3. The following should be shown:
            - y-axis vs x-axis (100000ft by 100000ft), represented in a 100x100 grid
            - Middle dot for blank space, "A" for aircrafts, and "X" for aircrafts that are in violation.
            - A list of the aircrafts, their IDs, and their positions.
        4. The main thread should loop every five seconds, which will require a posix thread and a 5-second timer.
    */

    // Initial greeting message.
    cout << "Welcome to the Visual Display Subsystem!" << endl
         << "Information regarding aircrafts will appear below..." << endl;

    // Open all shared memory files.
    // Data
    // Regular Aircrafts
    int shm_fd_reg = shm_open(SHARED_MEMORY_AIRCRAFT_DATA, O_RDONLY, 0666);
    if (shm_fd_reg == -1)
    {
        perror("shm_open() for the regular aircrafts failed: "); // This will print the String argument with the errno value appended.
        exit(1);
    }

    // Resize the shared memory for the regular aircrafts.
    int size_reg = ftruncate(shm_fd_reg, SHM_SIZE);
    if (size_reg == -1)
    {
        perror("ftruncate() resizing for the regular aircrafts failed: "); // This will print the String argument with the errno value appended.
        return -1;
    }

    // Mapping the shared memory into the Visual Display's address space.
    void *shm_ptr_reg = mmap(0, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd_reg, 0);
    if (shm_ptr_reg == MAP_FAILED)
    {
        cerr << "Shared Memory Mapping for the augmented aircrafts failed..." << endl;
        return -1;
    }

    // Augmented Aircrafts
    int shm_fd_aug = shm_open(SHARED_MEMORY_AUGMENTED_DATA, O_RDONLY, 0666);
    if (shm_fd_aug == -1)
    {
        perror("shm_open() for the augmented aircrafts failed: "); // This will print the String argument with the errno value appended.
        exit(1);
    }

    // Resize the shared memory for the regular aircrafts.
    int size_aug = ftruncate(shm_fd_aug, SHM_SIZE);
    if (size_aug == -1)
    {
        perror("ftruncate() resizing for the augmented aircrafts failed: "); // This will print the String argument with the errno value appended.
        return -1;
    }

    // Mapping the shared memory into the Visual Display's address space.
    void *shm_ptr_aug = mmap(0, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd_aug, 0);
    if (shm_ptr_aug == MAP_FAILED)
    {
        cerr << "Shared Memory Mapping for the augmented aircrafts failed..." << endl;
        return -1;
    }

    // Violations
    int shm_fd_viol = shm_open(SHARED_MEMORY_ALERTS, O_RDONLY, 0666);
    if (shm_fd_viol == -1)
    {
        perror("shm_open() for the violations failed: "); // This will print the String argument with the errno value appended.
        exit(1);
    }

    // Resize the shared memory for the regular aircrafts.
    int size_viol = ftruncate(shm_fd_viol, SHM_SIZE);
    if (size_aug == -1)
    {
        perror("ftruncate() resizing for the violations failed: "); // This will print the String argument with the errno value appended.
        return -1;
    }

    // Mapping the shared memory into the Visual Display's address space.
    void *shm_ptr_viol = mmap(0, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd_viol, 0);
    if (shm_ptr_viol == MAP_FAILED)
    {
        cerr << "Shared Memory Mapping for the violations failed..." << endl;
        return -1;
    }

    // Termination
    int shm_fd_term = shm_open(SHARED_MEMORY_TERMINATION, O_CREAT | O_RDWR, 0666);
    if (shm_fd_term == -1)
    {
        perror("shm_open() for termination failed(): ");
        exit(1);
    }

    // Resize the shared memory for termination.
    int size_term = ftruncate(shm_fd_term, SHM_SIZE);
    if (size_term == -1)
    {
        perror("ftruncate() resizing for termination failed: "); // This will print the String argument with the errno value appended.
        return -1;
    }

    // Mapping the shared memory into the Operator's address space.
    void *shm_ptr_term = mmap(0, SHM_SIZE, PROT_WRITE, MAP_SHARED, shm_fd_term, 0);
    if (shm_ptr_term == MAP_FAILED)
    {
        cerr << "Shared Memory Mapping for termination failed..." << endl;
        return -1;
    }

    // Create the Semaphores that the Operator needs to synchronize with other processes in shared memory.
    sem_t *sem_data = sem_open(SEMAPHORE_DATA, O_RDONLY, 0666, 1);
    if (sem_data == SEM_FAILED)
    {
        perror("sem_open() for all data has failed: ");
        return -1;
    }

    sem_t *sem_term = sem_open(SEMAPHORE_TERMINATION, O_RDONLY, 0666, 1);
    if (sem_term == SEM_FAILED)
    {
        perror("sem_open() for termination has failed");
        return -1;
    }

    // Create the threads that are needed to organize all the tasks in the Visual Display subsystem.
    // Data
    pthread_t thread_data; // Checks for regular and augmented aircrafts every 5 seconds and prints their location and information.
    int create_thread_data = pthread_create(&thread_data, nullptr, aircraftDataHandling, nullptr);
    if (create_thread_data != 0)
    {
        cout << "Failed to create thread_data..." << endl;
        return EXIT_FAILURE;
    }

    // Violations
    pthread_t thread_viol; // Checks for violations.
    int create_thread_viol = pthread_create(&thread_viol, nullptr, violationHandling, nullptr);
    if (create_thread_viol != 0)
    {
        cout << "Failed to create thread_viol..." << endl;
        return EXIT_FAILURE;
    }

    // Termination
    pthread_t thread_term; // Checks for termination signal from the Operator subsystem.
    int create_thread_term = pthread_create(&thread_term, nullptr, terminationHandling, nullptr);
    if (create_thread_term != 0)
    {
        cout << "Failed to create thread_term..." << endl;
    }

    // Read the shared memory containing all of the data.
    sem_wait(sem_data);

    // Read the shared memory that holds the regular aircraft data.
    char *regularData = static_cast<char *>(shm_ptr_reg);

    // Turn the regularData character buffer into a string, to ensure that it is null-terminated.
    string regularDataString(regularData);

    // Create a stringstream object from the shared memory string, so that it can be parsed into individual lines.
    stringstream regularDataStringStream(regularDataString);

    // Create a string representing a single line of the regular aircraft data.
    string regularDataLine;

    // Read each line from the stringstream individually.
    while (getline(regularDataStringStream, regularDataLine))
    {
        // Create a temporary array of 5 strings that represents a deconstrcuted line of data.
        array<string, 5> regularDataLineDeconstructed;

        // Create a stringstream object from the current line of data that is being read.
        stringstream regularDataLineStringStream(regularDataLine);

        // Create a string representing a single aircraft variable.
        string regularDataVariable;

        // Add each variable to the string array.
        for (int i = 0; i < regularDataLineDeconstructed.size(); i++)
        {
            if (getline(regularDataLineStringStream, regularDataVariable, ' '))
            {
                regularDataLineDeconstructed[i] = regularDataVariable;
            }
            else
            {
                cerr << "Reading the data has failed: " << regularDataVariable << endl;
                break;
            }
        }

        // Append the line of regular aircraft data to its associated vector
        regularAircraftData.push_back(regularDataLineDeconstructed);
    }

    // Calculate the position of each regular aircraft that will be displayed on the grid.
    aircraftGridPositions = calculateAirspacePositions(aircraftGridPositions, regularAircraftData);

    // TODO: Read the shared memory that holds the augmented aircraft data.
    if (augmentedAircraftData.size() != 0)
    {
        augmentedAircraftsPresent = true; // Augmented aircraft data is present in the system.

        aircraftGridPositions = calculateAirspacePositions(aircraftGridPositions, augmentedAircraftData);
    }

    // Read the shared memory that holds the violation data

    sem_post(sem_data);

    // Beginning of the visual display
    insertBanner("Monitored En-Route Airspace" + getCurrentTimestamp());

    // Print the current state of the airspace.
    drawAirspace(regularAircraftData, augmentedAircraftData, aircraftGridPositions);

    // Print all regular aircraft data.
    insertBanner("Generic Aicraft Information");
    for (int i = 0; i < regularAircraftData.size(); i++)
    {
        cout << regularAircraftData[i] << endl; // Print all the regular aircraft data, line-by-line.
    }

    // Print all augmented aircraft data.
    if (augmentedAircraftsPresent) // Augmented aircraft data is present.
    {
        insertBanner("Augmented Aircraft Information");
        for (int i = 0; i < augmentedAircraftData.size(); i++)
        {
            cout << augmentedAircraftData[i] << endl; // Print all the augmented aircraft data, line-by-line.
        }
    }
    // End of the visual display.

    // Clear all of the vectors
    regularAircraftData.clear();
    augmentedAircraftData.clear();
    violations.clear();

    pthread_join(thread_data, NULL);
    pthread_join(thread_viol, NULL);
    pthread_join(thread_term, NULL);

    sem_wait(sem_data); // The shared memory should block other processes while being cleaned up.
                        // Clean up the shared memory for the data.
    // Unmaps the shared memory for the regular aircrafts.
    int unmap_reg = munmap(shm_ptr_reg, SHM_SIZE);
    if (unmap_reg == -1)
    {
        perror("munmap() for the regular aircrafts failed: "); // This will print the String argument with the errno value appended.
        return false;
    }

    // Closes the shared memory file.
    int close_reg = close(shm_fd_reg);
    if (close_reg == -1)
    {
        perror("close() for regular aircrafts failed: "); // This will print the String argument with the errno value appended.
        return false;
    }

    // Unmaps the shared memory for the augmented aircrafts.
    int unmap_aug = munmap(shm_ptr_aug, SHM_SIZE);
    if (unmap_aug == -1)
    {
        perror("munmap() for the augmented aircrafts failed: "); // This will print the String argument with the errno value appended.
        return false;
    }

    // Closes the shared memory file.
    int close_aug = close(shm_fd_aug);
    if (close_aug == -1)
    {
        perror("close() for the augmented aircrafts failed: "); // This will print the String argument with the errno value appended.
        return false;
    }

    // Unmaps the shared memory for the violations.
    int unmap_viol = munmap(shm_ptr_viol, SHM_SIZE);
    if (unmap_viol == -1)
    {
        perror("munmap() for the violations failed: "); // This will print the String argument with the errno value appended.
        return false;
    }

    // Closes the shared memory file.
    int close_viol = close(shm_fd_aug);
    if (close_viol == -1)
    {
        perror("close() for the violations failed: "); // This will print the String argument with the errno value appended.
        return false;
    }
    sem_post(sem_data);

    // Close the data semaphore.
    int close_data_semaphore = sem_close(sem_data);
    if (close_data_semaphore == -1)
    {
        perror("sem_close() for all data failed: ");
        return false;
    }

    sem_wait(sem_term); // The shared memory should block other processes while being cleaned up.
    // Clean up the shared memory for the termination signal.
    // Unmaps the shared memory for the termination.
    int unmap_term = munmap(ptr_term, SHM_SIZE);
    if (unmap_term == -1)
    {
        perror("munmap() for termination failed: "); // This will print the String argument with the errno value appended.
        return false;
    }

    // Closes the shared memory file.
    int close_term = close(fd_term);
    if (close_term == -1)
    {
        perror("close() for termination failed: "); // This will print the String argument with the errno value appended.
        return false;
    }
    sem_post(sem_term);

    // Close the termination semaphore.
    int close_termination_semaphore = sem_close(sem_term);
    if (close_termination_semaphore == -1)
    {
        perror("sem_close() for termination failed: ");
        return false;
    }

    return 0;
}

void insertBanner(string title)
{
    cout << endl
         << "===================================================================================================" << endl
         << title << endl
         << "==================================================================================================="
         << endl;
}

string getCurrentTimestamp()
{
    // Create the timestamp
    time_t timestamp = time(NULL);

    // Convert the timestamp to a string
    string timestampString = string(ctime(&timestamp));

    // Ensure that the string does not contain the newline character created by "ctime()"
    timestampString.pop_back();

    return timestampString;
}

vector<tuple<int, int>> calculateAirspacePositions(vector<tuple<int, int>> currentPositions, vector<array<string>> newAircrafts, int arraySize)
{
    for (int i = 0; i < newAircrafts.size(); i++)
    {
        // Read the x-axis and y-ais position from the regular aircraft data.
        array<string, arraySize> currentAircraft = newAircrafts[i];
        float xPosition = (float)currentAircraft[1]; // x-axis position of the aircraft.
        float yPosition = (float)currentAircraft[2]; // y-axis position of the aircraft.

        // Calculate the aircraft's current position in the airspace grid.
        tuple<int, int> airspaceGridPosition;

        // Calculate the position of the aircraft on the visual grid along the x-axis.
        int posX = round(x / 1000);

        // Calculate the position of the aircraft on the visual grid along the y-axis.
        int posY = round(y / 2000);

        airspaceGridPosition = make_tuple(posX, posY);

        currentPositions.push_back(calculateAirspacePosition(xPosition, yPosition));
    }

    return currentPositions;
}

void drawAirspace(vector<array<string, 5>> regularAircrafts, vector<array<string, 8>> augmentedAircrafts, vector<tuple<int, int>> gridPositions)
{
    // Combine the regular and augmented aircraft data.
    vector<array<string>> aircrafts = regularAircrafts;
    if (augmentedAircrafts.size() != 0)
    {
        for (int i = 0; i < augmentedAircrafts.size(); i++)
        {
            aircrafts.push_back(augmentedAircrafts[i]);
        }
    }

    // Draw the airspace grid.
    cout << "y [ft] \n"
         << endl; // Print the y-axis label.

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < columns; j++)
        {
            bool aircraftPresent;
            string currentAircraft;
            // Iterate through the vector to check if the aircraft is present.
            for (int k = 0; k < gridPositions.size(); k++)
            {
                if (gridPositions[k] == make_tuple(i, j))
                {
                    aircraftPresent = true;         // An aircraft is present.
                    currentAircraft = aircrafts[k]; // Store the current aircraft data temporarily.
                    break;
                }
            }

            // Check if one of the grid positions matches the current position.  Otherwise, leave it blank.
            if (aircraftPresent)
            {
                if (currentAircraft.end() == "1")
                {
                    cout << "X"; // Aircraft in violation.
                }
                else
                {
                    cout << "A"; // Aircraft not in violation.
                }
            }
            else
            {
                cout << "·"; // No aircraft in this part of the airspace.
            }
        }
        if (i == rows - 1)
        {
            cout << " x [ft]" << endl; // Print the x-axis label.
        }
        else
        {
            cout << endl; // Move to the next row.
        }
    }
}

void *aircraftDataHandling(void *arg)
{
    return nullptr;
}

void *violationHandling(void *arg)
{
    return nullptr;
}

void *terminationHandling(void *arg)
{
    return nullptr;
}