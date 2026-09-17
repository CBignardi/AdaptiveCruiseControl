#pragma once

#include "utils.hpp"

class ControllerThread
{
public:
    ControllerThread(SharedData &aSharedData, Logger &aDataLogger, TaskWatch &aPeriodicTaskMonitor);

    void close();

    void start(double aVelocity);

    void join();

private:
    std::atomic<bool> mRunning{false};
    SharedData &mSharedData;
    Logger &mDataLogger;
    TaskWatch &mTaskWatch;

    std::thread mThread;

    double mSpeed;

    void run();
};