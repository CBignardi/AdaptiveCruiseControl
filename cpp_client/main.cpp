#include "utils.hpp"
#include "SensorThread.hpp"
#include "ControllerThread.hpp"
#include "SafteyThread.hpp"

int main(void)
{
    EnvClient client;

    SharedData sharedData;

    // Define all the logger objects
    Logger dataLogger({"acceleration", "egoSpeed", "leaderSpeed", "distanceLeader", "dinamicMinGap", "filteredLeaderSpeed", "filteredDistanceLeader"}, "dataLog.csv");
    dataLogger.startUDPConnection();
    Logger logTimesSensor({"Sensor/period", "Sensor/cycle"}, "timesSensorLog.csv");
    Logger logTimesControl({"Control/period", "Control/cycle"}, "timesControlLog.csv");
    Logger logTimesSaftey({"Saftey/period", "Saftey/cycle"}, "timesSafteyLog.csv");

    // Define the watcher for the deadline of the tasks
    TaskWatch taskWatchSensor(20, logTimesSensor);
    TaskWatch taskWatchControl(10, logTimesControl);
    TaskWatch taskWatchSaftey(5, logTimesSaftey);

    // Reset the simulation
    json resp = client.sendComandAndRecive({{"cmd", "reset"}});

    // Start all the threads
    SensorThread sensorThread(sharedData, client, taskWatchSensor);
    sensorThread.start();

    ControllerThread controllerThread(sharedData, dataLogger, taskWatchControl);
    controllerThread.start(30); // Target Speed: 30 m/s

    SafteyThread safteyThread(sharedData, sensorThread, controllerThread, client, taskWatchSaftey, taskWatchSensor, taskWatchControl);
    safteyThread.start();

    sensorThread.join();
    controllerThread.join();
    safteyThread.join();

    return EXIT_SUCCESS;
}