#pragma once

#include <mutex>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>

#include <fstream>

using json = nlohmann::json;
using time_type = std::chrono::time_point<std::chrono::high_resolution_clock>;

struct SimData
{
    int mId = 0;
    double mEgoSpeed;
    bool mTerminated;
    bool mTruncated;
    double mDistanceToLeader;
    double mVelocityLeader;
};

struct LogData
{
    bool mFullLog;

    time_type mStartTime;
    time_type mEndTime;

    double mAcceleration;
    double mEgoSpeed;
    double mLeaderSpeed;
    double mDistanceToLeader;
    double mSafteyDistance;
    std::string mState;

    void setData(double aAcceleration, double aEgoSpeed, double aLeaderSpeed, double aDistanceToLeader, double aSafteyDistance, std::string aState)
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

struct QueueLog
{
    std::queue<LogData> mQueue;

    std::mutex mMutex;

    // bool isEmpty() { return mIndex >= mVec.size(); }

    size_t size()
    {
        mMutex.lock();
        auto tmp = mQueue.size();
        mMutex.unlock();
        return tmp;
    }

    LogData getElem()
    {
        mMutex.lock();
        if (mQueue.empty())
        {
            mMutex.unlock();
            throw std::runtime_error("Errore, getElem() chiamato con queue vuota");
        }
        auto tmp = mQueue.front();
        mQueue.pop();
        mMutex.unlock();
        return tmp;
    }

    void pushElem(LogData aLogData)
    {
        mMutex.lock();
        mQueue.push(aLogData);
        mMutex.unlock();
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
        mSimData.mEgoSpeed = aEgoSpeed;
        mSimData.mTerminated = aTerminated;
        mSimData.mTruncated = aTruncated;
        mSimData.mDistanceToLeader = aDistanceToLeader;
        mSimData.mVelocityLeader = aVelocityLeader;
    }

    void incrementID() { mSimData.mId++; }

    // Log Data
    QueueLog mQueueLog;
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

    // json sendComandAndRecive(json msg)
    // {

    //     auto t0 = std::chrono::high_resolution_clock::now();

    //     std::string s = msg.dump();
    //     auto t1 = std::chrono::high_resolution_clock::now();

    //     auto tmpIgnore1 = mSocket.send(zmq::buffer(s), zmq::send_flags::none);
    //     auto t2 = std::chrono::high_resolution_clock::now();

    //     zmq::message_t reply;
    //     auto tmpIgnore2 = mSocket.recv(reply, zmq::recv_flags::none);
    //     auto t3 = std::chrono::high_resolution_clock::now();

    //     json result = json::parse(reply.to_string());
    //     auto t4 = std::chrono::high_resolution_clock::now();

    //     std::cout << "dump: " << std::chrono::duration<double>(t1 - t0).count()
    //               << " send: " << std::chrono::duration<double>(t2 - t1).count()
    //               << " recv(wait): " << std::chrono::duration<double>(t3 - t2).count()
    //               << " parse: " << std::chrono::duration<double>(t4 - t3).count() << std::endl;

    //     return result;
    // }
};