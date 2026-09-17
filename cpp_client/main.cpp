#include <iostream>
#include <vector>

#include "utils.hpp"
#include "SensorThread.hpp"
#include "ControllerThread.hpp"
#include "SafteyThread.hpp"


int main(void)
{
    EnvClient client;

    // std::mutex mutexGlobalId;
    SharedData sharedData;

    Logger dataLogger({"acceleration", "egoSpeed", "leaderSpeed", "distanceLeader", "safteyGap"}, "dataLog.csv");
    dataLogger.startUDPConnection();

    Logger logTimesSensor({"Sensor/period", "Sensor/cycle"}, "timesSensorLog.csv");
    Logger logTimesControl({"Control/period", "Control/cycle"}, "timesControlLog.csv");
    Logger logTimesSaftey({"Saftey/period", "Saftey/cycle"}, "timesSafteyLog.csv");

    TaskWatch taskWatchSensor("Sensor", 20, logTimesSensor);
    TaskWatch taskWatchControl("Control", 10, logTimesControl);
    TaskWatch taskWatchSaftey("Saftey", 5, logTimesSaftey);

    json resp = client.sendComandAndRecive({{"cmd", "reset"}});

    SensorThread sensorThread(sharedData, client, taskWatchSensor);
    sensorThread.start();

    ControllerThread controllerThread(sharedData, dataLogger, taskWatchControl);
    controllerThread.start(30); // Target Speed: 30 m/s

    SafteyThread safteyThread(sharedData, sensorThread, controllerThread, client, taskWatchSaftey);
    safteyThread.start();

    sensorThread.join();
    controllerThread.join();
    safteyThread.join();

    return EXIT_SUCCESS;
}