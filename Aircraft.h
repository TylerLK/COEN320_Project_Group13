#include <string>

using namespace std;

/*NOTE: All the aircrafts will be implemented as PERIODIC TASKS.
    The aircraft will update its position every second.
    Aircraft responds to radar requests. */

class Aircraft
{
public:
    // Constructor
    Aircraft(string t, string id, double posX, double posY, double posZ, double sX, double sY, double sZ)
    {
        time = t;
        aircraftID = id;
        positionX = posX;
        positionY = posY;
        positionZ = posZ;
        speedX = sX;
        speedY = sY;
        speedZ = sZ;
    }

    // Getters and Setters
    string getTime()
    {
        return time;
    }

    void setTime(string t)
    {
        time = t;
    }

    string getAircraftID()
    {
        return aircraftID;
    }

    void setAircraftID(string id)
    {
        aircraftID = id;
    }

    double getPositionX()
    {
        return positionX;
    }

    void setPositionX(double posX)
    {
        positionX = posX;
    }

    double getPositionY()
    {
        return positionY;
    }

    void setPositionY(double posY)
    {
        positionY = posY;
    }

    double getPositionZ()
    {
        return positionZ;
    }

    void setPositionZ(double posZ)
    {
        positionZ = posZ;
    }

    double getSpeedX()
    {
        return speedX;
    }

    void setSpeedX(double sX)
    {
        speedX = sX;
    }

    double getSpeedY()
    {
        return speedY;
    }

    void setSpeedY(double sY)
    {
        speedY = sY;
    }

    double getSpeedZ()
    {
        return speedZ;
    }

    void setSpeedZ(double sZ)
    {
        speedZ = sZ;
    }

    // This method updates the position of the aircraft based on the speed of the aircraft in the different directions.
    void updatePosition(double timeElapsed)
    {
        positionX = positionX + (speedX * timeElapsed);
        positionY = positionY + (speedY * timeElapsed);
        positionZ = positionZ + (speedZ * timeElapsed);
    }

    // This method responds to radar requests.
    string radarRequestResponse()
    {
        return aircraftID + " " + to_string(this->getPositionX()) + " " + to_string(this->getPositionY()) + " " + to_string(this->getPositionZ()) + " " + to_string(this->getSpeedX()) + " " + to_string(this->getSpeedY()) + " " + to_string(this->getSpeedZ());
    }

private:
    // Attributes
    string time;       // The time at which the aircraft enters the airspace.
    string aircraftID; // The unique ID of the aircraft.
    double positionX;  // The position of the aircraft along the x-axis.
    double positionY;  // The position of the aircraft along the y-axis.
    double positionZ;  // The position of the aircraft along the z-axis.
    double speedX;     // The speed of the aircraft along the x-axis.
    double speedY;     // The speed of the aircraft along the y-axis.
    double speedZ;     // The speed of the aircraft along the z-axis.
};