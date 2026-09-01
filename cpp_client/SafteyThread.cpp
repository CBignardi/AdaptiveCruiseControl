#include "SafteyThread.hpp"

// Parameters
const double FREQ = 5;          // Hz
const double FREQ_SENSOR = 25;  // Hz
const double FREQ_CONTROL = 10; // Hz
const std::string FILENAME_LOGDATA = "logData.txt";
const std::string FILENAME_LOGTIMES = "logTimes.txt";

SafteyThread::SafteyThread(SharedData &aSharedData, SensorThread &aSensorThread, ControllerThread &aControlThread, EnvClient &aClient)
    : mSharedData(aSharedData), mSensorThread(aSensorThread), mControllerThread(aControlThread), mClient(aClient), mThread(&SafteyThread::run, this)
{
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

void SafteyThread::run()
{
    sched_param sch;
    sch.sched_priority = 20;
    int result = pthread_setschedparam(mThread.native_handle(), SCHED_FIFO, &sch);
    if (result != 0)
    {
        throw std::runtime_error("Error scheduling parameter. " + std::string(strerror(result)));
    }

    // Variables
    std::chrono::duration<double> periodThread(1.0 / FREQ);
    std::ofstream osLogData(FILENAME_LOGDATA);
    std::ofstream osLogTimes(FILENAME_LOGTIMES);

    while (mRunning.load())
    {
        auto startTime = std::chrono::high_resolution_clock::now();
        // START of the function

        mSharedData.mMutexSim.lock();
        auto trucated = mSharedData.mSimData.mTruncated;
        auto terminated = mSharedData.mSimData.mTerminated;
        mSharedData.mMutexSim.unlock();

        if (trucated == true || terminated == true)
        { // Simulation died
            std::cout << "Saftey: SIMULATION DIED\n";
            mSensorThread.close();
            mControllerThread.close();
            close();
            mClient.sendComandAndRecive({{"cmd", "close"}});
        }

        auto sizeQueue = mSharedData.mQueueLog.size();

        for (int i = 0; i < sizeQueue; i++) // Used to bound the print of log
        {
            LogData log = mSharedData.mQueueLog.getElem();
            auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(log.mEndTime - log.mStartTime);

            std::chrono::duration<double> period;
            std::string name;

            if (log.mFullLog)
            { // It is a Full Log (Control Log)

                osLogData << std::fixed << std::setprecision(2)
                          << "Acc: " << log.mAcceleration
                          << " EgoSpeed: " << log.mEgoSpeed << " LeaderSpeed: " << log.mLeaderSpeed
                          << " DistToLeader: " << log.mDistanceToLeader << " SafteyDist: " << log.mSafteyDistance << std::endl;

                // Setup some variables for the full print in LogTimes
                name = "Control";
                period = std::chrono::duration<double>(1.0 / FREQ_CONTROL);
            }
            else
            { // It is not a Full log (Sensor Log)

                // Setup some variables for the full print in LogTimes
                name = "Sensor";
                period = std::chrono::duration<double>(1.0 / FREQ_SENSOR);
            }

            osLogTimes << name << "Thread -> duration: " << duration.count() << " period: " << period.count()
                       << " startTime: " << formatTimestamp(log.mStartTime) << std::endl;

            if (duration > period)
            {
                std::cout << name << " Thread DEADLINE MISS" << std::endl;
                osLogTimes << name << " Thread DEADLINE MISS" << std::endl;
            }
        }

        // END of the function
        auto endTime = std::chrono::high_resolution_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);

        osLogTimes << "LogThread -> duration: " << elapsed.count() << " period: " << periodThread.count() << " startTime: " << formatTimestamp(startTime) << std::endl;

        if (elapsed < periodThread)
        {
            std::this_thread::sleep_for(periodThread - elapsed);
        }
        else
        {
            std::cout << "Saftey Thread DEADLINE MISS" << std::endl;
            osLogTimes << "Saftey Thread DEADLINE MISS" << std::endl;
        }
    }
}
