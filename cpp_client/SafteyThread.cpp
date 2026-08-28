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

// void SafteyThread::printLog()
// {
// auto elem = aVecLog.getElem();

// if (aIsSensor)
// {
//     std::chrono::duration<double> periodSensor(1.0 / 25);

//     mOsLogTimes << "SensorThread -> duration: " << duration.count() << " period: " << periodSensor.count()
//                 << " startTime: " << formatTimestamp(elem.mStartTime) << std::endl;
//     if (duration > periodSensor)
//     {
//         // Deadline Miss
//         std::cout << "Sensor Thread DEADLINE MISS" << std::endl;
//     }
// }
// else
// {
//     std::chrono::duration<double> periodControl(1.0 / 10);

//     mOsLogTimes << "ControlThread -> duration: " << duration.count() << " period: " << periodControl.count()
//                 << " startTime: " << formatTimestamp(elem.mStartTime) << std::endl;
//     if (duration > periodControl)
//     {
//         // Deadline Miss
//         std::cout << "Control Thread DEADLINE MISS" << std::endl;
//     }
// }
// }

void SafteyThread::run()
{
    while (mRunning.load())
    {
        mStartTime = std::chrono::high_resolution_clock::now();
        // START of the function

        auto sizeQueue = mSharedData.mQueueLog.size();

        for (int i = 0; i < sizeQueue; i++) // Used to bound the print of log
        {
            LogData log = mSharedData.mQueueLog.getElem();
            auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(log.mEndTime - log.mStartTime);

            std::chrono::duration<double> period;
            std::string name;

            if (log.mFullLog)
            { // It is a Full Log (Control Log)

                mOsLogData << "Acc: " << log.mAcceleration
                           << " EgoSpeed: " << log.mEgoSpeed << " LeaderSpeed: " << log.mLeaderSpeed
                           << " DistToLeader: " << log.mDistanceToLeader << " SafteyDist: " << log.mSafteyDistance << std::endl;

                // Setup some variables for the full print in LogTimes
                name = "Control";
                period = std::chrono::duration<double>(1.0 / 10);
            }
            else
            { // It is not a Full log (Sensor Log)

                // Setup some variables for the full print in LogTimes
                name = "Sensor";
                period = std::chrono::duration<double>(1.0 / 25);
            }

            mOsLogTimes << name << "Thread -> duration: " << duration.count() << " period: " << period.count()
                        << " startTime: " << formatTimestamp(log.mStartTime) << std::endl;
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
