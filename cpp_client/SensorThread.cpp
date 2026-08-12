#include "SensorThread.hpp"
#include <limits>
#include <vector>
#include <random>

void SensorThread::close()
{
    mMutex.lock();
    mRunning = false;
    mMutex.unlock();
}

void SensorThread::start()
{
    mMutex.lock();
    mRunning = true;
    mMutex.unlock();

    std::thread t(&SensorThread::run, this);
    t.join();
}

struct VehicleAttr
{
public:
    double mPresence;
    double mX;
    double mY;
    double mVx;
    double mVy;
};

double calculateDistanceToLeader(std::vector<VehicleAttr> &aVectorVehicleAttr)
{
    double minDistanceToLeader = std::numeric_limits<double>::max();
    double yEgoPos = aVectorVehicleAttr.at(0).mY;

    for (int i = 1; i < aVectorVehicleAttr.size(); i++) // exclude ego pos
    {
        auto actualVehicle = aVectorVehicleAttr.at(i);

        if ((actualVehicle.mY == yEgoPos) && (actualVehicle.mX < minDistanceToLeader) && (actualVehicle.mPresence > 0.5))
        {
            minDistanceToLeader = actualVehicle.mX;
        }
    }
    return minDistanceToLeader;
}

void SensorThread::run()
{
    while (mRunning)
    {
        mStartTime = std::chrono::high_resolution_clock::now();
        // START function

        std::time_t start_time = std::chrono::system_clock::to_time_t(mStartTime);
        std::cout << "Start Time: " << std::ctime(&start_time) << std::endl;

        static double last_accel = 0.0;
        json resp = mClient.sendComandAndRecive({{"cmd", "step"}, {"accel", last_accel}});

        // fill the array with the Vehicles Attributes
        std::vector<VehicleAttr> vVehicleAttr;
        VehicleAttr vehicleAttr;
        for (const auto row : resp["obs"])
        {
            vehicleAttr.mPresence = row[0];
            vehicleAttr.mX = row[1];
            vehicleAttr.mY = row[2];
            vehicleAttr.mVx = row[3];
            vehicleAttr.mVy = row[4];
            vVehicleAttr.push_back(vehicleAttr);
        }

        std::random_device rd;
        std::mt19937 generator(rd());
        std::normal_distribution<double> dist(0.0, 0.3);

        double egoSpeed = vVehicleAttr.at(0).mVx + dist(generator);
        double distanceToLeader = calculateDistanceToLeader(vVehicleAttr) + dist(generator);

        mSharedData.setAll(egoSpeed, resp["terminated"], resp["truncated"], distanceToLeader);

        // END function
        mEndTime = std::chrono::high_resolution_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(mEndTime - mStartTime);
        std::chrono::duration<double> period(1.0 / mFreq);

        if (elapsed < period)
        {
            std::this_thread::sleep_for(period - elapsed);
        }
        else
        {
            std::cout << "DEADLINE MISSSS";
        }
    }
}
