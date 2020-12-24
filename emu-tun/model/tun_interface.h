#pragma once
#include <stdint.h>
#include <string>
#include <map>
#include "tun_mutex.h"
namespace  ns3{
class TunDevice{
public:
  virtual ~TunDevice(){}
  virtual void SendToDevice(const char *data,size_t size)=0;
  virtual std::string Name() const=0;
};
class NsEndpoint{
public:
    virtual ~NsEndpoint(){}
    virtual void OnPacket(const char*data,size_t size)=0;
};
class PacketDispatcher{
public:
    static PacketDispatcher* Instance();
    void OnPacket(char*data,size_t size);
    bool SubscribePacket(uint32_t src,NsEndpoint *sink);
    void UnSubscribe(uint32_t src,NsEndpoint *sink);
private:
    PacketDispatcher();
    mutable Mutex mutex_;
    std::map<uint32_t,NsEndpoint*> sinks_;
};
}
