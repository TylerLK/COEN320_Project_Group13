#include <iostream>
#include <tuple>
#include <cmath>
#include <string>
#include <chrono>
#include <vector>
#include <limits>
#include <queue>
#include "Aircraft.h" // Include the Aircraft header file

using namespace std;
using namespace std::chrono;

bool violationCheck(Aircraft* a1, Aircraft* a2);
tuple<bool, double> collisionCheck(Aircraft* a1, Aircraft* a2);

void sendAlert(const string& message) {
    cout << "ALERT: " << message << endl;
}

struct Alert {
    double time;
    string message;

    bool operator<(const Alert& other) const {
        return time > other.time; // Higher priority for more imminent alerts
    }
};

void checkAircraftPair(Aircraft a1, Aircraft a2, priority_queue<Alert>& alerts) {
    bool isViolation = violationCheck(&a1, &a2);

    auto start = high_resolution_clock::now();
    auto willCollide = collisionCheck(&a1, &a2);
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start).count();

    bool willViolate = violationCheck(&a1, &a2);

    if (isViolation) {
        alerts.push({0, "Violation currently happening between " + a1.getAircraftID() + " and " + a2.getAircraftID() + "."});
    } else if (get<0>(willCollide)) {
        double collisionTime = get<1>(willCollide);
        alerts.push({collisionTime, "Collision will occur in " + to_string(collisionTime) + " seconds between " + a1.getAircraftID() + " and " + a2.getAircraftID() + "."});
    } else if (willViolate) {
        alerts.push({120, "Violation will occur within the next 2 minutes between " + a1.getAircraftID() + " and " + a2.getAircraftID() + "."});
    } else {
        cout << "No alerts between " + a1.getAircraftID() + " and " + a2.getAircraftID() + ". Continue monitoring." << endl;
    }

    //cout << "Time taken for collisionCheck between " + a1.getAircraftID() + " and " + a2.getAircraftID() + ": " << duration << " microseconds" << endl;
}

int main() {
    vector<Aircraft> aircrafts = {
        Aircraft("12:00", "A1", 1000, 2000, 3000, 100, 100, 0),
        Aircraft("12:00", "A2", 4000, 5500, 3500, -100, -100, 0),
        Aircraft("12:00", "A3", 5000, 5000, 3000, 5, 5, 0),
        Aircraft("12:00", "A4", 1500, 2000, 3000, -5, -5, 0),
        Aircraft("12:00", "A5", 7000, 4000, 5000, 15, 15, 0),
        Aircraft("12:00", "A6", 8000, 4500, 5500, -15, -15, 0),
        Aircraft("12:00", "A7", 9000, 5000, 6000, 20, 20, 0),
        Aircraft("12:00", "A8", 10000, 5500, 6500, -20, -20, 0),
        Aircraft("12:00", "A9", 11000, 6000, 7000, 25, 25, 0),
        Aircraft("12:00", "A10", 12000, 6500, 7500, -25, -25, 0)
    };

    auto totalStart = high_resolution_clock::now(); // Start timing the entire monitoring algorithm

    priority_queue<Alert> alerts;

    for (size_t i = 0; i < aircrafts.size(); ++i) {
        for (size_t j = i + 1; j < aircrafts.size(); ++j) {
            Aircraft a1 = aircrafts[i]; // Create a copy of aircraft i
            Aircraft a2 = aircrafts[j]; // Create a copy of aircraft j

            checkAircraftPair(a1, a2, alerts);
        }
    }

    while (!alerts.empty()) {
        Alert alert = alerts.top();
        alerts.pop();
        sendAlert(alert.message);
    }

    auto totalEnd = high_resolution_clock::now(); // End timing the entire monitoring algorithm
    auto totalDuration = duration_cast<microseconds>(totalEnd - totalStart).count();
    cout << "Total time taken for the entire monitoring algorithm: " << totalDuration << " microseconds" << endl;

    return 0;
}

bool violationCheck(Aircraft* a1, Aircraft* a2) {
    double dx = abs(a1->getPositionX() - a2->getPositionX());
    double dy = abs(a1->getPositionY() - a2->getPositionY());
    double dz = abs(a1->getPositionZ() - a2->getPositionZ());

    return ((dx < 3000 || dy < 3000) && dz < 1000);
}

bool solveQuadraticFirstTime(double A, double B, double C, double R, double& t_first) {
    C -= R * R; // Adjust for threshold squared

    double discriminant = B * B - 4 * A * C;
    if (discriminant < 0) return false; // No real solutions

    double sqrtD = sqrt(discriminant);
    double t1 = (-B - sqrtD) / (2 * A);
    double t2 = (-B + sqrtD) / (2 * A);

    // Choose the first positive t
    if (t1 >= 0) t_first = t1;
    else if (t2 >= 0) t_first = t2;
    else return false; // No positive solutions

    return true;
}

bool solveLinearFirstTime(double dz0, double dvz, double R, double& t_first) {
    if (dvz == 0) {
        // If there is no relative vertical velocity, check the initial distance
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

    // Choose the first positive t
    if (t1 >= 0) t_first = t1;
    else if (t2 >= 0) t_first = t2;
    else return false; // No positive solutions

    return true;
}

tuple<bool, double> collisionCheck(Aircraft* a1, Aircraft* a2) {
    const int maxTime = 120;  
    const double horizontalThreshold = 3000.0;  
    const double verticalThreshold = 1000.0;    

    // Compute quadratic coefficients for horizontal distance
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

    // Debug output
    // cout << "Horizontal valid: " << horizontalValid << ", t_h_first: " << t_h_first << endl;
    // cout << "Vertical valid: " << verticalValid << ", t_v_first: " << t_v_first << endl;

    if (!horizontalValid || !verticalValid) {
        return make_tuple(false, -1.0); // No collision within the next 2 minutes
    }

    // Find the first time both conditions are met
    double t_first_alert = max(t_h_first, t_v_first);

    if (t_first_alert <= maxTime) {
        return make_tuple(true, t_first_alert); // Return true and the time in seconds
    }

    return make_tuple(false, -1.0); // No collision within the next 2 minutes
}