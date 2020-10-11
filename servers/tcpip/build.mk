name := tcpip
description := A TCP/IP server
objs-y := main.o arp.o device.o dhcp.o ethernet.o ipv4.o mbuf.o tcp.o udp.o \
	stats.o icmp.o dns.o
