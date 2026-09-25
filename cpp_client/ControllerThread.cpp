#include "ControllerThread.hpp"

// For the Dinamic Minimun Gap
const double MIN_GAP = 2.0;           // meters
const double TIME_GAP = 1.6;          // seconds
const double MAX_DECELERATION = 1.67; // m/s^2 (must be positive number)
const double MAX_ACELERATION = 0.73;  // m/s^2

// For the value Filter
const double ALPHA_INPUT = 0.5; // 1 consider only new data, 0 consider only previous data

ControllerThread::ControllerThread(SharedData &aSharedData, Logger &aDataLogger, TaskWatch &aPeriodicTaskMonitor)
    : mSharedData(aSharedData), mDataLogger(aDataLogger), mTaskWatch(aPeriodicTaskMonitor), mThread(&ControllerThread::run, this)
{
}

void ControllerThread::close()
{
    mRunning.store(false);
}

void ControllerThread::start(double aVelocity)
{
    mSpeed = aVelocity;
    mRunning.store(true);
}

void ControllerThread::join()
{
    mThread.join();
}

void ControllerThread::run()
{
    sched_param sch;
    sch.sched_priority = 40;
    int result = pthread_setschedparam(mThread.native_handle(), SCHED_FIFO, &sch);
    if (result != 0)
    {
        throw std::runtime_error("Error scheduling parameter. " + std::string(strerror(result)));
    }

    // Variables
    size_t lastId = 0;
    double filteredDistanceToLeader = 0.0;
    double filteredVelocityLeader = 0.0;

    double lastDistanceLeader = 0.0;
    double lastVelocityLeader = 0.0;

    while (mRunning.load())
    {
        // START of the function
        mTaskWatch.startCycle();

        // Save the values in local variables
        mSharedData.mMutexSim.lock();
        int actualId = mSharedData.mSimData.mId;
        double distanceToLeaderRaw = mSharedData.mSimData.mDistanceToLeader;
        double velocityLeaderRaw = mSharedData.mSimData.mVelocityLeader;
        double egoSpeed = mSharedData.mSimData.mEgoSpeed;
        mSharedData.mMutexSim.unlock();

        if (actualId > lastId)
        {
            double acceleration;

            bool hasLeader = (distanceToLeaderRaw > 0);

            filteredDistanceToLeader = ALPHA_INPUT * distanceToLeaderRaw + (1 - ALPHA_INPUT) * lastDistanceLeader;
            filteredVelocityLeader = ALPHA_INPUT * velocityLeaderRaw + (1 - ALPHA_INPUT) * lastVelocityLeader;

            // Dinamic Minimum Gap
            double dinamicMinGap = MIN_GAP + egoSpeed * TIME_GAP + ((egoSpeed * (egoSpeed - filteredVelocityLeader)) / (2 * std::sqrt(MAX_ACELERATION * MAX_DECELERATION)));
            double distanceToUse;

            if (hasLeader)
            {
                distanceToUse = filteredDistanceToLeader;
            }
            else
            { // Since there is no leader and the distance is 0, we don't want to divide by 0. Therefore, we set the value to "infinity".
                distanceToUse = std::numeric_limits<double>::max();
            }

            // Model IDM
            acceleration = MAX_ACELERATION * (1 - std::pow(egoSpeed / mSpeed, 4) - std::pow(dinamicMinGap / distanceToUse, 2));

            mSharedData.mAcceleration.store(acceleration);

            mDataLogger.log({acceleration, egoSpeed, velocityLeaderRaw, distanceToLeaderRaw, dinamicMinGap, filteredVelocityLeader, filteredDistanceToLeader});
            mDataLogger.flush();
        }
        lastId = actualId;

        lastDistanceLeader = filteredDistanceToLeader;
        lastVelocityLeader = filteredVelocityLeader;

        // END of the function
        mTaskWatch.endCycle();
        mTaskWatch.sleep();
    }
}