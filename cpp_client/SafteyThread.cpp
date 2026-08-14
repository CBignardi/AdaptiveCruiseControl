#include "SafteyThread.hpp"

SafteyThread::SafteyThread(SharedData &aSharedData) : mSharedData(aSharedData), mThread(&SafteyThread::run, this)
{
    sched_param sch;
    sch.sched_priority = 20;
    int policy = SCHED_FIFO;
    int result = pthread_setschedparam(mThread.native_handle(), policy, &sch);
    if (result != 0)
    {
        throw std::runtime_error("Error scheduling parameter. " + std::string(strerror(result)));
    }

    pthread_getschedparam(mThread.native_handle(), &policy, &sch);
    std::cout << "ThreadSaftey" << " is executing at priority " << sch.sched_priority << std::endl;
}

void SafteyThread::close()
{
    mRunning.store(false);
}

void SafteyThread::start()
{
    mRunning.store(true);
}

void SafteyThread::join()
{
    mThread.join();
}

std::string formatTimestamp(std::chrono::time_point<std::chrono::system_clock> aTime)
{
    std::time_t timestamp = std::chrono::system_clock::to_time_t(aTime);

    auto duration = aTime.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&timestamp), "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << millis.count();

    return oss.str();
}

struct VectorLog
{
    std::vector<LogData> mVec;
    int mIndex = 0;

    bool isEmpty() { return mIndex >= mVec.size(); }

    LogData getElem() { return mVec.at(mIndex); }
};

void SafteyThread::run()
{
    while (mRunning.load())
    {
        mStartTime = std::chrono::high_resolution_clock::now();
        // START of the function

        // Copy the data from the vector to a local vector
        VectorLog vecSensor;
        VectorLog vecControl;

        mSharedData.mMutexLog.lock();
        std::copy(mSharedData.mVecSensor.begin(), mSharedData.mVecSensor.end(), vecSensor.mVec.begin());
        std::copy(mSharedData.mVecControl.begin(), mSharedData.mVecControl.end(), vecControl.mVec.begin());
        mSharedData.mVecSensor.clear();
        mSharedData.mVecControl.clear();
        mSharedData.mMutexLog.unlock();

        // debug print...

        for (const auto &log : vecControl.mVec)
        {
            mOsLogData << "Acc: " << log.mAcceleration
                       << " EgoSpeed: " << log.mEgoSpeed << " LeaderSpeed: " << log.mLeaderSpeed
                       << " DistToLeader: " << log.mDistanceToLeader << " SafteyDist: " << log.mSafteyDistance << std::endl;
        }

        std::chrono::duration<double> periodControl(1.0 / 10);
        std::chrono::duration<double> periodSensor(1.0 / 25);
        std::string name;

        while (!vecControl.isEmpty() || !vecSensor.isEmpty()) // Continue until both vector are empty
        {
            // Assumpion in this round: one or both are NOT empty

            VectorLog vecTmp;

            // Find whitch log is the most recent
            if (vecControl.isEmpty())
            { // Print only Sensors (vecControl: empty, vecSensor: something; garantee by assumption)
                vecTmp = vecSensor;
                name = "Sensors";
            }
            // (vecControl: something, vecSensors: empty/something)
            else if (vecSensor.isEmpty())
            { // Print only Control (vecControl: something, vecSensor: empty; garantee by assumption)
                vecTmp = vecControl;
                name = "Control";
            }
            else // Both vec contains something
            {
                auto logControl = vecControl.getElem();
                auto logSensor = vecSensor.getElem();

                if (logSensor.mStartTime < logControl.mStartTime)
                {
                    vecTmp = vecSensor;
                    name = "Sensors";
                }
                else
                {
                    vecTmp = vecControl;
                    name = "Control";
                }
            }

            // Use vecTmp to print
            auto elem = vecTmp.getElem();
            auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(elem.mEndTime - elem.mStartTime);

            mOsLogTimes << name << "Thread -> duration: " << duration.count() << " period: ";
            if (name == "sensors")
            {
                mOsLogTimes << periodSensor.count() << " startTime: " << formatTimestamp(elem.mStartTime) << std::endl;
                if (duration > periodSensor)
                {
                    // Deadline Miss
                    std::cout << "Sensor Thread DEADLINE MISS" << std::endl;
                }
            }
            else
            {
                mOsLogTimes << periodControl.count() << " startTime: " << formatTimestamp(elem.mStartTime) << std::endl;
                if (duration > periodControl)
                {
                    // Deadline Miss
                    std::cout << "Control Thread DEADLINE MISS" << std::endl;
                }
            }
        }

        // END of the function
        mEndTime = std::chrono::high_resolution_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(mEndTime - mStartTime);

        mOsLogTimes << "SafteyThread -> duration: " << elapsed.count() << " period: " << mPeriod.count() << " startTime: " << formatTimestamp(mStartTime) << std::endl;

        if (elapsed < mPeriod)
        {
            std::this_thread::sleep_for(mPeriod - elapsed);
        }
        else
        {
            std::cout << "Saftey Thread DEADLINE MISS" << std::endl;
        }
    }
}
