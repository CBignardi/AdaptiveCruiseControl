#pragma once

#include <random>

#include "utils.hpp"

struct VehicleAttr
{
public:
    double mPresence; // 1 Real vehicle, 0 Placeholder
    double mX;
    double mY;
    double mVx;
    double mVy;
};

class SensorThread
{
public:
    SensorThread(SharedData &aSharedData, EnvClient &aClient, TaskWatch &aPeriodicTaskMonitor);

    void close();

    void start();

    void join();

private:
    std::atomic<bool> mRunning{false};
    SharedData &mSharedData;
    EnvClient &mClient;
    TaskWatch &mTaskWatch;

    std::thread mThread;

    void run();
};