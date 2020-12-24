#include "ns3/emu-tun-module.h"
#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include <string>
#include <signal.h>
#include <memory.h>
#include <algorithm>
#include <iostream>
#include <vector>
using namespace ns3;
bool StartWith(std::string &a,std::string &b){
    bool ret=false;
    size_t size=std::min(a.size(),b.size());
    if((size>0)&&(memcmp(a.data(),b.data(),size)==0)){
        ret=true;
    }
    return ret;
}
static NodeContainer BuildExampleTopo (uint64_t bps,
                                       uint32_t msDelay,
                                       uint32_t msQdelay,
                                       uint32_t mtu=1500)
{
    NodeContainer nodes;
    nodes.Create (2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", DataRateValue  (DataRate (bps)));
    pointToPoint.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (msDelay)));
    pointToPoint.SetQueue ("ns3::DropTailQueue",
                           "MaxSize", StringValue (std::to_string(10)+"p"));   
    NetDeviceContainer devices = pointToPoint.Install (nodes);

    InternetStackHelper stack;
    stack.Install (nodes);
    
    auto bufSize = std::max<uint32_t> (mtu, bps * msQdelay / 8000);
    int packets=bufSize/mtu;
    TrafficControlHelper pfifoHelper;
    uint16_t handle = pfifoHelper.SetRootQueueDisc ("ns3::FifoQueueDisc", "MaxSize", StringValue (std::to_string(packets)+"p"));
    pfifoHelper.AddInternalQueues (handle, 1, "ns3::DropTailQueue", "MaxSize",StringValue (std::to_string(packets)+"p"));
    pfifoHelper.Install(devices);
    
    Ipv4AddressHelper address;
    std::string nodeip="10.1.1.0";
    address.SetBase (nodeip.c_str(), "255.255.255.0");
    address.Assign (devices);
    return nodes;
}
void InstallApplication(Ptr<Node> sender,
                        Ptr<Node> receiver,
                        uint16_t send_port,
                        uint16_t recv_port,
                        std::string &real_ip1,
                        std::string &real_ip2,
                        TunDevice* tun1,
                        TunDevice* tun2,
                        float startTime,
                        float stopTime)
{
    Ptr<EmuUdpSender> sendApp = CreateObject<EmuUdpSender> (tun1,real_ip1);
	Ptr<EmuUdpSink> recvApp = CreateObject<EmuUdpSink>(tun2,real_ip2);
   	sender->AddApplication (sendApp);
    receiver->AddApplication (recvApp);
    Ptr<Ipv4> ipv4 = receiver->GetObject<Ipv4>();
	Ipv4Address receiverIp = ipv4->GetAddress (1, 0).GetLocal();
	sendApp->Bind(send_port);
	recvApp->Bind(recv_port);

	sendApp->ConfigurePeer(receiverIp,recv_port);
    sendApp->SetStartTime (Seconds (startTime));
    sendApp->SetStopTime (Seconds (stopTime));
    recvApp->SetStartTime (Seconds (startTime));
    recvApp->SetStopTime (Seconds (stopTime));	
}
static double simDuration=40;
float appStart=0.0;
float appStop=simDuration-1;
static volatile bool virdev_closed=true;
std::string ethName1;
std::string ethName2;
void close_vir_device(){
    if(!virdev_closed){
        device_down(ethName1.c_str());
        device_down(ethName2.c_str());
        virdev_closed=true;
    }
}
void signal_exit_handler(int sig)
{
    close_vir_device();
}
/*
ping -I 192.168.24.1 192.168.26.1

iperf3 -s -B 192.168.27.1
iperf3 -B 192.168.24.1 -c 192.168.26.1 -i 1 -t 20 
*/
int main(){
    signal(SIGTERM, signal_exit_handler);
    signal(SIGINT, signal_exit_handler);
    signal(SIGTSTP, signal_exit_handler); 
    GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
    LogComponentEnable("tun-interface",LOG_LEVEL_ALL); 
    LogComponentEnable("emu-udp",LOG_LEVEL_ALL);    
    EpollThread epoll_thead;
    int tun_id=0;
    std::string tun_ip0("10.0.1.0");
    std::string tun_ip1("10.0.2.0");

    std::string real_ip1("192.168.24.1");
    std::string black_ip1("192.168.23.1");
    std::string real_ip2("192.168.27.1");
    std::string black_ip2("192.168.26.1");


    TunServer tun0(epoll_thead.epoll_server(),tun_id);
    tun_id++;
    bool ret=tun0.Init(tun_ip0.data(),tun_ip0.size());
    if(!ret){
        std::cout<<"tun0 init failed"<<std::endl;
        return 0;
    }

    TunServer tun1(epoll_thead.epoll_server(),tun_id);
    tun_id++;
    ret=tun1.Init(tun_ip1.data(),tun_ip1.size());
    if(!ret){
        std::cout<<"tun1 init failed"<<std::endl;
        return 0;
    }

    //real_ip1---send-data -->black_ip2    
    std::string tun_dev1=tun0.Name();
    configure_route(ROUTE_ADD,tun_dev1.c_str(),black_ip2.c_str(),32);

    std::string tun_dev2=tun1.Name(); 
    configure_route(ROUTE_ADD,tun_dev2.c_str(),black_ip1.c_str(),32);

   // std::vector<std::string> eths;
   // get_eth_name(eths);
    //int n=eths.size();
    //std::string tun_com("tun");
    std::string eth_name("enp0s25"); 
  
    int virEth=0;
    std::string colon=std::string(":");
    ethName1=eth_name+colon+std::to_string(virEth);
    virEth++;
    create_vnic(ethName1.c_str(),real_ip1.c_str());
    ethName2=eth_name+colon+std::to_string(virEth);
    virEth++;
    create_vnic(ethName2.c_str(),real_ip2.c_str());
    virdev_closed=false;
    uint64_t linkBw   = 5000000;
    uint32_t msDelay  = 100;
    uint32_t msQDelay = 200;
    uint16_t send_port=1234;
	uint16_t recv_port=4321;
    NodeContainer nodes = BuildExampleTopo (linkBw, msDelay, msQDelay);
    InstallApplication(nodes.Get(0), nodes.Get(1),send_port,recv_port,
                       real_ip1,real_ip2,&tun0,&tun1,appStart,appStop);
    epoll_thead.Start();
    Simulator::Stop (Seconds(simDuration));
    Simulator::Run ();
    Simulator::Destroy();
    configure_route(ROUTE_DEL,tun_dev1.c_str(),black_ip2.c_str(),32);
    configure_route(ROUTE_DEL,tun_dev2.c_str(),black_ip1.c_str(),32);
    epoll_thead.PostTask([&]{
        tun0.CloseFd();
        tun1.CloseFd();
    });    

    close_vir_device();
    epoll_thead.Stop();
    return 0;
}
