#include <iostream>
#include <cstdlib>

using namespace std;

int main()
{
    /* TODO:
        1. Store requests/commands in a log file (Writable Shared Memory).
        2. Define the possible commands available to the operator:
            - Request a change in the aricraft's speed.
            - Request a change in the aircraft's altitude.
            - Request a change in the aircraft's position.
            - Request augmented information about a specific aircraft
        3. Compilation and execution of the other subsystems.
    */

    cout << "This is the Operator Subsystem" << endl;

    // Compile Execute the other Subsystems
    // Visual Display Subsystem
    try
    {
        int visualDisplay = system("g++ -o VisualDisplay.exe VisualDisplay.cpp"); // Compile
        if (visualDisplay != 0)
        { // Null Pointer Exception
            throw runtime_error("VisualDisplay.cpp failed to compile... Null Pointer Exception");
        }
        else
        {
            cout << "VisualDisplay.cpp has compiled..." << endl; // Test Statement

            int runVisualDisplay = system("VisualDisplay.exe"); // Execute
            if (runVisualDisplay != 0)
            { // Null Pointer Exception
                throw runtime_error("VisualDisplay.exe failed to execute... Null Pointer Exception");
            }
            else
            {
                cout << "VisualDisplay.exe has executed..." << endl; // Test Statement
            }
        }
    }
    catch (const exception &e)
    {
        cerr << "An error occurred: " << e.what() << endl;
        return -1;
    }

    return 0;
}