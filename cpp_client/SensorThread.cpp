#include "SensorThread.hpp"
#include <limits>
#include <vector>
#include <random>

SensorThread::SensorThread(SharedData &aSharedData, EnvClient &aClient) : mSharedData(aSharedData), mClient(aClient), mThread(&SensorThread::run, this)
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
        if ((std::round(actualVehicle.mY) != egoYPos) || (actualVehicle.mPresence < 0.5))
        {
            continue;
        }

        if (((actualVehicle.mX - egoXPos) < minDistanceToLeader))
        {

            minDistanceToLeader = actualVehicle.mX - egoXPos;
            velocityLeader = actualVehicle.mVx;

            // if (minDistanceToLeader < 0)
            // {
            //     // std::cout << "distToLeader negativa: " << minDistanceToLeader << " veicolo: " << i << " xPos: " << actualVehicle.mX << " egoPos: " << egoXPos << std::endl;
            // }
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
    sched_param sch;
    sch.sched_priority = 70;
    int result = pthread_setschedparam(mThread.native_handle(), SCHED_FIFO, &sch);
    if (result != 0)
    {
        throw std::runtime_error("Error scheduling parameter. " + std::string(strerror(result)));
    }

    std::cout << "ThreadSensor" << " is executing at priority " << sch.sched_priority << std::endl;

    while (mRunning.load())
    {
        mStartTime = std::chrono::high_resolution_clock::now();
        // START of the function

        mSharedData.mMutexSim.lock();
        double acceleration = mSharedData.mAcceletation;
        mSharedData.mMutexSim.unlock();

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
        std::normal_distribution<double> dist(0.0, 0.1);

        double egoSpeed = vVehicleAttr.at(0).mVx;
        // std::cout << "egoSpeed: " << egoSpeed << std::endl;
        LeaderDistanceVelocity leaderDistanceVelocity = calculateVelocityAndDistanceToLeader(vVehicleAttr);

        if (leaderDistanceVelocity.mDistanceToLeader != 0)
        {
            // leaderDistanceVelocity.mDistanceToLeader += dist(generator);
            // leaderDistanceVelocity.mVelocity += dist(generator);
        }

        mSharedData.mMutexSim.lock();

        mSharedData.setAll(egoSpeed, resp["terminated"], resp["truncated"], leaderDistanceVelocity.mDistanceToLeader, leaderDistanceVelocity.mVelocity);
        mSharedData.incrementID();

        mSharedData.mMutexSim.unlock();

        // END of the function
        mEndTime = std::chrono::high_resolution_clock::now();

        LogData logData;
        logData.setTime(mStartTime, mEndTime);
        logData.mFullLog = false;

        // std::cout << "sensor, prima pushback \n";

        mSharedData.mQueueLog.pushElem(logData);

        // std::cout << "sensor, dopo pushback \n";

        auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(mEndTime - mStartTime);

        if (elapsed < mPeriod)
        {
            std::this_thread::sleep_for(mPeriod - elapsed);
        }
    }
}
