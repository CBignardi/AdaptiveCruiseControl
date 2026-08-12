#pragma once

#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>


#include "utils.hpp"

using time_type = std::chrono::time_point<std::chrono::high_resolution_clock>;

class SensorThread
{
public:
    SensorThread(double aFreq, SharedData &aSharedData, EnvClient& aClient) : mFreq(aFreq), mSharedData(aSharedData), mClient(aClient), mThread(&SensorThread::run, this) {}

    void close();

    void start();

    void join();

private:
    double mFreq;
    std::atomic<bool> mRunning{false};
    SharedData &mSharedData;
    EnvClient &mClient;
    time_type mStartTime;
    time_type mEndTime;

    std::mutex mMutex;
    std::thread mThread;

    void run();
};