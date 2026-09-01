#include <iostream>
#include <vector>

#include "utils.hpp"
#include "SensorThread.hpp"
#include "ControllerThread.hpp"
#include "SafteyThread.hpp"

int main(void)
{
    EnvClient client;

    std::mutex mutexGlobalId;
    SharedData sharedData;

    json resp = client.sendComandAndRecive({{"cmd", "reset"}});

    SensorThread sensorThread(sharedData, client);
    sensorThread.start();

    ControllerThread controllerThread(sharedData);
    controllerThread.start(30); // 30 m/s

    SafteyThread safteyThread(sharedData, sensorThread, controllerThread, client);
    safteyThread.start();

    sensorThread.join();
    controllerThread.join();
    safteyThread.join();

    return EXIT_SUCCESS;
}