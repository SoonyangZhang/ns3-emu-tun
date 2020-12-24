#pragma  once
#include "third_party/tuntap/tuntap.h"
#include "ns3/tun_interface.h"
#include "ns3/epoll_api.h"
namespace ns3{
class TunServer:public epoll_server::EpollCallbackInterface,
public TunDevice{
public:
    TunServer(EpollServer* epoll_server,int tun_id=0,int mtu=1500);
    ~TunServer();
    bool Init(const char *ip,int len);
    bool IsInterfaceUp() const {return interface_up_;}  
    std::string Name() const override;
    // From EpollCallbackInterface
    void OnRegistration(EpollServer* eps, int fd, int event_mask) override;
    void OnModification(int fd, int event_mask) override;
    void OnEvent(int fd, EpollEvent* event) override;
    // |fd_| can be unregistered without the client being disconnected. This
    // happens in b3m QuicProber where we unregister |fd_| to feed in events to
    // the client from the SelectServer.
    void OnUnregistration(int fd, bool replaced) override;
    void OnShutdown(EpollServer* eps, int fd) override; 
    //TunDevice
    void SendToDevice(const char *data,size_t size) override;
    void CloseFd();
private:
    void RegisterEvent();
    void ReadFromTun();
    EpollServer *eps_;
    int tun_id_;
    int mtu_;
    struct device *tun_device_=nullptr;
    bool interface_up_=false;
};    
}
