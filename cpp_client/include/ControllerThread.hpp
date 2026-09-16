#pragma once

#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>

#include "utils.hpp"

class ControllerThread
{
public:
    ControllerThread(SharedData &aSharedData, DataLogger& aDataLogger, PeriodicTaskMonitor &aPeriodicTaskMonitor);

    void close();

    void start(double aVelocity);

    void join();

private:
    std::atomic<bool> mRunning{false};
    SharedData &mSharedData;
    DataLogger &mDataLogger;
    PeriodicTaskMonitor &mPeriodicTaskMonitor;

    std::thread mThread;

    double mVelocity;

    void run();
};