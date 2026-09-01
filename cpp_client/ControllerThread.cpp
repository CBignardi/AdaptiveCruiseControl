#include "ControllerThread.hpp"
#include <limits>

// Parameters
const double KP_CRUICE = 0.02;
const double KP1_GAP = 0.02;
const double KP2_GAP = 0.05;
const double MIN_GAP = 5.0;           // meters
const double TIME_GAP = 1.5;          // seconds
const double MAX_DECELERATION = -4.0; // m/s^2
const double MAX_ACELERATION = 2.0;   // m/s^2

const double FREQ = 10; // Hz

ControllerThread::ControllerThread(SharedData &aSharedData) : mSharedData(aSharedData), mThread(&ControllerThread::run, this)
{
}

void ControllerThread::close()
{
    mRunning.store(false);
}

void ControllerThread::start(double aVelocity)
{
    mVelocity = aVelocity;
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
    int lastId = -1; // a negative number, because the first id will be 0
    std::chrono::duration<double> period(1.0 / FREQ);

    while (mRunning.load())
    {
        LogData logData;

        auto startTime = std::chrono::high_resolution_clock::now();
        // START of the function

        // Save the values in local variables
        mSharedData.mMutexSim.lock();
        int actualId = mSharedData.mSimData.mId;
        double distanceToLeader = mSharedData.mSimData.mDistanceToLeader;
        double velocityLeader = mSharedData.mSimData.mVelocityLeader;
        double egoSpeed = mSharedData.mSimData.mEgoSpeed;
        mSharedData.mMutexSim.unlock();

        if (actualId > lastId)
        {
            lastId = actualId;

            double acceleration;

            bool hasLeader = (distanceToLeader > 0);
            double safteyGap = MIN_GAP + TIME_GAP * egoSpeed;

            std::string state;

            if (!hasLeader || distanceToLeader > (safteyGap * 1.5))
            { // Normal cruice control
                state = "C";

                acceleration = KP_CRUICE * (mVelocity - egoSpeed);
            }
            else
            { // Car in front, Gap cruice control
                state = "G";

                double gapError = distanceToLeader - safteyGap;
                double speedDiff = velocityLeader - egoSpeed;

                acceleration = KP1_GAP * gapError + KP2_GAP * speedDiff;

                if ((egoSpeed >= mVelocity) && (acceleration > 0))
                {
                    acceleration = 0;
                }
            }

            acceleration = std::clamp(acceleration, MAX_DECELERATION, MAX_ACELERATION);

            mSharedData.mAcceleration.store(acceleration);

            std::cout << std::fixed << std::setprecision(2) 
                      << "acc: " << acceleration << " V_ego: " << egoSpeed
                      << " V_lead: " << velocityLeader << " D_lead: " << distanceToLeader
                      << " SafteyGap: " << safteyGap << " state: " << state << std::endl;

            logData.setData(acceleration, egoSpeed, velocityLeader, distanceToLeader, safteyGap, state);
        }

        // END of the function
        auto endTime = std::chrono::high_resolution_clock::now();

        logData.setTime(startTime, endTime);
        logData.mFullLog = true;

        mSharedData.mQueueLog.pushElem(logData);

        auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);

        if (elapsed < period)
        {
            std::this_thread::sleep_for(period - elapsed);
        }
    }
}