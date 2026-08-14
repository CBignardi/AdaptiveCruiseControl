#include <iostream>
#include <vector>

#include "utils.hpp"
#include "SensorThread.hpp"
#include "ControllerThread.hpp"

int main(void)
{
    EnvClient client;

    std::mutex mutexGlobalId;
    SharedData sharedData;

    json resp = client.sendComandAndRecive({{"cmd", "reset"}});

    SensorThread sensorThread(sharedData, client);
    sensorThread.start();

    ControllerThread controllerThread(sharedData);
    controllerThread.start(20, 25); // 20 meters, 25 m/s

    // priprità 20 per il controllore;

    sensorThread.join();
    controllerThread.join();

    return EXIT_SUCCESS;
}