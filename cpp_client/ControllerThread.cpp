#include "ControllerThread.hpp"
#include <limits>

// Parameters
double KP_CRUICE = 0.02;
double KP1_GAP = 0.02;
double KP2_GAP = 0.05;
double MIN_GAP = 5.0;           // meters
double TIME_GAP = 1.5;          // seconds
double MAX_DECELERATION = -4.0; // m/s^2
double MAX_ACELERATION = 2.0;   // m/s^2

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

    std::cout << "ThreadController" << " is executing at priority " << sch.sched_priority << std::endl;

    int lastId = -1; // a negative number, because the first id will be 0 

    while (mRunning.load())
    {
        LogData logData;

        mStartTime = std::chrono::high_resolution_clock::now();
        // START of the function

        // std::cout << "contrl, prima di copiamento variabili\n";

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
                state = "Cruise";

                acceleration = KP_CRUICE * (mVelocity - egoSpeed);
            }
            else
            { // Car in front, Gap cruice control
                state = "Gap";

                double gapError = distanceToLeader - safteyGap;
                double speedDiff = velocityLeader - egoSpeed;

                acceleration = KP1_GAP * gapError + KP2_GAP * speedDiff;

                if ((egoSpeed >= mVelocity) && (acceleration > 0))
                {
                    acceleration = 0;
                }
            }

            acceleration = std::clamp(acceleration, MAX_DECELERATION, MAX_ACELERATION);

            // std::cout << "control, prima lock acc\n";

            mSharedData.mMutexSim.lock();
            mSharedData.mAcceletation = acceleration;
            mSharedData.mMutexSim.unlock();
            // std::cout << "control, dopo lock acc\n";

            // std::cout << "contrl, acc: " << acceleration << " egoSpeed: " << egoSpeed << " velLeader: " << velocityLeader << "distLeader: " << distanceToLeader << " safteygay: " << safteyGap << " state: " << state << std::endl;

            logData.setData(acceleration, egoSpeed, velocityLeader, distanceToLeader, safteyGap, state);
        }

        // END of the function
        mEndTime = std::chrono::high_resolution_clock::now();

        logData.setTime(mStartTime, mEndTime);
        logData.mFullLog = true;

        // std::cout << "controller, prima pushback \n";

        mSharedData.mQueueLog.pushElem(logData);

        // std::cout << "controller, dopo pushback \n";

        auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(mEndTime - mStartTime);

        if (elapsed < mPeriod)
        {
            std::this_thread::sleep_for(mPeriod - elapsed);
        }
    }
}