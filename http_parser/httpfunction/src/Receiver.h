#ifndef RECEIVE_H_
#define RECEIVE_H_

#include "Public.h"
#include "Handle.h"
static int counter;
class Receiver
{
public:
    Receiver();
    ~Receiver();
    void Work(char* msg, CSessionPtr& sesp);

private:
    void HandleMessage(char* msg, CSessionPtr& sesp);

    //zmq::context_t _ctx;
    //zmq::socket_t _socket;
    CHandle  _handle;
};

#endif
