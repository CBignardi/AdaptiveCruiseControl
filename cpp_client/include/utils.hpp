#pragma once

#include <mutex>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <algorithm>

using json = nlohmann::json;


// size_t globalId = 0;

struct SharedData
{
public:
    SharedData() {}

    void incrementId()
    {
        mMutex.lock();
        mId++;
        mMutex.unlock();
    }

    double getEgoSpeed()
    {
        mMutex.lock();
        auto tmp = mEgoSpeed;
        mMutex.unlock();
        return tmp;
    }

    bool getTerminated()
    {
        mMutex.lock();
        auto tmp = mTerminated;
        mMutex.unlock();
        return tmp;
    }

    bool getTruncated()
    {
        mMutex.lock();
        auto tmp = mTruncated;
        mMutex.unlock();
        return tmp;
    }

    double getDistanceToLeader()
    {
        mMutex.lock();
        auto tmp = mDistanceToLeader;
        mMutex.unlock();
        return tmp;
    }

    void setEgoSpeed(double value)
    {
        mMutex.lock();
        mEgoSpeed = value;
        mMutex.unlock();
    }

    void setTerminated(bool value)
    {
        mMutex.lock();
        mTerminated = value;
        mMutex.unlock();
    }

    void setTruncated(bool value)
    {
        mMutex.lock();
        mTruncated = value;
        mMutex.unlock();
    }

    void setDistanceToLeader(double value)
    {
        mMutex.lock();
        mDistanceToLeader = value;
        mMutex.unlock();
    }

    void setAll(double aEgoSpeed, bool aTerminated, bool aTruncated, double aDistanceToLeader)
    {
        mMutex.lock();
        mId++;
        mEgoSpeed = aEgoSpeed;
        mTerminated = aTerminated;
        mTruncated = aTruncated;
        mDistanceToLeader = aDistanceToLeader;
        mMutex.unlock();
    }

private:
    size_t mId = 0;
    double mEgoSpeed;
    bool mTerminated;
    bool mTruncated;
    double mDistanceToLeader;
    std::mutex mMutex;
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
        mSocket.send(zmq::buffer(s), zmq::send_flags::none);
        zmq::message_t reply;
        mSocket.recv(reply, zmq::recv_flags::none);
        return json::parse(reply.to_string());
    }
};