#pragma once
#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <string>
#include <arpa/inet.h> 
namespace ns3{
enum RouteCommand:int{
    ROUTE_ADD,
    ROUTE_DEL,
};
struct ipv4_header
{
#if defined(__LITTLE_ENDIAN)
uint8_t ihl:4,
	version:4;
#elif defined(__BIG_ENDIAN)
uint8_t version:4,
	ihl:4;
#else
# error "Please fix endianness defines"
#endif
    uint8_t  tos;                /**< type of service */
    uint16_t length;             /**< total length */
    uint16_t id;                 /**< identification */
    uint16_t offset;             /**< fragment offset field */
    uint8_t  ttl;                /**< time to live */
    uint8_t  protocol;           /**< protocol */
    uint16_t check;           /**< checksum */
    uint32_t srcaddr;           /**< source address */
    uint32_t dstaddr;           /**< destination address */
};
uint16_t cksum_generic(const char *p, size_t len, uint16_t initial);
int configure_route(RouteCommand add,const char* dev,const char *dst,int masklen);
void create_vnic(const char*dev,const char *ip);
int device_down(const char* dev);
int get_eth_name(std::vector<std::string> &eth);
}