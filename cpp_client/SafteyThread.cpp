#include "SafteyThread.hpp"

SafteyThread::SafteyThread(SharedData &aSharedData, SensorThread &aSensorThread, ControllerThread &aControlThread, EnvClient &aClient,
                           TaskWatch &aPeriodicTaskMonitor)
    : mSharedData(aSharedData), mSensorThread(aSensorThread), mControllerThread(aControlThread), mClient(aClient), mThread(&SafteyThread::run, this),
      mTaskWatch(aPeriodicTaskMonitor)
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

void checkDeadlines(TaskWatch &aPeriodicMonitor, const std::string aName)
{
    // ASSUMPION: the threads have the deadline equals to the period.

    std::vector<double> vecPeriods;
    std::vector<double> vecCycles;

    // Copy the vectors in the local variables
    aPeriodicMonitor.mMutex.lock();
    std::copy(aPeriodicMonitor.mVecPeriods.begin(), aPeriodicMonitor.mVecPeriods.end(), vecPeriods.begin());
    aPeriodicMonitor.mVecPeriods.clear();
    std::copy(aPeriodicMonitor.mVecCycles.begin(), aPeriodicMonitor.mVecCycles.end(), vecCycles.begin());
    aPeriodicMonitor.mVecCycles.clear();
    aPeriodicMonitor.mMutex.unlock();

    auto period = aPeriodicMonitor.getPeriod();

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
        mTaskWatch.startCycle();

        mSharedData.mMutexSim.lock();
        size_t actualId = mSharedData.mSimData.mId;
        bool trucated = mSharedData.mSimData.mTruncated;
        bool terminated = mSharedData.mSimData.mTerminated;
        mSharedData.mMutexSim.unlock();

        if (actualId > lastId)
        {
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

        // END of the function
        // std::cout << "fine ciclo\n";
        mTaskWatch.endCycle();
        // std::cout << "fine ciclo 2\n";
        mTaskWatch.sleep();
    }
}
