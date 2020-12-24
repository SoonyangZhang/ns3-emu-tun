#pragma once
#include "third_party/epoll/simple_epoll_server.h"
namespace ns3{
using EpollServer=epoll_server::SimpleEpollServer;
using EpollEvent=epoll_server::EpollEvent;    
}
