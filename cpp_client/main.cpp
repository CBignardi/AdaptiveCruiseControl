#include <iostream>
#include <vector>

#include "utils.hpp"
#include "SensorThread.hpp"
#include "ControllerThread.hpp"
#include "SafteyThread.hpp"

// const double FREQ = 5;          // Hz
// const double FREQ_SENSOR = 20;  // Hz
// const double FREQ_CONTROL = 10; // Hz

int main(void)
{
    EnvClient client;

    // std::mutex mutexGlobalId;
    SharedData sharedData;

    DataLogger dataLogger({"acceleration", "egoSpeed", "leaderSpeed", "distanceLeader", "safteyGap"}, "dataLog.csv");
    dataLogger.startUDPConnection();

    DataLogger timesLoggerSensor({"period", "cycle"}, "timesSensorLog.csv");
    DataLogger timesLoggerControl({"period", "cycle"}, "timesControlLog.csv");
    DataLogger timesLoggerSaftey({"period", "cycle"}, "timesSafteyLog.csv");

    PeriodicTaskMonitor periodicMonitorSensor("Sensor", 20, timesLoggerSensor);
    PeriodicTaskMonitor periodicMonitorControl("Control", 10, timesLoggerControl);
    PeriodicTaskMonitor periodicMonitorSaftey("Saftey", 5, timesLoggerSaftey);

    json resp = client.sendComandAndRecive({{"cmd", "reset"}});

    SensorThread sensorThread(sharedData, client, periodicMonitorSensor);
    sensorThread.start();

    ControllerThread controllerThread(sharedData, dataLogger, periodicMonitorControl);
    controllerThread.start(30); // 30 m/s

    SafteyThread safteyThread(sharedData, sensorThread, controllerThread, client, periodicMonitorSaftey);
    safteyThread.start();

    sensorThread.join();
    controllerThread.join();
    safteyThread.join();

    return EXIT_SUCCESS;
}