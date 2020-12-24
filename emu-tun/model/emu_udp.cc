#include "emu_udp.h"
#include "ns3/log.h"
#include "tun_util.h"
#include "net_endian.h"
#include "ip_address.h"
#include <memory.h>
namespace ns3{
const int kBufferSize=2048;
NS_LOG_COMPONENT_DEFINE("emu-udp");
EmuUdpSender::EmuUdpSender(TunDevice*tun,std::string &real_src):
tun_(tun){
    net::IpAddress ip_addr;
    ip_addr.FromString(real_src);    
    in_addr temp=ip_addr.GetIPv4();
    memcpy((void*)&src_ip_,(void*)&temp,sizeof(uint32_t));
    PacketDispatcher *ins=PacketDispatcher::Instance();
    ins->SubscribePacket(src_ip_,this);
}
void EmuUdpSender::Bind(uint16_t port){
    if(socket_==nullptr){
        socket_=Socket::CreateSocket (GetNode (),UdpSocketFactory::GetTypeId ());
        auto local = InetSocketAddress{Ipv4Address::GetAny (), port};
        auto res =socket_->Bind (local);
        NS_ASSERT (res == 0);        
    }
    bind_port_=port;
    socket_->SetRecvCallback (MakeCallback(&EmuUdpSender::RecvPacket,this));
    context_=GetNode()->GetId();
}
void EmuUdpSender::ConfigurePeer(Ipv4Address addr,uint16_t port){
    if(peer_port_==0){
        peer_ip_=addr;
        peer_port_=port;
    }
}
void EmuUdpSender::OnPacket(const char*data,size_t size){
    if(!running_){return ;}
    {
        MutexLock lock(&mutex_);
        size_t old_size=buffer_.size();
        buffer_.resize(old_size+size);
        memcpy(&buffer_[old_size],data,size);        
    }
    if(!in_delivery_){
        in_delivery_=true;
        Simulator::ScheduleWithContext(context_,Time (0),
                                        MakeEvent(&EmuUdpSender::DeliveryData, this));
    }
}
void EmuUdpSender::StartApplication(){}
void EmuUdpSender::StopApplication(){
    running_=false;
    PacketDispatcher *ins=PacketDispatcher::Instance();
    ins->UnSubscribe(src_ip_,this);    
}
void EmuUdpSender::RecvPacket(Ptr<Socket> socket){
	Address remoteAddr;
	auto packet = socket->RecvFrom (remoteAddr);
    int recv=packet->GetSize ();
    if(tun_&&running_){
        uint8_t buffer[kBufferSize];
        NS_ASSERT(kBufferSize>=recv);
        packet->CopyData(buffer,recv);
        tun_->SendToDevice((const char*)buffer,recv);
    }
}
void EmuUdpSender::DeliveryData(){
    std::string copy;
    {
         MutexLock lock(&mutex_);
         copy.swap(buffer_);
    }
    in_delivery_=false;
    size_t remain=copy.size();
    const uint8_t *data=(const uint8_t *)copy.data();
    while(remain>0){
        const struct ipv4_header *header=(const struct ipv4_header*)data;
        uint16_t len=net::QuicheEndian::NetToHost16(header->length);
        Ptr<Packet> p=Create<Packet>(data,len);
        NS_ASSERT(remain>=len);
        remain-=len;
        data+=len;
        SendToNetwork(p);
    }
}
void EmuUdpSender::SendToNetwork(Ptr<Packet> p){
    socket_->SendTo(p,0,InetSocketAddress{peer_ip_,peer_port_});
}


EmuUdpSink::EmuUdpSink(TunDevice*tun,std::string &real_src):
tun_(tun){
    net::IpAddress ip_addr;
    ip_addr.FromString(real_src);    
    in_addr temp=ip_addr.GetIPv4();
    memcpy((void*)&src_ip_,(void*)&temp,sizeof(uint32_t));
    PacketDispatcher *ins=PacketDispatcher::Instance();
    ins->SubscribePacket(src_ip_,this);
}
void EmuUdpSink::Bind(uint16_t port){
    if(socket_==nullptr){
        socket_=Socket::CreateSocket (GetNode (),UdpSocketFactory::GetTypeId ());
        auto local = InetSocketAddress{Ipv4Address::GetAny (), port};
        auto res =socket_->Bind (local);
        NS_ASSERT (res == 0);        
    }
    bind_port_=port;
    socket_->SetRecvCallback (MakeCallback(&EmuUdpSink::RecvPacket,this));
    context_=GetNode()->GetId();
}
InetSocketAddress EmuUdpSink::GetLocalAddress(){
    Ptr<Node> node=GetNode();
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
    Ipv4Address local_ip = ipv4->GetAddress (1, 0).GetLocal ();
	return InetSocketAddress{local_ip,bind_port_};    
}
void EmuUdpSink::OnPacket(const char*data,size_t size){
    if(!running_){return ;}
    {
        MutexLock lock(&mutex_);
        size_t old_size=buffer_.size();
        buffer_.resize(old_size+size);
        memcpy(&buffer_[old_size],data,size);        
    }
    if(!in_delivery_){
        in_delivery_=true;
        Simulator::ScheduleWithContext(context_,Time (0),
                                        MakeEvent(&EmuUdpSink::DeliveryData, this));
    }
}
void EmuUdpSink::StartApplication(){}
void EmuUdpSink::StopApplication(){
    running_=false;
    PacketDispatcher *ins=PacketDispatcher::Instance();
    ins->UnSubscribe(src_ip_,this);    
}
void EmuUdpSink::RecvPacket(Ptr<Socket> socket){
	Address remoteAddr;
	auto packet = socket->RecvFrom (remoteAddr);
    int recv=packet->GetSize ();
    if(!first_){
		peer_ip_= InetSocketAddress::ConvertFrom (remoteAddr).GetIpv4 ();
		peer_port_= InetSocketAddress::ConvertFrom (remoteAddr).GetPort ();        
        first_=true;
    }
    if(tun_&&running_){
        uint8_t buffer[kBufferSize];
        NS_ASSERT(kBufferSize>=recv);
        packet->CopyData(buffer,recv);
        tun_->SendToDevice((const char*)buffer,recv);
    }
}
void EmuUdpSink::DeliveryData(){
    std::string copy;
    {
         MutexLock lock(&mutex_);
         copy.swap(buffer_);
    }
    in_delivery_=false;
    size_t remain=copy.size();
    const uint8_t *data=(const uint8_t *)copy.data();
    while(remain>0){
        const struct ipv4_header *header=(const struct ipv4_header*)data;
        uint16_t len=net::QuicheEndian::NetToHost16(header->length);
        Ptr<Packet> p=Create<Packet>(data,len);
        NS_ASSERT(remain>=len);
        remain-=len;
        data+=len;
        SendToNetwork(p);
    }
}
void EmuUdpSink::SendToNetwork(Ptr<Packet> p){
    socket_->SendTo(p,0,InetSocketAddress{peer_ip_,peer_port_});
}
}
