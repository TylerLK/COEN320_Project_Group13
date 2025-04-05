#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>     // Used to create, read, and write to files.
#include <ctime>       // Used to create a timestamp.
#include <limits>      // Used to define the numeric limits.
#include <semaphore.h> // Used to define semaphores for inter-process synchronization.
#include <fcntl.h>     // Used to open shared memory.
#include <sys/mman.h>  // Used to map shared memory to an address space.
#include <sys/stat.h>  // Used to define file permissions.
#include <cstring>
#include <thread>

using namespace std;

// Global Variables
const int SHM_SIZE = 4096;

const char *SHARED_MEMORY_LOGS = "/shm_logs";   // Name for the shared memory used to store all operator commands.
const char *SEMAPHORE_LOGS = "/logs_semaphore"; // Name for the semaphore used to synchronize the Operator and the Computer.

const char *SHARED_MEMORY_TERMINATION = "/shm_term";   // Name for the shared memory used to terminate the system.
const char *SEMAPHORE_TERMINATION = "/term_semaphore"; // Name for the semaphore used to synchronize all processes for the termination of the RTOS.

// Function Prototypes
void insertBanner(string title);
string getCurrentTimestamp();
void speedChangeRequest(fstream &f, sem_t *sem_logs, void *ptr_logs);
void augmentedInformationRequest(fstream &f, sem_t *sem_logs, void *ptr_logs);
bool terminateSystem(fstream &f, void *ptr_logs, int fd_logs, void *ptr_term, int fd_term, sem_t *sem_logs, sem_t *sem_term);

int main()
{
    // Initial greeting message.
    cout << "Welcome to the Operator Subsystem!" << endl
         << "Information regarding aircrafts will appear in the visual display..." << endl;

    // Open "Logs.txt" to keep track of the operator's commands.
    fstream logs("Logs.txt");
    if (!logs.is_open())
    {
        cerr << "Logs.txt not found..." << endl;
        return -1;
    }

    // Open the Shared Memory Files that the Operator will write into.
    // Logs
    int shm_fd_logs = shm_open(SHARED_MEMORY_LOGS, O_CREAT | O_RDWR, 0666);
    if (shm_fd_logs == -1)
    {
        perror("shm_open() for logs failed"); // This will print the String argument with the errno value appended.
        exit(1);
    }

    // Resize the shared memory for the logs.
    int size_logs = ftruncate(shm_fd_logs, SHM_SIZE);
    if (size_logs == -1)
    {
        perror("ftruncate() resizing for logs failed"); // This will print the String argument with the errno value appended.
        return -1;
    }

    // Mapping the shared memory into the Operator's address space.
    void *shm_ptr_logs = mmap(0, SHM_SIZE, PROT_WRITE, MAP_SHARED, shm_fd_logs, 0);
    if (shm_ptr_logs == MAP_FAILED)
    {
        cerr << "Shared Memory Mapping for logs failed..." << endl;
        return -1;
    }

    // Termination
    int shm_fd_term = shm_open(SHARED_MEMORY_TERMINATION, O_CREAT | O_RDWR, 0666);
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
    void *shm_ptr_term = mmap(0, SHM_SIZE, PROT_WRITE, MAP_SHARED, shm_fd_term, 0);
    if (shm_ptr_term == MAP_FAILED)
    {
        cerr << "Shared Memory Mapping for termination failed..." << endl;
        return -1;
    }

    // Create the Semaphores that the Operator needs to synchronize with other processes in shared memory.
    sem_t *sem_logs = sem_open(SEMAPHORE_LOGS, O_CREAT, 0666, 1);
    if (sem_logs == SEM_FAILED)
    {
        perror("sem_open() for logs has failed");
        return -1;
    }

    sem_t *sem_term = sem_open(SEMAPHORE_TERMINATION, O_CREAT, 0666, 1);
    if (sem_term == SEM_FAILED)
    {
        perror("sem_open() for termination has failed");
        return -1;
    }

    // The algorithm for this subsystem should run until the operator terminates it.
    while (1)
    {
        // Prompt the operator to make the initial selection.
        int primarySelection = 0;
        insertBanner("Main Menu");
        cout << "Would you like to:" << endl
             << "[1] Make a request about an aircraft?" << endl
             << "[2] Terminate the system?" << endl
             << "Enter the number of your choice: ";
        // Take the operator's primary selection as input.
        cin >> primarySelection;

        if (primarySelection == 1)
        { // The operator would like to make a request about an aircraft.
            int secondarySelection = 0;
            insertBanner("Request Menu");
            cout << "You can make the following requests about an aircraft:" << endl
                 << "[1] Request a change in an aircraft's speed." << endl
                 << "[2] Request augmented information about a specific aircraft." << endl
                 << "[3] Go back to the main menu." << endl
                 << "Enter the number of your choice: ";
            // Take the operator's secondary selection as input.
            cin >> secondarySelection;

            switch (secondarySelection)
            {
            case 1:
                insertBanner("[1] Speed Change Request");
                speedChangeRequest(logs, sem_logs, shm_ptr_logs);
                break;
            case 2:
                insertBanner("[2] Augmented Information Request");
                augmentedInformationRequest(logs, sem_logs, shm_ptr_logs);
                break;
            case 3:
                break;
            default:
                cout << "Invalid input. Please enter a number between 1 and 3." << endl;
                break;
            }
        }
        else if (primarySelection == 2)
        { // The operator would like to terminate the system.
            insertBanner("System Termination");
            if (terminateSystem(logs, shm_ptr_logs, shm_fd_logs, shm_ptr_term, shm_fd_term, sem_logs, sem_term))
            {
                break;
            }
        }
        else
        {
            cout << "Invalid input. Please enter '1' or '2'." << endl;
        }
    }

    // Close the "Logs.txt" file.
    logs.close();

    cout << "The Operator Subsystem has been terminated..." << endl;

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

// Get the current timestamp.
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

// Request an aircraft to change its speed.
void speedChangeRequest(fstream &f, sem_t *sem_logs, void *ptr_logs)
{
    int aircraftID = 0;
    // Have the Operator enter the ID of the desired aircraft.
    cout << "Which aircraft's speed would you like to change? " << endl
         << "Enter the desired aircraft's ID (i.e., an 8-digit integer value between 00000000 and 99999999): ";
    while (1)
    {
        cin >> aircraftID;

        if ((!cin.fail()) && (aircraftID > 0) && (aircraftID < 99999999))
        {
            break;
        }
        else
        {
            cout << "Invalid input. Please enter a valid integer aircraft ID between 00000000 and 99999999." << endl;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }

    // Have the Operator enter the new speed of the desired aircraft.
    int newSpeedX = 0;
    int newSpeedY = 0;
    int newSpeedZ = 0;
    cout << "What should the new speed of aircraft " << aircraftID << " be?" << endl
         << "Along the x-axis (in ft/s): ";
    cin >> newSpeedX;
    cout << "Along the y-axis (in ft/s): ";
    cin >> newSpeedY;
    cout << "Along the z-axis (in ft/s): ";
    cin >> newSpeedZ;

    // Create the speed change request command
    string speedChange = getCurrentTimestamp() + " " + "Speed_Change" + " " + to_string(aircraftID) + " " + to_string(newSpeedX) + " " + to_string(newSpeedY) + " " + to_string(newSpeedZ) + "\n";

    // Log the speed change request command in "Logs.txt"
    f << speedChange;

    // Write the speed change request command into shared memory
    const char *command = speedChange.c_str();
    if (speedChange.size() < SHM_SIZE)
    {
        sem_wait(sem_logs); // Lock the semaphore to enter the critical section.
        strncpy((char *)ptr_logs, command, SHM_SIZE);
        sem_post(sem_logs); // Unlock the semaphore to exit the critical section.

        cout << "The speed change request has been logged..." << endl;
    }
    else
    {
        cerr << "Error: Log entry exceeds the allotted shared memory space." << endl;
    }
}

// Request augmented information about an aircraft.
void augmentedInformationRequest(fstream &f, sem_t *sem_logs, void *ptr_logs)
{
    int aircraftID = 0;
    // Have the Operator enter the ID of the desired aircraft.
    cout << "Which aircraft's augmented information would you like? " << endl
         << "Enter the desired aircraft's ID (i.e., an 8-digit integer value between 00000000 and 99999999): ";
    while (1)
    {
        cin >> aircraftID;

        if ((!cin.fail()) && (aircraftID > 0) && (aircraftID < 99999999))
        {
            break;
        }
        else
        {
            cout << "Invalid input. Please enter a valid integer aircraft ID between 00000000 and 99999999." << endl;
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }

    // Create the augmented information request command
    string augmentedInformation = getCurrentTimestamp() + " " + "Augmented_Information" + " " + to_string(aircraftID) + "\n";

    // Log the augmented information request command in "Logs.txt"
    f << augmentedInformation;

    // Write the augmented information request command into shared memory
    const char *command = augmentedInformation.c_str();
    if (augmentedInformation.size() < SHM_SIZE)
    {
        sem_wait(sem_logs); // Lock the semaphore to enter the critical section.
        strncpy((char *)ptr_logs, command, SHM_SIZE);
        sem_post(sem_logs); // Unlock the semaphore to exit the critical section.

        cout << "The augmented information request has been logged..." << endl;
    }
    else
    {
        cerr << "Error: Log entry exceeds the allotted shared memory space." << endl;
    }
}

// Terminate the system.
bool terminateSystem(fstream &f, void *ptr_logs, int fd_logs, void *ptr_term, int fd_term, sem_t *sem_logs, sem_t *sem_term)
{
    while (1)
    {
        char input = ' ';
        cout << "Are you sure you like to terminate the subsystem? [y/n]: ";
        cin >> input;
        if (input == 'y')
        {
            sem_wait(sem_logs); // The shared memory should block other processes while being cleaned up.

            // Log the final command by the Operator in "Logs.txt"
            // Create the system termination request command
            string terminateSystem = getCurrentTimestamp() + " " + "Terminate_System \n";

            // Log the system termination request command in "Logs.txt"
            f << terminateSystem;

            // Clean up the shared memory for the logs.
            // Unmaps the shared memory for the logs.
            if (munmap(ptr_logs, SHM_SIZE) == -1)
            {
                perror("munmap() for logs failed"); // This will print the String argument with the errno value appended.
                return false;
            }

            // Closes the shared memory file.
            if (close(fd_logs) == -1)
            {
                perror("close() for logs failed"); // This will print the String argument with the errno value appended.
                return false;
            }

            // Unlinks the shared memory for logs, since the system is terminating.
            if (shm_unlink(SHARED_MEMORY_LOGS) == -1)
            {
                perror("shm_unlink() for logs failed"); // This will print the String argument with the errno value appended.
                return false;
            }

            // Close the logs semaphore.
            if (sem_close(sem_logs) == -1)
            {
                perror("sem_close() for logs failed");
                return false;
            }

            // Unlink the termination semaphore from the RTOS.
            if (sem_unlink(SEMAPHORE_LOGS) == -1)
            {
                perror("sem_unlink() for logs has failed");
                return false;
            }

            sem_wait(sem_term); // The shared memory should block other processes while being cleaned up.
            // Write into the termination shared memory that the other subsystems should begin termination.
            string terminationSignal = "Termination \n";

            // Write the termination signal into shared memory
            const char *command = terminationSignal.c_str();
            if (terminationSignal.size() < SHM_SIZE)
            {
                strncpy((char *)ptr_term, command, SHM_SIZE);
                cout << "The termination signal has been sent to the other subsystems..." << endl;
            }
            else
            {
                cerr << "Error: Termination Signal entry exceeds the allotted shared memory space." << endl;
            }
            sem_post(sem_term);

            // Flags needed to determine if the Operator may finish its termination process.
            bool termSignalSent = false;
            bool computerTerminated = false;
            bool radarTerminated = false;
            bool communicationSystemTerminated = false;
            bool visualDisplayTerminated = false;
            vector<string> terminationSignals; // Temporarily holds all of the current termination signals in the shared memory.
            while (1)
            {
                // Check if all of the subsystems have terminated before proceeding to the next step of cleanup.
                sem_wait(sem_term);
                // Read all the contents from shared memory.
                char *terminationData = static_cast<char *>(ptr_term);

                // Turn the terminationData character buffer into a string, to ensure that it is null-terminated.
                string terminationDataString(terminationData);

                // Create a stringstream object from the shared memory string, so that it can be parsed into individual lines.
                stringstream terminationDataStringStream(terminationDataString);

                // Create a string representing a single line of the violation data.
                string terminationDataLine;

                // Read each line from the stringstream individually.
                while (getline(terminationDataStringStream, terminationDataLine))
                {
                    terminationSignals.push_back(terminationDataLine);
                }

                if (terminationSignals.size() == 5)
                {
                    // Check for the presence of each termination signal in the vector.
                    for (size_t i = 0; i < terminationSignals.size(); i++)
                    {
                        if (terminationSignals[i] == "Terminate")
                        {
                            termSignalSent = true; // The termination signal has been sent.
                        }
                        else if (terminationSignals[i] == "Computer")
                        {
                            computerTerminated = true;
                        }
                        else if (terminationSignals[i] == "Radar")
                        {
                            radarTerminated = true;
                        }
                        else if (terminationSignals[i] == "Communication System")
                        {
                            communicationSystemTerminated = true;
                        }
                        else if (terminationSignals[i] == "Visual Display")
                        {
                            visualDisplayTerminated = true;
                        }
                    }
                }
                sem_post(sem_term);

                if (termSignalSent && computerTerminated && radarTerminated && communicationSystemTerminated && visualDisplayTerminated)
                {
                    break; // All subsystems have terminated.
                }
                else
                {
                    terminationSignals.clear();
                    cout << "Waiting for all of the other subsystems to terminate..." << endl;
                    this_thread::sleep_for(chrono::seconds(5)); // Wait for 5 seconds before checking again.
                }
            }

            // Clean up the shared memory for the termination signal.
            // Unmaps the shared memory for the termination.
            if (munmap(ptr_term, SHM_SIZE) == -1)
            {
                perror("munmap() for termination failed"); // This will print the String argument with the errno value appended.
                return false;
            }

            // Closes the shared memory file.
            if (close(fd_term) == -1)
            {
                perror("close() for termination failed"); // This will print the String argument with the errno value appended.
                return false;
            }

            // Unlinks the shared memory for logs, since the system is terminating.
            if (shm_unlink(SHARED_MEMORY_TERMINATION) == -1)
            {
                perror("shm_unlink() for termination failed"); // This will print the String argument with the errno value appended.
                return false;
            }
            // Close the termination semaphore.
            if (sem_close(sem_term) == -1)
            {
                perror("sem_close() for termination failed");
                return false;
            }

            // Unlink the termination semaphore from the RTOS.
            if (sem_unlink(SEMAPHORE_TERMINATION) == -1)
            {
                perror("sem_unlink() for termination has failed");
                return false;
            }

            cout << "The Operator Subsystem has terminated..." << endl;
            return true;
        }
        else if (input == 'n')
        {
            cout << "The Operator Subsystem is still running..." << endl;
            return false;
        }
        else
        {
            cout << "Invalid input. Please enter 'y' or 'n'." << endl;
        }
    }
}