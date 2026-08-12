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

    size_t mId = 0;
    double mEgoSpeed;
    bool mTerminated;
    bool mTruncated;
    double mDistanceToLeader;
    double mVelocityLeader;

    double mAcceletation = 0.0;
    std::mutex mMutex;

    void setAll(double aEgoSpeed, bool aTerminated, bool aTruncated, double aDistanceToLeader, double aVelocityLeader)
    {
        mId++;
        mEgoSpeed = aEgoSpeed;
        mTerminated = aTerminated;
        mTruncated = aTruncated;
        mDistanceToLeader = aDistanceToLeader;
        mVelocityLeader = aVelocityLeader;
    }
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