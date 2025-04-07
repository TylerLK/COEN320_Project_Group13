#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <cmath>       // Used for round();
#include <vector>      // Used to store aircraft data.
#include <array>       // Used to store aircraft data.
#include <tuple>       // Used to store grid positions.
#include <ctime>       // Used to create a timestamp.
#include <chrono>      // Used for a steady_clock
#include <semaphore.h> // Used to define semaphores for inter-process synchronization.
#include <fcntl.h>     // Used to open shared memory.
#include <sys/mman.h>  // Used to map shared memory to an address space.
#include <sys/stat.h>  // Used to define file permissions.
#include <pthread.h>   // Used to create pthreads.
#include <thread>      // For "this_thread::sleep_for()".
#include <atomic>      // Used to synchronize all threads for termination.
#include <unistd.h>    // Used to allow the threads to sleep; Used for alarm().

using namespace std;

// Global Variables
const int SHM_SIZE = 4096;
const int rows = 50;                    // Number of rows in the airspace grid.
const int columns = 100;                // Number of columns in the airspace grid.
bool augmentedAircraftsPresent = false; // Indicates if any augmented information was requested by the operator.
bool violationsPresent = false;         // Indicates if any violations have been detected.
atomic<bool> *terminateNow;             // Indicates if the subsystem should be terminating.

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
template <size_t arraySize> // Allows the following function to work with different-sized arrays.
vector<tuple<int, int>> calculateAirspacePositions(vector<tuple<int, int>> currentPositions, vector<array<string, arraySize>> newAircrafts);
void drawAirspace(vector<array<string, 5>> regularAircrafts, vector<array<string, 8>> augmentedAricrafts, vector<tuple<int, int>> gridPositions);
void *aircraftDataHandling(void *arg); // This function will be ran by the thread to print the regular visual display.
void *violationHandling(void *arg);    // This function will be ran by the thread to print the violations.
void *terminationHandling(void *arg);  // This function will be ran by the thread to terminate the system.

// Struct to pass arguments to threads
struct ThreadParameters
{
    // Shared Memory File Descriptors
    int shm_fd_reg;
    int shm_fd_aug;
    int shm_fd_viol;
    int shm_fd_term;

    // Shared Memory Pointers
    void *shm_ptr_reg;
    void *shm_ptr_aug;
    void *shm_ptr_viol;
    void *shm_ptr_term;

    // Semaphores
    sem_t *sem_data;
    sem_t *sem_term;

    // Termination Signal
    atomic<bool> *terminateNow;
};

int main()
{
    // Initial greeting message.
    cout << "Welcome to the Visual Display Subsystem!" << endl
         << "Information regarding aircrafts will appear below..." << endl;

    /* START SETUP*/
    terminateNow = new atomic<bool>(false);

    // Data
    // Regular Aircrafts
    int shm_fd_reg = shm_open(SHARED_MEMORY_AIRCRAFT_DATA, O_RDONLY, 0666);
    if (shm_fd_reg == -1)
    {
        perror("shm_open() for the regular aircrafts failed"); // This will print the String argument with the errno value appended.
        exit(1);
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
        perror("shm_open() for the augmented aircrafts failed"); // This will print the String argument with the errno value appended.
        exit(1);
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
        perror("shm_open() for the violations failed"); // This will print the String argument with the errno value appended.
        exit(1);
    }

    // Mapping the shared memory into the Visual Display's address space.
    void *shm_ptr_viol = mmap(0, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd_viol, 0);
    if (shm_ptr_viol == MAP_FAILED)
    {
        cerr << "Shared Memory Mapping for the violations failed..." << endl;
        return -1;
    }

    // Termination
    int shm_fd_term = shm_open(SHARED_MEMORY_TERMINATION, O_RDWR, 0666);
    if (shm_fd_term == -1)
    {
        perror("shm_open() for termination failed()");
        exit(1);
    }

    // Resize the shared memory for termination.
    int size_term = ftruncate(shm_fd_term, SHM_SIZE);
    if (size_term == -1)
    {
        perror("ftruncate() resizing for termination failed"); // This will print the String argument with the errno value appended.
        return -1;
    }

    // Mapping the shared memory into the Operator's address space.
    void *shm_ptr_term = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_term, 0);
    if (shm_ptr_term == MAP_FAILED)
    {
        cerr << "Shared Memory Mapping for termination failed..." << endl;
        return -1;
    }

    // Create the Semaphores that the Operator needs to synchronize with other processes in shared memory.
    sem_t *sem_data = sem_open(SEMAPHORE_DATA, 0);
    if (sem_data == SEM_FAILED)
    {
        perror("sem_open() for all data has failed");
        return -1;
    }

    sem_t *sem_term = sem_open(SEMAPHORE_TERMINATION, 0);
    if (sem_term == SEM_FAILED)
    {
        perror("sem_open() for termination has failed");
        return -1;
    }

    ThreadParameters parameters = {
        shm_fd_reg,
        shm_fd_aug,
        shm_fd_viol,
        shm_fd_term,
        shm_ptr_reg,
        shm_ptr_aug,
        shm_ptr_viol,
        shm_ptr_term,
        sem_data,
        sem_term,
        terminateNow};
    /* END SETUP*/

    // Create the threads that are needed to organize all the tasks in the Visual Display subsystem.
    // Data
    pthread_t thread_data; // Checks for regular and augmented aircrafts every 5 seconds and prints their location and information.
    if (pthread_create(&thread_data, nullptr, aircraftDataHandling, &parameters) != 0)
    {
        perror("pthread_create() for thread_data failed");
        return EXIT_FAILURE;
    }

    // Violations
    pthread_t thread_viol; // Checks for violations.
    if (pthread_create(&thread_viol, nullptr, violationHandling, &parameters) != 0)
    {
        perror("pthread_create() for thread_viol failed");
        return EXIT_FAILURE;
    }

    // Termination
    pthread_t thread_term; // Checks for termination signal from the Operator subsystem.
    if (pthread_create(&thread_term, nullptr, terminationHandling, &parameters) != 0)
    {
        perror("pthread_create() for thread_term failed");
        return EXIT_FAILURE;
    }

    pthread_join(thread_data, NULL);
    pthread_join(thread_viol, NULL);
    pthread_join(thread_term, NULL);

    /* START CLEANUP */
    // Close the data semaphore.
    if (sem_close(sem_data) == -1)
    {
        perror("sem_close() for all data failed");
        return false;
    }

    // Close the termination semaphore.
    if (sem_close(sem_term) == -1)
    {
        perror("sem_close() for termination failed");
        return false;
    }

    delete terminateNow;
    /* END CLEANUP */

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

template <size_t arraySize> // Allows the following function to work with different-sized arrays.
vector<tuple<int, int>> calculateAirspacePositions(vector<tuple<int, int>> currentPositions, vector<array<string, arraySize>> newAircrafts)
{
    for (size_t i = 0; i < newAircrafts.size(); i++)
    {
        // Read the x-axis and y-ais position from the regular aircraft data.
        array<string, arraySize> currentAircraft = newAircrafts[i];
        float xPosition = stof(currentAircraft[1]); // x-axis position of the aircraft.
        float yPosition = stof(currentAircraft[2]); // y-axis position of the aircraft.

        // Calculate the aircraft's current position in the airspace grid.
        tuple<int, int> airspaceGridPosition;

        // Calculate the position of the aircraft on the visual grid along the x-axis.
        int posX = round(xPosition / 1000);

        // Calculate the position of the aircraft on the visual grid along the y-axis.
        int posY = round(yPosition / 2000);

        airspaceGridPosition = make_tuple(posX, posY);

        currentPositions.push_back(airspaceGridPosition);
    }

    return currentPositions;
}

void drawAirspace(vector<array<string, 5>> regularAircrafts, vector<array<string, 8>> augmentedAircrafts, vector<tuple<int, int>> gridPositions)
{
    // Combine the regular and augmented aircraft data.
    vector<string> aircrafts;

    // Add each line of data regular aircraft data to the vector.
    for (size_t i = 0; i < regularAircrafts.size(); i++)
    {
        // Combine the aircraft data into a single string.
        string currentAircraft = regularAircrafts[i][0] + " " + regularAircrafts[i][1] + " " + regularAircrafts[i][2] + " " + regularAircrafts[i][3] + " " + regularAircrafts[i][4];

        // Add the string of data to the vector.
        aircrafts.push_back(currentAircraft);
    }

    // Add each line of augmented aircraft data to the vector.
    if (augmentedAircrafts.size() > 0)
    {
        for (size_t i = 0; i < augmentedAircrafts.size(); i++)
        {
            // Combine the aircraft data into a single string.
            string currentAircraft = augmentedAircrafts[i][0] + " " + augmentedAircrafts[i][1] + " " + augmentedAircrafts[i][2] + " " + augmentedAircrafts[i][3] + " " + augmentedAircrafts[i][4] + " " + augmentedAircrafts[i][5] + " " + augmentedAircrafts[i][6] + " " + augmentedAircrafts[i][7];

            // Add the string of data to the vector.
            aircrafts.push_back(currentAircraft);
        }
    }

    // Draw the airspace grid.
    cout << "y [ft] \n"
         << endl; // Print the y-axis label.

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < columns; j++)
        {
            bool aircraftPresent = false;
            string currentAircraft;
            // Iterate through the vector to check if the aircraft is present.
            for (size_t k = 0; k < gridPositions.size(); k++)
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
                if (currentAircraft.back() == '1')
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
    ThreadParameters *args = static_cast<ThreadParameters *>(arg);

    while (!*(args->terminateNow))
    {
        // Store the starting time of this task
        std::chrono::steady_clock::time_point startTime = steady_clock::now();

        // Clear all of the vectors
        regularAircraftData = {};
        augmentedAircraftData = {};
        aircraftGridPositions = {};

        sem_wait(args->sem_data); // The data thread locks the semaphore for all data.
        // Read the shared memory that holds the regular aircraft data.
        char *regularData = static_cast<char *>(args->shm_ptr_reg);

        // Turn the regularData character buffer into a string, to ensure that it is null-terminated.
        string regularDataString(regularData);

        // Create a stringstream object from the shared memory string, so that it can be parsed into individual lines.
        stringstream regularDataStringStream(regularDataString);

        // Create a string representing a single line of the regular aircraft data.
        string regularDataLine;

        // Read each line from the stringstream individually.
        while (getline(regularDataStringStream, regularDataLine))
        {
            // Create a temporary array of 5 strings that represents a de-constructed line of data.
            array<string, 5> regularDataLineDeconstructed;

            // Create a stringstream object from the current line of data that is being read.
            stringstream regularDataLineStringStream(regularDataLine);

            // Create a string representing a single aircraft variable.
            string regularDataVariable;

            // Add each variable to the string array.
            for (size_t i = 0; i < regularDataLineDeconstructed.size(); i++)
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

        // Read the shared memory that holds the augmented aircraft data.
        char *augmentedData = static_cast<char *>(args->shm_ptr_aug);

        // Turn the augmentedData character buffer into a string, to ensure that it is null-terminated.
        string augmentedDataString(augmentedData);

        // Create a stringstream object from the shared memory string, so that it can be parsed into individual lines.
        stringstream augmentedDataStringStream(augmentedDataString);

        // Create a string representing a single line of the augmented aircraft data.
        string augmentedDataLine;

        // Read each line from the stringstream individually.
        while (getline(augmentedDataStringStream, augmentedDataLine))
        {
            // Create a temporary array of 5 strings that represents a de-constructed line of data.
            array<string, 8> augmentedDataLineDeconstructed;

            // Create a stringstream object from the current line of data that is being read.
            stringstream augmentedDataLineStringStream(augmentedDataLine);

            // Create a string representing a single aircraft variable.
            string augmentedDataVariable;

            // Add each variable to the string array.
            for (size_t i = 0; i < augmentedDataLineDeconstructed.size(); i++)
            {
                if (getline(augmentedDataLineStringStream, augmentedDataVariable, ' '))
                {
                    augmentedDataLineDeconstructed[i] = augmentedDataVariable;
                }
                else
                {
                    cerr << "Reading the data has failed: " << augmentedDataVariable << endl;
                    break;
                }
            }

            // Append the line of regular aircraft data to its associated vector
            augmentedAircraftData.push_back(augmentedDataLineDeconstructed);
        }

        // Calculate the position of each augmented aircraft that will be displayed on the grid, granted any exist.
        if (augmentedAircraftData.size() > 0)
        {
            augmentedAircraftsPresent = true; // Augmented aircraft data is present in the system.

            aircraftGridPositions = calculateAirspacePositions(aircraftGridPositions, augmentedAircraftData);
        }
        sem_post(args->sem_data); // The data thread unlocks the semaphore for all data.

        // Beginning of the visual display
        insertBanner("Monitored En-Route Airspace" + getCurrentTimestamp());

        // Print the current state of the airspace.
        drawAirspace(regularAircraftData, augmentedAircraftData, aircraftGridPositions);

        // Print all regular aircraft data, line-by-line.
        insertBanner("Generic Aircraft Information");
        for (size_t i = 0; i < regularAircraftData.size(); i++)
        {
            // Create temporary object for each line of data.
            array<string, 5> currentAircraftData = regularAircraftData[i];

            // Print all part of the data in a line.
            for (size_t j = 0; j < currentAircraftData.size(); j++)
            {
                cout << currentAircraftData[j] << " ";
            }
        }

        // Print all augmented aircraft data, line-by-line.
        if (augmentedAircraftsPresent)
        { // Augmented aircraft data is present.
            insertBanner("Augmented Aircraft Information");
            for (size_t i = 0; i < augmentedAircraftData.size(); i++)
            {
                // Create temporary object for each line of data.
                array<string, 8> currentAircraftData = augmentedAircraftData[i];

                // Print all part of the data in a line.
                for (size_t j = 0; j < currentAircraftData.size(); j++)
                {
                    cout << currentAircraftData[j] << " ";
                }
            }
        }
        // End of the visual display.

        // Store the ending time of this task
        std::chrono::steady_clock::time_point endTime = steady_clock::now();

        // Calculate the execution time of this task
        std::chrono::duration<double> executionTime = duration_cast<duration<double>>(endTime - startTime);

        // Calculate the maximum allowable time for the task to sleep without missing its deadline
        double sleepTime = 5.0 - executionTime.count();

        // Make the thread sleep for its maximum allowable sleeping time.
        if (sleepTime >= 0.0)
        { // The thread can sleep for the remaining amount of its period.
            this_thread::sleep_for(chrono::duration<double>(sleepTime));
        }
        else
        { // The thread's execution time has exceeded its period.
            cerr << "Caution: The data handling thread has missed its deadline..." << endl;
        }
    }

    return nullptr;
}

void *violationHandling(void *arg)
{
    ThreadParameters *args = static_cast<ThreadParameters *>(arg);

    while (!*(args->terminateNow))
    {
        // Store the starting time of this task
        std::chrono::steady_clock::time_point startTime = steady_clock::now();

        // Clear the vectors holding the outdated violation data.
        violations = {};

        sem_wait(args->sem_data); // The violations thread locks the semaphore for all data.
        // Read the shared memory that holds the violation data
        char *violationData = static_cast<char *>(args->shm_ptr_viol);

        // Turn the violationData character buffer into a string, to ensure that it is null-terminated.
        string violationDataString(violationData);

        // Create a stringstream object from the shared memory string, so that it can be parsed into individual lines.
        stringstream violationDataStringStream(violationDataString);

        // Create a string representing a single line of the violation data.
        string violationDataLine;

        // Read each line from the stringstream individually.
        while (getline(violationDataStringStream, violationDataLine))
        {
            violations.push_back(violationDataLine);
        }
        sem_post(args->sem_data); // The violations thread unlocks the semaphore for all data.

        // Check if any violations are present in the airspace.
        if (violations.size() > 0)
        {
            insertBanner("Violations" + getCurrentTimestamp());

            // Print all of the violations present in the airspace, lin-by-line
            for (size_t i = 0; i < violations.size(); i++)
            {
                cout << violations[i] << endl;
            }

            // Emit a sonorous alarm
            cout << '\a';
        }

        // Store the ending time of this task
        std::chrono::steady_clock::time_point endTime = steady_clock::now();

        // Calculate the execution time of this task
        std::chrono::duration<double> executionTime = duration_cast<duration<double>>(endTime - startTime);

        // Calculate the maximum allowable time for the task to sleep without missing its deadline
        double sleepTime = 5.0 - executionTime.count();

        // Make the thread sleep for its maximum allowable sleeping time.
        if (sleepTime >= 0.0)
        { // The thread can sleep for the remaining amount of its period.
            this_thread::sleep_for(chrono::duration<double>(sleepTime));
        }
        else
        { // The thread's execution time has exceeded its period.
            cerr << "Caution: The violation handling thread has missed its deadline..." << endl;
        }
    }

    return nullptr;
}

void *terminationHandling(void *arg)
{
    ThreadParameters *args = static_cast<ThreadParameters *>(arg);

    while (!*(args->terminateNow))
    {
        sem_wait(args->sem_term); // The termination thread locks the semaphore for the termination signal.
        // Read the shared memory that holds the termination signal.
        char *terminationSignal = static_cast<char *>(args->shm_ptr_term);

        // Turn the termination signal character buffer into a string, to ensure that it is null-terminated.
        string terminationSignalString(terminationSignal);

        // Create a stringstream object from the shared memory string, so that it can be parsed into individual lines.
        stringstream terminationSignalStringStream(terminationSignalString);

        // Create a string representing a single line of the violation data.
        string terminationSignalLine;

        // Check if the first line in the shared memory contains the String "Terminate".
        if (getline(terminationSignalStringStream, terminationSignalLine))
        {
            if (terminationSignalLine == "Terminate")
            {
                *(args->terminateNow) = true; // The system should be terminating.

                // Write the subsystem name into the termination shared memory
                string terminationConfirmation = "Visual Display\n";

                const char *command = terminationConfirmation.c_str();

                if (terminationConfirmation.size() < SHM_SIZE)
                {
                    strncpy((char *)args->shm_ptr_term, command, SHM_SIZE); // Write the termination confirmation into shared memory.

                    cout << "The termination confirmation has been sent to the Operator subsystem..." << endl;
                }
                else
                {
                    cerr << "Error: The termination confirmation exceeded the allotted shared memory space." << endl;
                }
            }
        }

        // Make the thread sleep for 5 seconds.
        this_thread::sleep_for(chrono::seconds(5));
    }

    // Check if the subsystem should be terminating.
    if (*(args->terminateNow))
    {
        sem_wait(args->sem_data); // The shared memory should block other processes while being cleaned up.
        // Clean up the shared memory for the data.
        // Unmaps the shared memory for the regular aircrafts.
        if (munmap(args->shm_ptr_reg, SHM_SIZE) == -1)
        {
            perror("munmap() for the regular aircrafts failed"); // This will print the String argument with the errno value appended.
        }

        // Closes the shared memory file.
        if (close(args->shm_fd_reg) == -1)
        {
            perror("close() for regular aircrafts failed"); // This will print the String argument with the errno value appended.
        }

        // Unmaps the shared memory for the augmented aircrafts.
        if (munmap(args->shm_ptr_aug, SHM_SIZE) == -1)
        {
            perror("munmap() for the augmented aircrafts failed"); // This will print the String argument with the errno value appended.
        }

        // Closes the shared memory file.
        if (close(args->shm_fd_aug) == -1)
        {
            perror("close() for the augmented aircrafts failed"); // This will print the String argument with the errno value appended.
        }

        // Unmaps the shared memory for the violations.
        if (munmap(args->shm_ptr_viol, SHM_SIZE) == -1)
        {
            perror("munmap() for the violations failed"); // This will print the String argument with the errno value appended.
        }

        // Closes the shared memory file.
        if (close(args->shm_fd_viol) == -1)
        {
            perror("close() for the violations failed"); // This will print the String argument with the errno value appended.
        }
        sem_post(args->sem_data);

        // Clean up the shared memory for the termination signal.
        // Unmaps the shared memory for the termination.
        if (munmap(args->shm_ptr_term, SHM_SIZE) == -1)
        {
            perror("munmap() for termination failed"); // This will print the String argument with the errno value appended.
        }

        // Closes the shared memory file.
        if (close(args->shm_fd_term) == -1)
        {
            perror("close() for termination failed"); // This will print the String argument with the errno value appended.
        }
    }
    sem_post(args->sem_term); // The termination thread unlocks the semaphore for the termination signal.

    return nullptr;
}