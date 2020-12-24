#pragma once
#include <string>
#include <atomic>
#include "ns3/event-id.h"
#include "ns3/callback.h"
#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/core-module.h"
#include "ns3/application.h"
#include "ns3/tun_interface.h"
#include "ns3/tun_mutex.h"
namespace ns3{
class EmuUdpSender:public Application,
public NsEndpoint{
public:
    EmuUdpSender(TunDevice*tun,std::string &real_src);
	void Bind(uint16_t port);
	void ConfigurePeer(Ipv4Address addr,uint16_t port);
    void OnPacket(const char*data,size_t size) override;
private:
	virtual void StartApplication() override;
	virtual void StopApplication() override;
	void RecvPacket(Ptr<Socket> socket);
    void DeliveryData();
    void SendToNetwork(Ptr<Packet> p);
    TunDevice *tun_;
    uint32_t src_ip_;
    Ipv4Address peer_ip_;
    uint16_t peer_port_=0;
    uint16_t bind_port_;
    Ptr<Socket> socket_;
    uint32_t context_=0;
    mutable Mutex mutex_;
    std::string buffer_;
    std::atomic<bool> in_delivery_{false};
    bool running_=true;
};
class EmuUdpSink:public Application,
public NsEndpoint{
public:
    EmuUdpSink(TunDevice*tun,std::string &real_src);
    void Bind(uint16_t port);
    InetSocketAddress GetLocalAddress();
    void OnPacket(const char*data,size_t size) override;
private:
	virtual void StartApplication() override;
	virtual void StopApplication() override;
	void RecvPacket(Ptr<Socket> socket);
    void DeliveryData();
    void SendToNetwork(Ptr<Packet> p);
    TunDevice *tun_;
    uint32_t src_ip_;
    Ipv4Address peer_ip_;
    bool first_=false;
    uint16_t peer_port_=0;
    uint16_t bind_port_;
    Ptr<Socket> socket_;
    uint32_t context_=0;
    mutable Mutex mutex_;
    std::string buffer_;
    std::atomic<bool> in_delivery_{false};
    bool running_=true;    
};
}
