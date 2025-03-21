#include <iostream>
#include <string>
#include <fstream>
#include <ctime>

using namespace std;

// Function Prototypes
void insertBanner(string title);
void speedChangeRequest(fstream &f);
void altitudeChangeRequest(fstream &f);
void positionChangeRequest(fstream &f);
void augmentedInformationRequest(fstream &f);
bool terminateSystem();
string getCurrentTimestamp();

int main()
{
    /* TODO:
        1. Store requests/commands in a log file (Writable Shared Memory).
        2. Define the possible commands available to the operator:
            - Request a change in an aricraft's speed.
            - Request a change in an aircraft's altitude.
            - Request a change in an aircraft's position.
            - Request augmented information about a specific aircraft.
        3. Implement a termination signal to send to the Launcher.cpp.
        4. Implement any try-catch statements for error handling.
    */

    // Initial greeting message.
    cout << "Welcome to the Operator Subsystem!" << endl
         << "Information regarding aircrafts will appear in the visual display..." << endl;

    // Open "Logs.txt" to keep track of the operator's commands.
    fstream logs("Logs.txt");

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
                 << "[2] Request a change in an aircraft's altitude." << endl
                 << "[3] Request a change in an aircraft's position." << endl
                 << "[4] Request augmented information about a specific aircraft." << endl
                 << "[5] Go back to the main menu." << endl
                 << "Enter the number of your choice: ";
            // Take the operator's secondary selection as input.
            cin >> secondarySelection;

            switch (secondarySelection)
            {
            case 1:
                insertBanner("[1] Speed Change Request");
                speedChangeRequest(logs);
                break;
            case 2:
                insertBanner("[2] Altitude Change Request");
                altitudeChangeRequest(logs);
                break;
            case 3:
                insertBanner("[3] Position Change Request");
                positionChangeRequest(logs);
                break;
            case 4:
                insertBanner("[4] Augmented Information Request");
                augmentedInformationRequest(logs);
                break;
            case 5:
                break;
            default:
                cout << "Invalid input. Please enter a number between 1 and 5." << endl;
                break;
            }
        }
        else if (primarySelection == 2)
        { // The operator would like to terminate the system.
            insertBanner("System Termination");
            if (terminateSystem())
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
         << "=================================================" << endl
         << title << endl
         << "================================================="
         << endl;
}

// Request an aricraft to change its speed.
void speedChangeRequest(fstream &f)
{
    // Have the Operator enter the ID of the desired aricraft.
    int aircraftID = 0;
    cout << "Which aircraft should change its speed? ";
    cin >> aircraftID;

    // Have the Operator enter the new speed of the desired aricraft.
    int newSpeedX = 0;
    int newSpeedY = 0;
    int newSpeedZ = 0;
    cout << "What should the new speed of aircraft " << aircraftID << " be?" << endl
         << "Along the x-axis (SpeedX): ";
    cin >> newSpeedX;
    cout << "Along the y-axis (SpeedY): ";
    cin >> newSpeedY;
    cout << "Along the z-axis (SpeedZ): ";
    cin >> newSpeedZ;

    // Log the speed change request command in "Logs.txt"
    f << getCurrentTimestamp() << " " << "Speed_Change" << " " << aircraftID << " " << newSpeedX << " " << newSpeedY << " " << newSpeedZ << "\n";
    cout << "The speed change request has been logged..." << endl;
}

// Request an aricraft to change its altitude (Along the z-axis).
void altitudeChangeRequest(fstream &f)
{
    // Have the Operator enter the ID of the desired aricraft.
    int aircraftID = 0;
    cout << "Which aircraft should change its speed? ";
    cin >> aircraftID;

    // Have the Operator enter the new altitude of the desired aricraft.
    int altitude = 0;
    cout << "What should the new altitude of aircraft " << aircraftID << " be?" << endl
         << "Altitude (Position along the z-axis): ";
    cin >> altitude;

    // Log the speed change request command in "Logs.txt"
    f << getCurrentTimestamp() << " " << "Altitude_Change" << " " << aircraftID << " " << altitude << "\n";
    cout << "The altitude change request has been logged..." << endl;
}

// Request an aricraft to change its position (Along the x-axis and y-axis).
void positionChangeRequest(fstream &f)
{
    // Have the Operator enter the ID of the desired aricraft.
    int aircraftID = 0;
    cout << "Which aircraft should change its speed? ";
    cin >> aircraftID;

    // Have the Operator enter the new position of the desired aricraft.
    int newPositionX = 0;
    int newPositionY = 0;
    cout << "What should the new position of aircraft " << aircraftID << " be?" << endl
         << "Along the x-axis (PositionX): ";
    cin >> newPositionX;
    cout << "Along the y-axis (PositionY): ";
    cin >> newPositionY;

    // Log the position change request command in "Logs.txt"
    f << getCurrentTimestamp() << " " << "Position_Change" << " " << aircraftID << " " << newPositionX << " " << newPositionY << "\n";
    cout << "The position change request has been logged..." << endl;
}

// Request augmented information about an aricraft.
void augmentedInformationRequest(fstream &f)
{
    // Have the Operator enter the ID of the desired aricraft.
    int aircraftID = 0;
    cout << "Which aircraft should change its speed? ";
    cin >> aircraftID;

    // Log the augmented information request command in "Logs.txt"
    f << getCurrentTimestamp() << " " << "Augmented_Information" << " " << aircraftID << "\n";
    cout << "The augmented information request has been logged..." << endl;
}

// Terminate the system.
bool terminateSystem()
{
    while (1)
    {
        char input = ' ';
        cout << "Are you sure you like to terminate the subsystem? [y/n]: ";
        cin >> input;
        if (input == 'y')
        {
            cout << "The Operator Subsystem is terminating..." << endl;
            /* TODO: Send a termination signal to the Launcher.cpp */
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