#pragma once

#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>

#include "utils.hpp"

struct LeaderDistanceVelocity
{
    double mDistanceToLeader = 0;
    double mVelocity = 0;
};

struct VehicleAttr
{
public:
    double mPresence;
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
    double mFreq;
    std::atomic<bool> mRunning{false};
    SharedData &mSharedData;
    EnvClient &mClient;
    TaskWatch &mTaskWatch;

    std::thread mThread;

    void run();
};