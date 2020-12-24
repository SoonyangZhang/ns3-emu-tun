#include "tun_util.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/if.h>
#include <string.h>
namespace ns3{
uint16_t cksum_generic(const char *p, size_t len, uint16_t initial){
  uint32_t sum = htons(initial);
  const uint16_t *u16 = (const uint16_t *)p;

  while (len >= (sizeof(*u16) * 4)) {
    sum += u16[0];
    sum += u16[1];
    sum += u16[2];
    sum += u16[3];
    len -= sizeof(*u16) * 4;
    u16 += 4;
  }
  while (len >= sizeof(*u16)) {
    sum += *u16;
    len -= sizeof(*u16);
    u16 += 1;
  }
  /* if length is in odd bytes */
  if (len == 1)
    sum += *((const uint8_t *)u16);

  while(sum>>16)
    sum = (sum & 0xFFFF) + (sum>>16);
  return /*ntohs*/((uint16_t)~sum);    
}
//https://github.com/adrienverge/openfortivpn/blob/master/src/ipv4.c
int configure_route(RouteCommand add,const char* dev,const char *dst,int masklen){
    char cmd[128]; 
    uint32_t ip;
    ::inet_pton(AF_INET,dst,(void*)&ip);
    ip=ntohl(ip); 
    uint32_t mask = 0xffffffff << (32-masklen);  
    ip=(ip)&mask;
    ip=htonl(ip);
    char ip4[20]={0};
    ::inet_ntop(AF_INET, (void *)&ip, ip4, 16);   
    if(add==ROUTE_ADD){
        sprintf(cmd, "ip route add %s/%d dev %s", ip4, masklen, dev);
    }else{
        sprintf(cmd, "ip route del %s/%d dev %s", ip4, masklen, dev);
    }
    
    int res=system(cmd);
    if ( res< 0){
       return -1;
    }
    return 0;
}
//sudo ifconfig eth0:0 192.168.23.1 up
void create_vnic(const char*dev,const char *ip){
    char net_prefix_cmd[128];
    sprintf(net_prefix_cmd, "ifconfig %s %s up", dev,ip);
   if (system(net_prefix_cmd) < 0){
       perror("vdev failed");
   }
}
int device_down(const char* dev){
    int ret=-1;
    char net_prefix_cmd[128];
    sprintf(net_prefix_cmd, "ifconfig %s down",dev);
    ret=system(net_prefix_cmd);
    return ret;
}
int get_eth_name(std::vector<std::string> &eth){
    int ret=-1;
        int fd;
    int interfaceNum = 0;
    struct ifreq buf[16];
    struct ifconf ifc;
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        close(fd);
        return ret;
    }
    
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;
    if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc))
    {
        interfaceNum = ifc.ifc_len / sizeof(struct ifreq);
        while (interfaceNum-- > 0)
        {
            if(strcmp(buf[interfaceNum].ifr_name,"lo")==0){
                continue;
            }
            std::string name(buf[interfaceNum].ifr_name);
            eth.push_back(name);
        }
    }
    ret=0;
    close(fd);
    return ret;
}
}
