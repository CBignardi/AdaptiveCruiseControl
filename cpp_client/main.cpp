#include <iostream>
#include <vector>

#include "utils.hpp"
#include "SensorThread.hpp"


int main(void)
{
    std::cout << "Inizio main" << std::endl;
    EnvClient client;

    std::mutex mutexGlobalId;
    SharedData sharedData;

    json resp = client.sendComandAndRecive({{"cmd", "reset"}});

    SensorThread sensorThread(20.0, sharedData, client);

    sensorThread.start();

    

    return EXIT_SUCCESS;
}