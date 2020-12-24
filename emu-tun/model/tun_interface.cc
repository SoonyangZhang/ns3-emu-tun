#include "ns3/log.h"
#include "ns3/core-module.h"
#include <memory.h>
#include "tun_interface.h"
#include "tun_util.h"
#include "net_endian.h"
#include "ip_address.h"
namespace ns3{
NS_LOG_COMPONENT_DEFINE("tun-interface");
uint32_t kIpAddressOffset=0x00000100;
uint32_t kIpAddressOffsetNetOrder=0;
void tun_parameter_initial(){
    kIpAddressOffsetNetOrder=net::QuicheEndian::HostToNet32(kIpAddressOffset);
}
inline uint32_t GetBlackAddrFromRealIp(uint32_t &real){
    uint32_t fake=real;
    fake-=kIpAddressOffsetNetOrder;
    return fake;
}
inline uint32_t GetRealIpfromBlackAddr(uint32_t &fake){
    uint32_t real=fake;
    real+=kIpAddressOffsetNetOrder;
    return real;
}
PacketDispatcher *PacketDispatcher::Instance(){
    static PacketDispatcher  *const ins=new PacketDispatcher();
    return ins;
}
PacketDispatcher::PacketDispatcher(){
    tun_parameter_initial();
}
bool PacketDispatcher::SubscribePacket(uint32_t src,NsEndpoint *sink){
    bool insert=false;
    {
        in_addr temp;
        memcpy(&temp,&src,sizeof(int));
        net::IpAddress ip(temp);               
    }    
    MutexLock lock(&mutex_);
    auto it=sinks_.find(src);
    if(it==sinks_.end()){
        sinks_.insert(std::make_pair(src,sink));
        insert=true;
    }
    return insert;
}
void PacketDispatcher::UnSubscribe(uint32_t src,NsEndpoint *sink){
    MutexLock lock(&mutex_);
    auto it=sinks_.find(src);
    if(it!=sinks_.end()){
        sinks_.erase(it);
    }
}
void PacketDispatcher::OnPacket(char*data,size_t size){
    struct ipv4_header *header=(struct ipv4_header*)data;
    if(size>=sizeof(struct ipv4_header)){
        uint32_t from=header->srcaddr;
        uint32_t to=header->dstaddr;
        NsEndpoint *endpoint=nullptr;
        {
            MutexLock lock(&mutex_);
            auto it=sinks_.find(from);
            if(it!=sinks_.end()){
                endpoint=it->second;
            }
        }
        if(endpoint){
            header->srcaddr=GetBlackAddrFromRealIp(from);
            header->dstaddr=GetRealIpfromBlackAddr(to);            
            header->check=0;
            uint16_t check=0;
            check=cksum_generic((const char*)header,20,0);
            header->check=check;
            check=cksum_generic((const char*)header,20,0);
            NS_ASSERT(check==0);
            endpoint->OnPacket(data,size);
        }
    }
}
}
