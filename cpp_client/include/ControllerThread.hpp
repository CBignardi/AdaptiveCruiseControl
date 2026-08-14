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

    void start(double aDist, double aVelocity);

    void join();

private:
    double mFreq;
    std::atomic<bool> mRunning{false};
    SharedData &mSharedData;
    time_type mStartTime;
    time_type mEndTime;

    size_t mLastId;

    std::mutex mMutex;
    std::thread mThread;

    double mDist;
    double mVelocity;

    void run();
};