#pragma once

#include <mutex>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <algorithm>
#include <vector>

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

struct LogData
{
    time_type mStartTime;
    time_type mEndTime;

    double mAcceleration;
    double mEgoSpeed;
    double mLeaderSpeed;
    double mDistanceToLeader;
    double mSafteyDistance;
    std::string mState;

    void setAll(time_type aStartTime, time_type aEndTime, double aAcceleration, double aEgoSpeed, double aLeaderSpeed, double aDistanceToLeader, double aSafteyDistance, std::string aState)
    {
        mStartTime = aStartTime;
        mEndTime = aEndTime;
        mAcceleration = aAcceleration;
        mEgoSpeed = aEgoSpeed;
        mLeaderSpeed = aLeaderSpeed;
        mDistanceToLeader = aDistanceToLeader;
        mSafteyDistance = aSafteyDistance;
        mState = aState;
    }

    void setParams(double aAcceleration, double aEgoSpeed, double aLeaderSpeed, double aDistanceToLeader, double aSafteyDistance, std::string aState)
    {
        mAcceleration = aAcceleration;
        mEgoSpeed = aEgoSpeed;
        mLeaderSpeed = aLeaderSpeed;
        mDistanceToLeader = aDistanceToLeader;
        mSafteyDistance = aSafteyDistance;
        mState = aState;
    }

    void setTime(time_type aStartTime, time_type aEndTime)
    {
        mStartTime = aStartTime;
        mEndTime = aEndTime;
    }
};

struct SharedData
{
public:
    SharedData() {}

    // Simulator Data
    std::mutex mMutexSim;
    SimData mSimData;
    double mAcceletation = 0.0;
    void setAll(double aEgoSpeed, bool aTerminated, bool aTruncated, double aDistanceToLeader, double aVelocityLeader)
    {
        mSimData.mId++;
        mSimData.mEgoSpeed = aEgoSpeed;
        mSimData.mTerminated = aTerminated;
        mSimData.mTruncated = aTruncated;
        mSimData.mDistanceToLeader = aDistanceToLeader;
        mSimData.mVelocityLeader = aVelocityLeader;
    }

    // Log Data
    std::mutex mMutexLog;
    std::vector<LogData> mVecSensor;
    std::vector<LogData> mVecControl;
};

class EnvClient
{
private:
    zmq::context_t mContext;
    zmq::socket_t mSocket;

public:
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