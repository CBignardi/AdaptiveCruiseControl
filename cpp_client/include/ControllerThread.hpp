#pragma once

#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>

#include "utils.hpp"

using time_type = std::chrono::time_point<std::chrono::high_resolution_clock>;

class ControllerThread
{
public:
    ControllerThread(SharedData &aSharedData);

    void close();

    void start(double aVelocity);

    void join();

private:
    std::atomic<bool> mRunning{false};
    SharedData &mSharedData;
    time_type mStartTime;
    time_type mEndTime;
    std::chrono::duration<double> mPeriod{1.0 / 25}; // 25Hz freq

    std::thread mThread;

    double mVelocity;

    void run();
};