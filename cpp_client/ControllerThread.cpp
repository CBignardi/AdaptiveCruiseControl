#include "ControllerThread.hpp"
#include <limits>

double KP_CRUICE = 0.02;
double KP1_GAP = 0.02;
double KP2_GAP = 0.05;
double MIN_GAP = 5.0;           // meters
double TIME_GAP = 1.5;          // seconds
double MAX_DECELERATION = -4.0; // m/s^2
double MAX_ACELERATION = 2.0;   // m/s^2

ControllerThread::ControllerThread(SharedData &aSharedData) : mSharedData(aSharedData), mThread(&ControllerThread::run, this)
{
    mFreq = 10; // Hz
    sched_param sch;
    sch.sched_priority = 40;
    int result = pthread_setschedparam(mThread.native_handle(), SCHED_FIFO, &sch);
    if (result != 0)
    {
        throw std::runtime_error("Error scheduling parameter. " + std::string(strerror(result)));
    }

    int policy;
    pthread_getschedparam(mThread.native_handle(), &policy, &sch);
    std::cout << "ThreadController" << " is executing at priority " << sch.sched_priority << std::endl;
}

void ControllerThread::close()
{
    mRunning.store(false);
}

void ControllerThread::start(double aDist, double aVelocity)
{
    mDist = aDist;
    mVelocity = aVelocity;
    mRunning.store(true);
}

void ControllerThread::join()
{
    mThread.join();
}

void ControllerThread::run()
{
    while (mRunning.load())
    {
        mStartTime = std::chrono::high_resolution_clock::now();
        // START of the function

        // Save the values in local variables
        mSharedData.mMutex.lock();
        size_t actualId = mSharedData.mId;
        double distanceToLeader = mSharedData.mDistanceToLeader;
        double velocityLeader = mSharedData.mVelocityLeader;
        double egoSpeed = mSharedData.mEgoSpeed;
        mSharedData.mMutex.unlock();

        if (actualId > mLastId)
        {
            mLastId = actualId;

            double acceleration;

            bool hasLeader = (distanceToLeader > 0);
            double safteyGap = MIN_GAP + TIME_GAP * egoSpeed;

            bool state = false;

            if (!hasLeader || distanceToLeader > (safteyGap * 1.5))
            { // Normal cruice control
                state = false;
                acceleration = KP_CRUICE * (mVelocity - egoSpeed);
            }
            else
            { // Car in front, Gap cruice control
                state = true;
                double gapError = distanceToLeader - safteyGap;
                double speedDiff = velocityLeader - egoSpeed;

                acceleration = KP1_GAP * gapError + KP2_GAP * speedDiff;

                if ((egoSpeed >= mVelocity) && (acceleration > 0))
                {
                    acceleration = 0;
                }
            }

            acceleration = std::clamp(acceleration, MAX_DECELERATION, MAX_ACELERATION);

            mSharedData.mMutex.lock();
            mSharedData.mAcceletation = acceleration;
            mSharedData.mMutex.unlock();

            mSharedData.os << "acceleration: " << acceleration << " egoSpeed: " << egoSpeed << " velocityLeader: " << velocityLeader << " mVelocity: " << mVelocity << " distanceToLeader: " << distanceToLeader << "  desiredGap: " << safteyGap << "  state: " << state << std::endl;
            mSharedData.os << std::endl;

            // std::cout << "egoSpeed: " << egoSpeed << " leaderSpeed: " << velocityLeader << " distToLeader: " << distanceToLeader << " desiredGap: " << safteyGap << " state: " << state << std::endl;
            // std::cout << std::endl;
        }

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