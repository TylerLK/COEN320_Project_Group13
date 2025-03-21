#include <iostream>
#include <cstdlib>

using namespace std;

void killSubsystems();

int main()
{
    /* TODO:
        3. Compilation and execution of the other subsystems:
            - Adapt this to QNX OS.

    */

    // COMPILATION AND EXECUTION OF TERMINALS IN WINDOWS (ADAPT TO QNX OS)
    // try
    // {
    //     // Compile all of the subsystems
    //     int visualDisplaySystem = system("g++ -o VisualDisplay.exe VisualDisplay.cpp");
    //     int communicationSystem = system("g++ -o Communication.exe Communication.cpp");
    //     int computerSystem = system("g++ -o Computer.exe Computer.cpp");
    //     int radarSystem = system("g++ -o Radar.exe Radar.cpp");
    //     if ((visualDisplaySystem != 0) || (communicationSystem != 0) || (computerSystem != 0) || (radarSystem != 0))
    //     { // Null Pointer Exception
    //         throw runtime_error("The subsystmes failed to compile... Null Pointer Exception");
    //     }
    //     else
    //     {
    //         cout << "All subsystems have compiled compiled..." << endl; // Test Statement

    //         int runVisualDisplaySystem = system("start cmd /k VisualDisplay.exe"); // Execute Visual Display Subsystem
    //         if (runVisualDisplaySystem != 0)
    //         { // Null Pointer Exception
    //             throw runtime_error("VisualDisplay.exe failed to execute... Null Pointer Exception");
    //         }
    //         int runCommunicationSystem = system("start cmd /k Communication.exe"); // Execute Communication Subsystem
    //         if (runCommunicationSystem != 0)
    //         { // Null Pointer Exception
    //             throw runtime_error("Communication.exe failed to execute... Null Pointer Exception");
    //         }
    //         int runComputerSystem = system("start cmd /k Computer.exe"); // Execute Computer Subsystem
    //         if (runComputerSystem != 0)
    //         { // Null Pointer Exception
    //             throw runtime_error("Computer.exe failed to execute... Null Pointer Exception");
    //         }
    //         int runRadarSystem = system("start cmd /k Radar.exe"); // Execute Radar Subsystem
    //         if (runRadarSystem != 0)
    //         { // Null Pointer Exception
    //             throw runtime_error("Radar.exe failed to execute... Null Pointer Exception");
    //         }
    //     }
    // }
    // catch (const exception &e)
    // {
    //     cerr << "An error occurred: " << e.what() << endl;
    //     return -1;
    // }

    return 0;
}

// void killSubsystems()
// {
//     // Kill all of the subystem processes.
//     int killRadarSystem = system("taskkill /IM Radar.exe /F");
//     int killComputerSystem = system("taskkill /IM Computer.exe /F");
//     int killCommunicationSystem = system("taskkill /IM Communication.exe /F");
//     int killVisualDisplaySystem = system("taskkill /IM VisualDisplay.exe /F");
//     cout << "All of the subsystem processes have been terminated..." << endl;
// }