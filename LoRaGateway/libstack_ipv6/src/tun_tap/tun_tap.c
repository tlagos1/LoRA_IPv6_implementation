
#define _BSD_SOURCE

#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

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
    command = "ip -6 addr add fe80:0000:0000:0000::1/64 dev tun0";
    system(command);

    strcpy(tun_name, "tun0");

    tun_fd = tun_alloc(tun_name, IFF_TUN | IFF_NO_PI);  /* tun interface, no Ethernet headers*/
    
    if(tun_fd < 0){
        return -1;
    }

    return tun_fd;
}