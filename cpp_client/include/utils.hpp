#pragma once

#include <mutex>
#include <thread>
#include <chrono>

#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <algorithm>
#include <vector>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <fstream>

using json = nlohmann::json;

// Helped by AI
class Logger
{
private:
    std::vector<std::string> mFields;
    std::ofstream mFile;
    std::chrono::steady_clock::time_point mStartTime;

    int mSock;
    sockaddr_in mAddr{};

public:
    Logger(const std::vector<std::string> &aFields, const std::string &aCsvFilename)
        : mFields(aFields)
    {
        // Setup for the .csv file
        mFile.open(aCsvFilename);
        mFile << "timestamp";
        for (const auto &f : mFields)
            mFile << ",/" << f;
        mFile << "\n";

        mStartTime = std::chrono::steady_clock::now();
    }

    void startUDPConnection()
    {
        mSock = socket(AF_INET, SOCK_DGRAM, 0);
        mAddr.sin_family = AF_INET;
        mAddr.sin_port = htons(9870);
        inet_pton(AF_INET, "127.0.0.1", &mAddr.sin_addr);
    }

    double logCSV(const std::vector<double> &aValues)
    {
        double t = std::chrono::duration<double>(std::chrono::steady_clock::now() - mStartTime).count();

        // Write in the .csv file
        mFile << std::fixed << std::setprecision(6) << t;
        for (double v : aValues)
        {
            mFile << "," << v;
        }
        mFile << "\n";

        return t;
    }

    void log(const std::vector<double> &aValues)
    {
        // Write in the .csv file
        double t = logCSV(aValues);

        // Streaming to PlotJuggler
        std::ostringstream udpMsg;
        udpMsg << "{";
        udpMsg << "\"timestamp\":" << t;
        for (size_t i = 0; i < mFields.size() && i < aValues.size(); ++i)
        {
            udpMsg << ",\"" << mFields[i] << "\":" << aValues[i];
        }
        udpMsg << "}";
        std::string payload = udpMsg.str();
        sendto(mSock, payload.c_str(), (int)payload.size(), 0, reinterpret_cast<sockaddr *>(&mAddr), sizeof(mAddr));
    }

    void flush()
    {
        mFile.flush();
    }
};

struct SharedData
{
public:
    SharedData() {}

    std::mutex mMutexSim;

    // Simulator Data
    size_t mId = 0;
    double mEgoSpeed;
    bool mTerminated;
    bool mTruncated;
    double mDistanceToLeader;
    double mVelocityLeader;

    std::atomic<double> mAcceleration{0.0};

    void setAll(double aEgoSpeed, bool aTerminated, bool aTruncated, double aDistanceToLeader, double aVelocityLeader)
    {
        mEgoSpeed = aEgoSpeed;
        mTerminated = aTerminated;
        mTruncated = aTruncated;
        mDistanceToLeader = aDistanceToLeader;
        mVelocityLeader = aVelocityLeader;
    }

    void incrementID() { mId++; }
};

// Helped by AI
class EnvClient
{
private:
    zmq::context_t mContext;
    zmq::socket_t mSocket;

public:
    std::mutex mMutex;

    EnvClient() : mContext(1), mSocket(mContext, zmq::socket_type::req)
    {
        mSocket.connect("tcp://localhost:5555");
    }

    json sendComandAndRecive(json aMsg)
    {
        std::string s = aMsg.dump();
        auto tmpIgnore1 = mSocket.send(zmq::buffer(s), zmq::send_flags::none);
        zmq::message_t reply;
        auto tmpIgnore2 = mSocket.recv(reply, zmq::recv_flags::none);
        return json::parse(reply.to_string());
    }
};

// Helped by AI
class TaskWatch
{
    using Clock = std::chrono::steady_clock;

private:
    double mPeriod;

    Clock::time_point mStartTime;
    bool mHasStarted = false;

    std::chrono::duration<double, std::milli> mLastPeriod;
    std::chrono::duration<double, std::milli> mLastCycle;
    Logger &mLogger;
    bool mHasStartedLogging = false;

public:
    TaskWatch(double aFreq, Logger &aLogger) : mPeriod(1 / aFreq), mLogger(aLogger) {}

    std::mutex mMutex;
    std::vector<double> mVecPeriods;
    std::vector<double> mVecCycles;

    void startCycle()
    {
        auto now = Clock::now();

        if (mHasStarted)
        {
            mLastPeriod = std::chrono::duration<double, std::milli>(now - mStartTime);
        }
        mStartTime = now;
        mHasStarted = true;
    }

    void endCycle()
    {
        mLastCycle = std::chrono::duration<double, std::milli>(Clock::now() - mStartTime);

        // Log the data (skip the first log, because mLastPeriod will be not initialize)
        if (mHasStartedLogging)
        {
            mLogger.logCSV({mLastPeriod.count(), mLastCycle.count()});
            mLogger.flush();

            mMutex.lock();
            mVecPeriods.push_back(mLastPeriod.count());
            mVecCycles.push_back(mLastCycle.count());
            mMutex.unlock();
        }
        mHasStartedLogging = true;
    }

    void sleep()
    {
        double periodMillisec = mPeriod * 1000.0; // Conversion to millisecond
        if (periodMillisec > mLastCycle.count())
        {
            auto timeToSleep = periodMillisec - mLastCycle.count();
            std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(timeToSleep));
        }
    }

    double getPeriod() { return mPeriod; }
};