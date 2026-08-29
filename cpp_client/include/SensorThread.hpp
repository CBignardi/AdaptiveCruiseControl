#pragma once

#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>

#include "utils.hpp"

using time_type = std::chrono::time_point<std::chrono::high_resolution_clock>;

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
    SensorThread(SharedData &aSharedData, EnvClient &aClient);

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
    std::chrono::duration<double> mPeriod{1.0 / 45}; // 45Hz freq

    std::thread mThread;

    void run();
};