#pragma once

#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>

#include "utils.hpp"
#include "SensorThread.hpp"
#include "ControllerThread.hpp"

class SafteyThread
{
public:
    SafteyThread(SharedData &aSharedData, SensorThread &aSensorThread, ControllerThread &aControlThread, EnvClient &aClient);

    void close();

    void start();

    void join();

private:
    std::atomic<bool> mRunning{false};
    SharedData &mSharedData;

    SensorThread &mSensorThread;
    ControllerThread &mControllerThread;
    EnvClient &mClient;

    std::thread mThread;

    void run();
};