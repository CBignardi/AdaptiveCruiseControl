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

    std::thread t(&SensorThread::run, this);
    mThread = t;
}

void SensorThread::join()
{
    mThread.join();
    std::thread t(&SensorThread::run, this);
}

struct VehicleAttr
{
public:
    double mPresence;
    double mX;
    double mY;
    double mVx;
    double mVy;
};

std::pair<double, double> calculateVelocityAndDistanceToLeader(std::vector<VehicleAttr> &aVectorVehicleAttr)
{
    double minDistanceToLeader = std::numeric_limits<double>::max();
    double velocityLeader;
    double yEgoPos = aVectorVehicleAttr.at(0).mY;

    for (int i = 1; i < aVectorVehicleAttr.size(); i++) // exclude ego pos
    {
        auto actualVehicle = aVectorVehicleAttr.at(i);

        if ((actualVehicle.mY == yEgoPos) && (actualVehicle.mX < minDistanceToLeader) && (actualVehicle.mPresence > 0.5))
        {
            minDistanceToLeader = actualVehicle.mX;
            velocityLeader = actualVehicle.mVx;
        }
    }
    if (minDistanceToLeader == std::numeric_limits<double>::max())
    {
        return std::pair<double, double>(0.0, 0.0);
    }
    return std::pair<double, double>(minDistanceToLeader, velocityLeader);
}

void SensorThread::run()
{
    while (mRunning.load())
    {
        mStartTime = std::chrono::high_resolution_clock::now();
        // START of the function

        mSharedData.mMutex.lock();

        json resp = mClient.sendComandAndRecive({{"cmd", "step"}, {"accel", mSharedData.mAcceletation}});

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

        double egoSpeed = vVehicleAttr.at(0).mVx + dist(generator);
        std::pair<double, double> distAndVelocityLeader = calculateVelocityAndDistanceToLeader(vVehicleAttr);

        if (distAndVelocityLeader.first != 0)
        {
            distAndVelocityLeader.first += dist(generator);
            distAndVelocityLeader.second += dist(generator);
        }

        mSharedData.setAll(egoSpeed, resp["terminated"], resp["truncated"], distAndVelocityLeader.first, distAndVelocityLeader.second);

        for (int i = 0; i < vVehicleAttr.size(); i++)
        {
            std::cout << "veicolo " << i << "-> v: " << vVehicleAttr.at(i).mVx << " distance: " << vVehicleAttr.at(i).mX << std::endl;
        }

        std::cout << std::endl;

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
