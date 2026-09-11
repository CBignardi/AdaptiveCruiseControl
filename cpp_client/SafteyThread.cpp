#include "SafteyThread.hpp"

// Parameters
const double FREQ = 5;          // Hz
const double FREQ_SENSOR = 20;  // Hz
const double FREQ_CONTROL = 10; // Hz

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
    size_t lastId = 0;

    while (mRunning.load())
    {
        auto startTime = std::chrono::high_resolution_clock::now();
        // START of the function

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
        }

        // END of the function
        auto endTime = std::chrono::high_resolution_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);

        if (elapsed < periodThread)
        {
            std::this_thread::sleep_for(periodThread - elapsed);
        }
        else
        {
            std::cout << "Saftey Thread DEADLINE MISS" << std::endl;
        }
    }
}
