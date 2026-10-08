#include "SafteyThread.hpp"

SafteyThread::SafteyThread(SharedData &aSharedData, SensorThread &aSensorThread, ControllerThread &aControlThread, EnvClient &aClient,
                           TaskWatch &aTaskWatchSaftey, TaskWatch &aTaskWatchSensor, TaskWatch &aTaskWatchControl)
    : mSharedData(aSharedData), mSensorThread(aSensorThread), mControllerThread(aControlThread), mClient(aClient), mThread(&SafteyThread::run, this),
      mTaskWatchSaftey(aTaskWatchSaftey), mTaskWatchSensor(aTaskWatchSensor), mTaskWatchControl(aTaskWatchControl)
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

void checkDeadlines(TaskWatch &aTaskWatch, const std::string aName)
{
    // ASSUMPION: the threads have the deadline equals to the period.

    std::vector<double> vecPeriods;
    std::vector<double> vecCycles;

    // Copy the vectors in the local variables
    aTaskWatch.mMutex.lock();

    vecPeriods.resize(aTaskWatch.mVecPeriods.size());
    std::copy(aTaskWatch.mVecPeriods.begin(), aTaskWatch.mVecPeriods.end(), vecPeriods.begin());
    aTaskWatch.mVecPeriods.clear();

    vecCycles.resize(aTaskWatch.mVecCycles.size());
    std::copy(aTaskWatch.mVecCycles.begin(), aTaskWatch.mVecCycles.end(), vecCycles.begin());
    aTaskWatch.mVecCycles.clear();

    aTaskWatch.mMutex.unlock();

    auto period = aTaskWatch.getPeriod() * 1000.0; // Convert to millisecond

    // The two vectors have the same size
    for (int i = 0; i < vecPeriods.size(); i++)
    {
        // Accept a little error (10% error)
        if (vecPeriods.at(i) > period * 1.1)
        { // Period miss
            std::cout << "Saftey: " << aName << " Thread missed period, Actual Period: " << vecPeriods.at(i) << ", Expected Period: " << period * 1.1 << std::endl;
        }
        if (vecCycles.at(i) > period)
        { // Deadline miss
            std::cout << "Saftey: " << aName << " Thread missed deadline, Actual Cycle: " << vecCycles.at(i) << ", Max Cycle: " << period << std::endl;
        }
    }
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
    size_t lastId = 0;

    while (mRunning.load())
    {
        // START of the function
        mTaskWatchSaftey.startCycle();

        mSharedData.mMutexSim.lock();
        size_t actualId = mSharedData.mId;
        bool trucated = mSharedData.mTruncated;
        bool terminated = mSharedData.mTerminated;
        mSharedData.mMutexSim.unlock();

        if (actualId > lastId)
        {
            // Check the Simulation
            if (trucated == true || terminated == true)
            { // Simulation died
                std::cout << "Saftey: SIMULATION DIED " << trucated << " " << terminated << std::endl;
                mSensorThread.close();
                mControllerThread.close();
                close();

                mClient.mMutex.lock();
                mClient.sendComandAndRecive({{"cmd", "close"}});
                mClient.mMutex.unlock();
            }
            lastId = actualId;
        }

        checkDeadlines(mTaskWatchControl, "Control");
        checkDeadlines(mTaskWatchSaftey, "Saftey");

        // END of the function
        mTaskWatchSaftey.endCycle();
        mTaskWatchSaftey.sleep();
    }
}
