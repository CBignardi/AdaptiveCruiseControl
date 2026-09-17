#pragma once

#include "utils.hpp"
#include "SensorThread.hpp"
#include "ControllerThread.hpp"

class SafteyThread
{
public:
    SafteyThread(SharedData &aSharedData, SensorThread &aSensorThread, ControllerThread &aControlThread, EnvClient &aClient,
                 TaskWatch &aTaskWatchSaftey, TaskWatch &aTaskWatchSensor, TaskWatch &aTaskWatchControl);

    void close();

    void start();

    void join();

private:
    std::atomic<bool> mRunning{false};
    SharedData &mSharedData;

    SensorThread &mSensorThread;
    ControllerThread &mControllerThread;
    EnvClient &mClient;
    TaskWatch &mTaskWatchSaftey;
    TaskWatch &mTaskWatchSensor;
    TaskWatch &mTaskWatchControl;

    std::thread mThread;

    void run();
};