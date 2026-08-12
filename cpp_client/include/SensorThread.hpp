#pragma once

#include <thread>
#include <mutex>
#include <chrono>

#include "utils.hpp"

using time_type = std::chrono::time_point<std::chrono::high_resolution_clock>;

class SensorThread
{
public:
    SensorThread(double aFreq, SharedData &aSharedData, EnvClient& aClient) : mFreq(aFreq), mSharedData(aSharedData), mClient(aClient) {}

    void close();

    void start();

private:
    double mFreq;
    bool mRunning = false;
    SharedData &mSharedData;
    EnvClient &mClient;
    time_type mStartTime;
    time_type mEndTime;

    std::mutex mMutex;

    void run();
};