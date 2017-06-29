
#define _BSD_SOURCE

#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include <net/if.h>
#include <linux/if_packet.h>
#include <sys/socket.h>
#include <netinet/ether.h>

#include "IPv6.h"
#include "tun_tap.h"


int tun_alloc(char *dev, int flags) {

  struct ifreq ifr;
  int fd, err;
  char const *clonedev = "/dev/net/tun";

  if( (fd = open(clonedev , O_RDWR)) < 0 ) {
    return fd;
  }

  memset(&ifr, 0, sizeof(ifr));

  ifr.ifr_flags = flags;

  if (*dev) {
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  }

  if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
    close(fd);
    return err;
  }

  strcpy(dev, ifr.ifr_name);

  return fd;
}

int init_socket()
{
    int sock_fd;
    
    if ( (sock_fd = socket(AF_PACKET,  SOCK_RAW, htons(ETH_P_ALL))) < 0) 
    {
      return(-1);
    }

    return sock_fd;
}

int cread(int fd, char *buf, int n){
  
  int nread;

  if((nread=read(fd, buf, n)) < 0){
    return -1;
  }
  return nread;
}

int read_n(int fd, char *buf, int n) {

  int nread, left = n;

  while(left > 0) 
  {
    nread = cread(fd, buf, left);
    if (nread != -1)
    {
      if (nread == 0)
      {
        return 0 ;      
      }
      else {
        left -= nread;
        buf += nread;
      }
    }
    else
    {
      return -1;
    }
  }
  return n;  
}

int init_tun()
{

    int tun_fd = 0;
    char tun_name[IFNAMSIZ];

    const char *command;
    command = "ip tuntap add dev tun0 mode tun user root";
    system(command);
    command = "ip link set tun0 up";
    system(command);
    command = "ip -6 addr add BBBB:0000:0000:0000::1/64 dev tun0";
    system(command);
    command = "ip -6 addr add fe80::1/64 dev tun0";
    system(command);

    strcpy(tun_name, "tun0");

    tun_fd = tun_alloc(tun_name, IFF_TUN | IFF_NO_PI);  /* tun interface, no Ethernet headers*/
    
    if(tun_fd < 0){
        return -1;
    }

    return tun_fd;
}

int send_raw_package(char *buffer ,uint8_t* mac_src, uint8_t* mac_dst, char *if_name, int net_fd)
{
    char buffer_to_send[1024]; 
    struct sockaddr_ll socket_address;
    struct ifreq if_idx;
    struct ifreq if_mac;
    struct ether_header *eh= (struct ether_header *) buffer_to_send;
    int tx_len = 0;
    int ix;
    int packet_size;
    char ifName[IFNAMSIZ];

    packet_size = (buffer[4] << 8 | buffer[5]) + 40;

    strcpy(ifName, if_name);

    memset(&if_idx, 0, sizeof(struct ifreq));

    strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
    if (ioctl(net_fd, SIOCGIFINDEX, &if_idx) < 0)
    {
      return(-1);
    }
    /* Get the MAC address of the interface to send on */
    memset(&if_mac, 0, sizeof(struct ifreq));
    strncpy(if_mac.ifr_name, ifName, IFNAMSIZ-1);
    if (ioctl(net_fd, SIOCGIFHWADDR, &if_mac) < 0)
    {
      return(-1);
    }

    memset(buffer_to_send, 0, 1024);
  
    eh->ether_shost[0] = mac_src[0];
    eh->ether_shost[1] = mac_src[1];
    eh->ether_shost[2] = mac_src[2];
    eh->ether_shost[3] = mac_src[3];
    eh->ether_shost[4] = mac_src[4];
    eh->ether_shost[5] = mac_src[5];
  
    eh->ether_dhost[0] = mac_dst[0];
    eh->ether_dhost[1] = mac_dst[1];
    eh->ether_dhost[2] = mac_dst[2];
    eh->ether_dhost[3] = mac_dst[3];
    eh->ether_dhost[4] = mac_dst[4];
    eh->ether_dhost[5] = mac_dst[5];
    /* Ethertype field */
    eh->ether_type = htons(ETH_P_IPV6);

    tx_len += sizeof(struct ether_header);

    socket_address.sll_ifindex = if_idx.ifr_ifindex;
    /* Address length*/
    socket_address.sll_halen = ETH_ALEN;

    for(ix = 0; ix < packet_size; ix++)
    {
        buffer_to_send[tx_len++] = buffer[ix];
    }

    socket_address.sll_addr[0] = mac_dst[0];
    socket_address.sll_addr[1] = mac_dst[1];
    socket_address.sll_addr[2] = mac_dst[2];
    socket_address.sll_addr[3] = mac_dst[3];
    socket_address.sll_addr[4] = mac_dst[4];
    socket_address.sll_addr[5] = mac_dst[5];

    if (sendto(net_fd, buffer_to_send, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
    {
        return (-1);    
    }
    else
    {
        return(1);
    }
}

char *recive_raw_package_by_interface(int net_fd, char *returned_buffer)
{
  char buffer[1024];
  int offset = 0;
  int payload_length;
  int plength;
  int ix;

  memset(buffer,0,1024);  
  plength = read(net_fd,buffer,sizeof(buffer)); 
  if(plength > 0)
  {
    offset += 12;
    if(buffer[offset] == 0x86 && buffer[offset+1] == 0xdd)
    { 
      offset += 2;
      
      memset(returned_buffer, 0, 1024); 
  
      payload_length = (buffer[offset+4] << 8 | buffer[offset+5]);

      for(ix = 0; ix < (payload_length+40); ix++)
      {
          returned_buffer[ix] = buffer[offset+ix];
      }
      return returned_buffer;
      
    }        
  }   
  return NULL;
}