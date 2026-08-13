#include "ControllerThread.hpp"

double Kp = 0.02;
double Kd = -0.35;
double Kp_velocity = 0.02;

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
            double acceleration = 0;

            // Rimuovi dopo aver tolto la stampa
            double safteyDistance;
            double distanceToUse;
            double diff;

            if (distanceToLeader > 0)
            { // Has leader
                safteyDistance = 3.0 + 0.5 * egoSpeed;
                distanceToUse = std::max(safteyDistance, mDist);

                diff = distanceToLeader - distanceToUse;
                acceleration = Kp * diff + Kd * (velocityLeader - egoSpeed);
            }
            else
            { // Has NO leader
                acceleration = Kp_velocity * (mVelocity - egoSpeed);
            }

            acceleration = std::clamp(acceleration, -1.0, 1.0);

            mSharedData.mMutex.lock();
            mSharedData.mAcceletation = acceleration;
            mSharedData.mMutex.unlock();

            mSharedData.os << "accelerazione: " << acceleration << " egoSpeed: " << egoSpeed << " velocityLeader: " << velocityLeader << " distanceToLeader: " << distanceToLeader << "  safteyDist: " << safteyDistance << " distToUse: " << distanceToUse << " diff: " << diff << " HasLeader: " << (distanceToLeader > 0) << std::endl;
            mSharedData.os << std::endl;
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