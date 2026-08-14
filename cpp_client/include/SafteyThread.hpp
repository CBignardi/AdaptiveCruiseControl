#pragma once

#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>

#include "utils.hpp"

using time_type = std::chrono::time_point<std::chrono::high_resolution_clock>;

class SafteyThread
{
public:
    SafteyThread(SharedData &aSharedData);

    void close();

    void start();

    void join();

private:
    std::atomic<bool> mRunning{false};
    SharedData &mSharedData;
    time_type mStartTime;
    time_type mEndTime;
    std::chrono::duration<double> mPeriod{1.0 / 5}; // 5Hz freq

    std::ofstream mOsLogData{"logData.txt", std::ios::binary};
    std::ofstream mOsLogTimes{"logTimes.txt", std::ios::binary};

    std::thread mThread;

    void run();
};