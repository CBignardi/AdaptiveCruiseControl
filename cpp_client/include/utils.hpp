#pragma once

#include <mutex>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>
#include <atomic>
// #include "spdlog/spdlog.h"
// #include "spdlog/sinks/basic_file_sink.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <fstream>

using json = nlohmann::json;
using time_type = std::chrono::time_point<std::chrono::high_resolution_clock>;

struct SimData
{
    size_t mId = 0;
    double mEgoSpeed;
    bool mTerminated;
    bool mTruncated;
    double mDistanceToLeader;
    double mVelocityLeader;
};

class DataLogger
{
public:
    DataLogger(const std::vector<std::string> &aFields, const std::string &aCsvFilename, const std::string &aUdpHost = "127.0.0.1", int aUdpPort = 9870)
        : mFields(aFields)
    {
        // Setup for the .csv file
        mFile.open(aCsvFilename);
        mFile << "timestamp";
        for (const auto &f : mFields)
            mFile << "," << f;
        mFile << "\n";

        mStartTime = std::chrono::steady_clock::now();

        mSock = socket(AF_INET, SOCK_DGRAM, 0);
        mAddr.sin_family = AF_INET;
        mAddr.sin_port = htons(aUdpPort);
        inet_pton(AF_INET, aUdpHost.c_str(), &mAddr.sin_addr);
    }

    void log(const std::vector<double> &values)
    {
        double t = std::chrono::duration<double>(std::chrono::steady_clock::now() - mStartTime).count();

        std::lock_guard<std::mutex> lock(mMutex);

        // Write in the .csv file
        mFile << std::fixed << std::setprecision(6) << t;
        for (double v : values)
        {
            mFile << "," << v;
        }
        mFile << "\n";

        // Streaming to PlotJuggler
        std::ostringstream udpMsg;
        udpMsg << "{";
        udpMsg << "\"timestamp\":" << t;
        for (size_t i = 0; i < mFields.size() && i < values.size(); ++i)
        {
            udpMsg << ",\"" << mFields[i] << "\":" << values[i];
        }
        udpMsg << "}";
        std::string payload = udpMsg.str();
        sendto(mSock, payload.c_str(), (int)payload.size(), 0, reinterpret_cast<sockaddr *>(&mAddr), sizeof(mAddr));
    }

private:
    std::vector<std::string> mFields;
    std::ofstream mFile;
    std::mutex mMutex;
    std::chrono::steady_clock::time_point mStartTime;

    int mSock;
    sockaddr_in mAddr{};
};

struct SharedData
{
public:
    SharedData() {}

    // Simulator Data
    std::mutex mMutexSim;
    SimData mSimData;

    std::atomic<double> mAcceleration{0.0};

    void setAll(double aEgoSpeed, bool aTerminated, bool aTruncated, double aDistanceToLeader, double aVelocityLeader)
    {
        mSimData.mEgoSpeed = aEgoSpeed;
        mSimData.mTerminated = aTerminated;
        mSimData.mTruncated = aTruncated;
        mSimData.mDistanceToLeader = aDistanceToLeader;
        mSimData.mVelocityLeader = aVelocityLeader;
    }

    void incrementID() { mSimData.mId++; }
};

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

    json sendComandAndRecive(json msg)
    {
        std::string s = msg.dump();
        auto tmpIgnore1 = mSocket.send(zmq::buffer(s), zmq::send_flags::none);
        zmq::message_t reply;
        auto tmpIgnore2 = mSocket.recv(reply, zmq::recv_flags::none);
        return json::parse(reply.to_string());
    }
};