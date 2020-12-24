# Introduction
run ns3 in emulation mode with help of tun device  
A similar project [ns3-emulation](https://github.com/SoonyangZhang/ns3-emulation).  
Two tun devices are used to make the project to run on a single machine.    
```
                      10.10.0.0           
                  n0---------------n1       
                tun0                tun1        
                /                     \         
            host1(192.168.24.1)        host2(192.168.27.1)  
     BlackHost1(192.168.23.1)        BlackHost2(192.168.26.1)  
```
Host1 will send packet to BlackHost2(whose ip is not exist).  
In TunInterface.cc, the src and dst addresses will be changed. 
host1---->BlackHost2 will be changed as:  
BlackHost1---->host2.   
```
    header->srcaddr=GetBlackAddrFromRealIp(from);   
    header->dstaddr=GetRealIpfromBlackAddr(to);            
    header->check=0;   
    uint16_t check=0;    
    check=cksum_generic((const char*)header,20,0);   
    header->check=check;    
    check=cksum_generic((const char*)header,20,0);    
    NS_ASSERT(check==0);   
    endpoint->OnPacket(data,size);    
```
# Build
Code is tested on ns3.31. To test it on older version will be ok.  
1 check the eth0 name of your PC and change the variable in tun-test.cc.  
```
std::string eth_name("enp0s25");  
```
2 put the emu-tun under src in ns3. Copy tun-test.cc to scratch.  
3 Add environment variable to /etc/profile   
```
sudo su  
gedit /etc/profile   
export EMU_TUN_INC=/xxx/xxx/ns-allinone-3.31/ns-3.31/src/emu-tun/model/  
export CPLUS_INCLUDE_PATH=CPLUS_INCLUDE_PATH:$EMU_TUN_INC  
```
4 build it on ns3.31     
```
sudo su  
source /etc/profile
cd  /ns-allinone-3.31/  
./build.py  
```
5 Run  
```
cd ns-3.31  
./waf --run scratch/tun-test   
```
6 Test the speed of tcp with iperf.  
shell1  
```
iperf3 -B 192.168.24.1 -c 192.168.26.1 -i 1 -t 20    
``` 
shell2   
```
iperf3 -s -B 192.168.27.1   
```





