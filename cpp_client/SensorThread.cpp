#include "SensorThread.hpp"
#include <limits>
#include <vector>
#include <random>

void SensorThread::close()
{
    mRunning.store(false);
}

void SensorThread::start()
{
    mRunning.store(true);
}

void SensorThread::join()
{
    mThread.join();
}

struct LeaderDistanceVelocity
{
    double mDistanceToLeader = 0;
    double mVelocity = 0;
};

struct VehicleAttr
{
public:
    double mPresence;
    double mX;
    double mY;
    double mVx;
    double mVy;
};

LeaderDistanceVelocity calculateVelocityAndDistanceToLeader(std::vector<VehicleAttr> &aVectorVehicleAttr)
{
    double minDistanceToLeader = std::numeric_limits<double>::max();
    double velocityLeader;
    double egoYPos = aVectorVehicleAttr.at(0).mY;
    double egoXPos = aVectorVehicleAttr.at(0).mX;

    for (int i = 1; i < aVectorVehicleAttr.size(); i++) // exclude ego pos
    {
        auto actualVehicle = aVectorVehicleAttr.at(i);

        // if the vehicle isn't in my lane OR the presence is lower than 0.5, we SKIP this vehicle
        if ((std::round(actualVehicle.mY) != egoYPos) && (actualVehicle.mPresence < 0.5))
        {
            continue;
        }

        if (((actualVehicle.mX - egoXPos) < minDistanceToLeader))
        {
            minDistanceToLeader = actualVehicle.mX - egoXPos;
            velocityLeader = actualVehicle.mVx;
        }
    }

    LeaderDistanceVelocity retValue;
    if (minDistanceToLeader != std::numeric_limits<double>::max())
    {
        retValue.mDistanceToLeader = minDistanceToLeader;
        retValue.mVelocity = velocityLeader;
    }

    return retValue;
}

void SensorThread::run()
{
    while (mRunning.load())
    {
        mStartTime = std::chrono::high_resolution_clock::now();
        // START of the function

        mSharedData.mMutex.lock();
        double acceleration = mSharedData.mAcceletation;
        mSharedData.mMutex.unlock();

        json resp = mClient.sendComandAndRecive({{"cmd", "step"}, {"accel", acceleration}});

        // fill the array with the Vehicles Attributes
        std::vector<VehicleAttr> vVehicleAttr;
        VehicleAttr vehicleAttr;
        for (const auto row : resp["obs"])
        {
            vehicleAttr.mPresence = row[0];
            vehicleAttr.mX = row[1];
            vehicleAttr.mY = row[2];
            vehicleAttr.mVx = row[3];
            vehicleAttr.mVy = row[4];
            vVehicleAttr.push_back(vehicleAttr);
        }

        // Add noise to the value
        std::random_device rd;
        std::mt19937 generator(rd());
        std::normal_distribution<double> dist(0.0, 0.3);

        double egoSpeed = vVehicleAttr.at(0).mVx;
        LeaderDistanceVelocity leaderDistanceVelocity = calculateVelocityAndDistanceToLeader(vVehicleAttr);

        if (leaderDistanceVelocity.mDistanceToLeader != 0)
        {
            leaderDistanceVelocity.mDistanceToLeader += dist(generator);
            leaderDistanceVelocity.mVelocity += dist(generator);
        }

        mSharedData.mMutex.lock();

        mSharedData.setAll(egoSpeed, resp["terminated"], resp["truncated"], leaderDistanceVelocity.mDistanceToLeader, leaderDistanceVelocity.mVelocity);

        for (int i = 0; i < vVehicleAttr.size(); i++)
        {
            mSharedData.os << "veicolo " << i << "-> v: " << vVehicleAttr.at(i).mVx << " distance: " << vVehicleAttr.at(i).mX << " y: " << vVehicleAttr.at(i).mY << std::endl;
        }
        mSharedData.os << std::endl;

        mSharedData.mMutex.unlock();

        // END of the function
        mEndTime = std::chrono::high_resolution_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(mEndTime - mStartTime);
        std::chrono::duration<double> period(1.0 / mFreq);

        if (elapsed < period)
        {
            std::this_thread::sleep_for(period - elapsed);
        }
    }
}
