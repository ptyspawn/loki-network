#ifndef LLARP_NET_NET_IF_HPP
#define LLARP_NET_NET_IF_HPP
#ifndef _WIN32
// this file is a shim include for #include <net/if.h>
#if defined(__linux__)
#include <linux/if.h>
extern "C" unsigned int
#ifndef __GLIBC__
if_nametoindex(const char* __ifname);
#else
if_nametoindex(const char* __ifname) __THROW;
#endif
#else
#include <net/if.h>
#endif
#endif
#endif
