#include "SensorThread.hpp"

// Parameters for the noise in the data
const double MEAN = 0.0;
const double STDDEV = 0.1;

SensorThread::SensorThread(SharedData &aSharedData, EnvClient &aClient, TaskWatch &aTaskWatch)
    : mSharedData(aSharedData), mClient(aClient), mTaskWatch(aTaskWatch), mThread(&SensorThread::run, this)
{
}

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

LeaderValues calculateVelocityAndDistanceToLeader(std::vector<VehicleAttr> &aVectorVehicleAttr)
{
    double minDistanceToLeader = std::numeric_limits<double>::max();
    double velocityLeader;
    double egoYPos = aVectorVehicleAttr.at(0).mY;
    double egoXPos = aVectorVehicleAttr.at(0).mX;

    for (int i = 1; i < aVectorVehicleAttr.size(); i++) // i = 1 because I exclude the ego pos
    {
        auto actualVehicle = aVectorVehicleAttr.at(i);

        // If the vehicle isn't in my lane OR the presence is lower than 0.5, I SKIP this vehicle
        if ((std::round(actualVehicle.mY) != egoYPos) || (actualVehicle.mPresence < 0.5))
        {
            continue;
        }

        if (((actualVehicle.mX - egoXPos) < minDistanceToLeader))
        {
            minDistanceToLeader = actualVehicle.mX - egoXPos;
            velocityLeader = actualVehicle.mVx;
        }
    }

    LeaderValues retValue;
    if (minDistanceToLeader != std::numeric_limits<double>::max())
    {
        retValue.mDistanceToLeader = minDistanceToLeader;
        retValue.mSpeed = velocityLeader;
    }

    return retValue;
}

void SensorThread::run()
{
    sched_param sch;
    sch.sched_priority = 60;
    int result = pthread_setschedparam(mThread.native_handle(), SCHED_FIFO, &sch);
    if (result != 0)
    {
        throw std::runtime_error("Error scheduling parameter. " + std::string(strerror(result)));
    }

    while (mRunning.load())
    {
        // START of the function
        mTaskWatch.startCycle();

        double acceleration = mSharedData.mAcceleration.load();

        mClient.mMutex.lock();
        json resp = mClient.sendComandAndRecive({{"cmd", "step"}, {"accel", acceleration}});
        mClient.mMutex.unlock();

        // Fill the array with the Vehicles Attributes
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

        // Generate some noise in the values
        std::random_device rd;
        std::mt19937 generator(rd());
        std::normal_distribution<double> dist(MEAN, STDDEV);

        double egoSpeed = vVehicleAttr.at(0).mVx;
        LeaderValues leaderDistanceVelocity = calculateVelocityAndDistanceToLeader(vVehicleAttr);

        // Add noise
        if (leaderDistanceVelocity.mDistanceToLeader != 0)
        {
            leaderDistanceVelocity.mDistanceToLeader += dist(generator);
            leaderDistanceVelocity.mSpeed += dist(generator);
        }

        mSharedData.mMutexSim.lock();
        mSharedData.setAll(egoSpeed, resp["terminated"], resp["truncated"], leaderDistanceVelocity.mDistanceToLeader, leaderDistanceVelocity.mSpeed);
        mSharedData.incrementID();
        mSharedData.mMutexSim.unlock();

        // END of the function
        mTaskWatch.endCycle();
        mTaskWatch.sleep();
    }
}
