#include "tun_server.h"
#include "third_party/logging/logging.h"
#include <errno.h>
#include <string.h>
#include <sys/epoll.h> 
#include <unistd.h>
#include <error.h>
#include <stdio.h>
//for inet_pton  htonl
#include <sys/socket.h>
#include <netinet/in.h>
#include<arpa/inet.h>
#include "tun_interface.h"
namespace ns3{
namespace {
//const int kEpollFlags = EPOLLIN| EPOLLET;
const int kEpollFlags = EPOLLIN;
const int kBufLen=1500;
}  // namespace
TunServer::TunServer(EpollServer* epoll_server,int tun_id,int mtu):
eps_(epoll_server),
tun_id_(tun_id),
mtu_(mtu){
    tun_device_=tuntap_init();    
} 
TunServer::~TunServer() {
    //CloseFd();
}
void TunServer::CloseFd(){
    if(eps_&&tun_device_){
        eps_->UnregisterFD((int)tuntap_get_fd(tun_device_));
    }
    if(interface_up_){
        tuntap_down(tun_device_);
                
    }
    if(tun_device_){
        tuntap_destroy(tun_device_);
        tun_device_=nullptr;
    }
    interface_up_=false;

}
bool TunServer::Init(const char *ip,int len){
    if(!tun_device_||interface_up_){
        return false;
    }
    if(tuntap_start(tun_device_,TUNTAP_MODE_TUNNEL,tun_id_)==-1){
        return false;
    }
    if(tuntap_up(tun_device_)==-1){
        return false;
    }
    if (tuntap_set_ip(tun_device_, ip, len) == -1) {
        return false;
    }
    interface_up_=true;
    RegisterEvent();
    return true;
}
std::string TunServer::Name() const{
    return "tun"+std::to_string(tun_id_);
}
void TunServer::OnRegistration(EpollServer* eps, int fd, int event_mask){}
void TunServer::OnModification(int fd, int event_mask){}
void TunServer::OnUnregistration(int fd, bool replaced){}
void TunServer::OnShutdown(EpollServer* eps, int fd){}
void TunServer::OnEvent(int fd, EpollEvent* event){
    if(event->in_events & EPOLLIN){
        ReadFromTun();
    }
}
void TunServer::RegisterEvent(){
    if(tun_device_){
        int fd=(int)tuntap_get_fd(tun_device_);
        eps_->RegisterFD(fd, this, kEpollFlags); 
    }
    
     
}
void TunServer::ReadFromTun(){
    char buffer[kBufLen];
    if(true){
        size_t ret=read((int)tuntap_get_fd(tun_device_),buffer,kBufLen);        
        /*if(ret<=0){
            if(errno == EWOULDBLOCK || errno == EAGAIN){
                //read until no dada
            }else{
                LOG(ERROR)<<"Tun Read Error";
            }
            break;
        }else*/{          
            if(ret>=20){
                PacketDispatcher *ins=PacketDispatcher::Instance();
                ins->OnPacket(buffer,ret);
            }
        }
        
    }
}
void TunServer::SendToDevice(const char *data,size_t size){
    if(interface_up_){
       int tun_fd=(int)tuntap_get_fd(tun_device_);
       ::write(tun_fd,data,size);       
    }   
}
}
