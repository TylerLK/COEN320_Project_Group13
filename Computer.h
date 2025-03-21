#include <iostream>
#include <tuple>
#include <cmath>
#include <string>
#include <chrono>
#include <vector>
#include <limits>
#include <queue>
#include <fstream>
#include <thread>      // Include thread library for multithreading
#include <mutex>       // Include mutex for thread synchronization
#include <atomic>      // Include atomic for termination flag
#include <semaphore.h> // Include semaphore for synchronization
#include <sys/mman.h>  // For mmap, munmap
#include <fcntl.h>     // For shm_open, O_CREAT, O_RDWR
#include <unistd.h>    // For ftruncate
#include <cstring>     // For memcpy
#include "Aircraft.h"  // Include the Aircraft header file
#include <sstream>

using namespace std;
using namespace std::chrono;

const char *AIRCRAFT_SEMAPHORE_NAME = "/aircraft_semaphore";
const char *ALERTS_SEMAPHORE_NAME = "/alerts_semaphore";

struct Alert
{
    double time;
    string message;

    bool operator<(const Alert &other) const
    {
        return time > other.time; // Higher priority for more imminent alerts
    }
};

class Computer
{
public:
    // Constructor
    Computer() : terminate(false)
    { // Initialize the termination flag
        cout << "Computer object created." << endl;
    }

    // Main function for the Computer class
    void run()
    {
        // Initialize semaphores
        aircraftSemaphore = sem_open(AIRCRAFT_SEMAPHORE_NAME, O_CREAT, 0666, 1);
        if (aircraftSemaphore == SEM_FAILED)
        {
            perror("Error creating aircraft semaphore");
            return;
        }

        alertsSemaphore = sem_open(ALERTS_SEMAPHORE_NAME, O_CREAT, 0666, 1);
        if (alertsSemaphore == SEM_FAILED)
        {
            perror("Error creating alerts semaphore");
            sem_close(aircraftSemaphore);
            sem_unlink(AIRCRAFT_SEMAPHORE_NAME);
            return;
        }

        // Start threads for different tasks
        thread monitoringThread(&Computer::monitorAircrafts, this);
        thread separationThread(&Computer::checkSeparation, this);
        thread alertThread(&Computer::displayAlerts, this);
        thread aircraftLogThread(&Computer::writeAircraftDataToSharedMemoryAndLog, this, "AircraftData");
        thread alertsThread(&Computer::writeAlertsToSharedMemory, this, "AlertsData");

        // Wait for threads to finish
        monitoringThread.join();
        separationThread.join();
        alertThread.join();
        aircraftLogThread.join();
        alertsThread.join();

        // Cleanup semaphores
        sem_close(aircraftSemaphore);
        sem_unlink(AIRCRAFT_SEMAPHORE_NAME);

        sem_close(alertsSemaphore);
        sem_unlink(ALERTS_SEMAPHORE_NAME);
    }

private:
    vector<Aircraft> aircrafts = {
        Aircraft("12:00", "A1", 1000, 2005, 3000, 100, 100, 0),
        Aircraft("12:00", "A2", 4000, 5500, 3500, -100, -100, 0),
        Aircraft("12:00", "A3", 5000, 5000, 3000, 5, 5, 0),
        Aircraft("12:00", "A4", 1500, 2000, 3000, -5, -5, 0),
        Aircraft("12:00", "A5", 7000, 4000, 5000, 15, 15, 0),
        Aircraft("12:00", "A6", 8000, 4500, 5500, -15, -15, 0),
        Aircraft("12:00", "A7", 9000, 5000, 6000, 20, 20, 0),
        Aircraft("12:00", "A8", 10000, 5500, 6500, -20, -20, 0),
        Aircraft("12:00", "A9", 11000, 6000, 7000, 25, 25, 0),
        Aircraft("12:00", "A10", 12000, 6500, 7500, -25, -25, 0)};

    priority_queue<Alert> alerts;
    mutex alertMutex;         // Mutex to protect access to the alerts queue
    atomic<bool> terminate;   // Termination flag
    sem_t *aircraftSemaphore; // Semaphore for aircraft shared memory
    sem_t *alertsSemaphore;   // Semaphore for alerts shared memory

    // Function to monitor aircraft and generate alerts
    void monitorAircrafts()
    {
        while (!terminate)
        {
            auto totalStart = high_resolution_clock::now();

            for (size_t i = 0; i < aircrafts.size(); ++i)
            {
                for (size_t j = i + 1; j < aircrafts.size(); ++j)
                {
                    Aircraft a1 = aircrafts[i];
                    Aircraft a2 = aircrafts[j];
                    checkAircraftPair(a1, a2);
                }
            }

            auto totalEnd = high_resolution_clock::now();
            auto totalDuration = duration_cast<microseconds>(totalEnd - totalStart).count();
            cout << "Total time taken for the monitoring algorithm: " << totalDuration << " microseconds" << endl;

            this_thread::sleep_for(chrono::seconds(5));
        }
    }

    // Function to check separation between aircraft
    void checkSeparation()
    {
        while (!terminate)
        {
            this_thread::sleep_for(chrono::seconds(5));

            for (size_t i = 0; i < aircrafts.size(); ++i)
            {
                for (size_t j = i + 1; j < aircrafts.size(); ++j)
                {
                    Aircraft a1 = aircrafts[i];
                    Aircraft a2 = aircrafts[j];

                    if (violationCheck(&a1, &a2))
                    {
                        lock_guard<mutex> lock(alertMutex);
                        alerts.push({0, "Separation violation detected between " + a1.getAircraftID() + " and " + a2.getAircraftID()});
                    }
                }
            }
        }
    }

    // Function to display alerts
    void displayAlerts()
    {
        while (!terminate)
        {
            this_thread::sleep_for(chrono::seconds(1));

            lock_guard<mutex> lock(alertMutex);
            while (!alerts.empty())
            {
                Alert alert = alerts.top();
                alerts.pop();
                sendAlert(alert.message);
            }
        }
    }

    void sendAlert(const string &message)
    {
        cout << "ALERT: " << message << endl;
    }

    void checkAircraftPair(Aircraft a1, Aircraft a2)
    {
        bool isViolation = violationCheck(&a1, &a2);

        auto start = high_resolution_clock::now();
        auto willCollide = collisionCheck(&a1, &a2);
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start).count();

        bool willViolate = violationCheck(&a1, &a2);

        lock_guard<mutex> lock(alertMutex);
        if (isViolation)
        {
            alerts.push({0, "Violation currently happening between " + a1.getAircraftID() + " and " + a2.getAircraftID() + "."});
        }
        else if (get<0>(willCollide))
        {
            double collisionTime = get<1>(willCollide);
            alerts.push({collisionTime, "Collision will occur in " + to_string(collisionTime) + " seconds between " + a1.getAircraftID() + " and " + a2.getAircraftID() + "."});
        }
        else if (willViolate)
        {
            alerts.push({120, "Violation will occur within the next 2 minutes between " + a1.getAircraftID() + " and " + a2.getAircraftID() + "."});
        }
    }

    bool violationCheck(Aircraft *a1, Aircraft *a2)
    {
        double dx = abs(a1->getPositionX() - a2->getPositionX());
        double dy = abs(a1->getPositionY() - a2->getPositionY());
        double dz = abs(a1->getPositionZ() - a2->getPositionZ());

        return ((dx < 3000 || dy < 3000) && dz < 1000);
    }

    tuple<bool, double> collisionCheck(Aircraft *a1, Aircraft *a2)
    {
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

        if (!horizontalValid || !verticalValid)
        {
            return make_tuple(false, -1.0);
        }

        double t_first_alert = max(t_h_first, t_v_first);

        if (t_first_alert <= maxTime)
        {
            return make_tuple(true, t_first_alert);
        }

        return make_tuple(false, -1.0);
    }

    bool solveQuadraticFirstTime(double A, double B, double C, double R, double &t_first)
    {
        C -= R * R;

        double discriminant = B * B - 4 * A * C;
        if (discriminant < 0)
            return false;

        double sqrtD = sqrt(discriminant);
        double t1 = (-B - sqrtD) / (2 * A);
        double t2 = (-B + sqrtD) / (2 * A);

        if (t1 >= 0)
            t_first = t1;
        else if (t2 >= 0)
            t_first = t2;
        else
            return false;

        return true;
    }

    bool solveLinearFirstTime(double dz0, double dvz, double R, double &t_first)
    {
        if (dvz == 0)
        {
            if (abs(dz0) <= R)
            {
                t_first = 0;
                return true;
            }
            else
            {
                return false;
            }
        }

        double t1 = (R - dz0) / dvz;
        double t2 = (-R - dz0) / dvz;

        if (t1 > t2)
            swap(t1, t2);

        if (t1 >= 0)
            t_first = t1;
        else if (t2 >= 0)
            t_first = t2;
        else
            return false;

        return true;
    }

    void writeAircraftDataToSharedMemoryAndLog(const string &sharedMemoryName)
    {
        sem_t *semaphore = sem_open("/aircraft_semaphore", O_CREAT, 0666, 1); // Open the semaphore
        if (semaphore == SEM_FAILED)
        {
            perror("Error opening aircraft semaphore");
            return;
        }

        int version = 0; // Initialize version number

        while (!terminate)
        {
            this_thread::sleep_for(chrono::seconds(20)); // Sleep for 20 seconds

            // Lock the semaphore
            sem_wait(semaphore);

            // Create or open a shared memory object
            int shm_fd = shm_open(sharedMemoryName.c_str(), O_CREAT | O_RDWR, 0666);
            if (shm_fd == -1)
            {
                perror("Error creating shared memory");
                sem_post(semaphore); // Unlock the semaphore
                return;
            }

            // Calculate the size of the shared memory
            size_t shm_size = 1024; // Base size
            for (const auto &aircraft : aircrafts)
            {
                shm_size += 150; // Estimate size for each aircraft's data
            }

            // Set the size of the shared memory object
            if (ftruncate(shm_fd, shm_size) == -1)
            {
                perror("Error setting shared memory size");
                close(shm_fd);
                sem_post(semaphore); // Unlock the semaphore
                return;
            }

            // Map the shared memory object into the process's address space
            void *shm_ptr = mmap(0, shm_size, PROT_WRITE, MAP_SHARED, shm_fd, 0);
            if (shm_ptr == MAP_FAILED)
            {
                perror("Error mapping shared memory");
                close(shm_fd);
                sem_post(semaphore); // Unlock the semaphore
                return;
            }

            // Clear the shared memory
            memset(shm_ptr, 0, shm_size);

            // Write the version number
            char *mem = static_cast<char *>(shm_ptr);
            memcpy(mem, &version, sizeof(version));
            mem += sizeof(version);

            // Write the aircraft data to the shared memory
            for (const auto &aircraft : aircrafts)
            {
                auto now = system_clock::to_time_t(system_clock::now());
                string timestamp = ctime(&now);
                timestamp.pop_back(); // Remove trailing newline

                string data = "Timestamp: " + timestamp +
                              ", Aircraft ID: " + aircraft.getAircraftID() +
                              ", Position: (" + to_string(aircraft.getPositionX()) + ", " +
                              to_string(aircraft.getPositionY()) + ", " +
                              to_string(aircraft.getPositionZ()) + ")\n";

                memcpy(mem, data.c_str(), data.size());
                mem += data.size();
            }

            cout << "Aircraft data written to shared memory and log file: " << sharedMemoryName << endl;

            // Increment the version number
            version++;

            // Clean up
            munmap(shm_ptr, shm_size); // Unmap the shared memory
            close(shm_fd);             // Close the shared memory file descriptor

            // Unlock the semaphore
            sem_post(semaphore);
        }

        sem_close(semaphore); // Close the semaphore
    }

    void writeAlertsToSharedMemory(const string &sharedMemoryName)
    {
        sem_t *semaphore = sem_open("/alerts_semaphore", O_CREAT, 0666, 1); // Open the semaphore
        if (semaphore == SEM_FAILED)
        {
            perror("Error opening alerts semaphore");
            return;
        }

        int version = 0; // Initialize version number

        while (!terminate)
        {
            this_thread::sleep_for(chrono::seconds(5)); // Sleep for 5 seconds

            // Lock the semaphore
            sem_wait(semaphore);

            // Create or open a shared memory object
            int shm_fd = shm_open(sharedMemoryName.c_str(), O_CREAT | O_RDWR, 0666);
            if (shm_fd == -1)
            {
                perror("Error creating shared memory");
                sem_post(semaphore); // Unlock the semaphore
                return;
            }

            // Calculate the size of the shared memory
            size_t shm_size = 1024; // Base size
            {
                lock_guard<mutex> lock(alertMutex); // Lock to safely access alerts
                auto tempAlerts = alerts;           // Copy the priority queue
                while (!tempAlerts.empty())
                {
                    shm_size += 200; // Estimate size for each alert
                    tempAlerts.pop();
                }
            }

            // Set the size of the shared memory object
            if (ftruncate(shm_fd, shm_size) == -1)
            {
                perror("Error setting shared memory size");
                close(shm_fd);
                sem_post(semaphore); // Unlock the semaphore
                return;
            }

            // Map the shared memory object into the process's address space
            void *shm_ptr = mmap(0, shm_size, PROT_WRITE, MAP_SHARED, shm_fd, 0);
            if (shm_ptr == MAP_FAILED)
            {
                perror("Error mapping shared memory");
                close(shm_fd);
                sem_post(semaphore); // Unlock the semaphore
                return;
            }

            // Clear the shared memory
            memset(shm_ptr, 0, shm_size);

            // Write the version number
            char *mem = static_cast<char *>(shm_ptr);
            memcpy(mem, &version, sizeof(version));
            mem += sizeof(version);

            // Write the alerts to the shared memory
            {
                lock_guard<mutex> lock(alertMutex); // Lock to safely access alerts
                auto tempAlerts = alerts;
                while (!tempAlerts.empty())
                {
                    const auto &alert = tempAlerts.top();

                    // Format the alert with limited precision
                    ostringstream alertStream;
                    alertStream << "ALERT: Time: " << fixed << setprecision(2) << alert.time
                                << ", Message: " << alert.message << "\n";
                    string alertData = alertStream.str();

                    memcpy(mem, alertData.c_str(), alertData.size());
                    mem += alertData.size();

                    tempAlerts.pop();
                }
            }

            cout << "Alerts written to shared memory: " << sharedMemoryName << endl;

            // Increment the version number
            version++;

            // Clean up
            munmap(shm_ptr, shm_size); // Unmap the shared memory
            close(shm_fd);             // Close the shared memory file descriptor

            // Unlock the semaphore
            sem_post(semaphore);
        }

        sem_close(semaphore); // Close the semaphore
    }
};