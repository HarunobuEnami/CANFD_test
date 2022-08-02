#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include "terminal.h"
#include "lib.h"
#include"aes-omac1.h"

int recieve_fd(struct sockaddr_can *addr, struct canfd_frame *frame, struct ifreq *ifr,const char *ifname);

int main(void)
{
    struct sockaddr_can addr;
	  struct canfd_frame frame;
    struct ifreq ifr;
    const char *ifname = "can1";
    strcpy(ifr.ifr_name, ifname);  
     printf("%d\n",recieve_fd(&addr,&frame,&ifr,ifname));
     
    return 0;
   
}

int recieve_fd(struct sockaddr_can *addr, struct canfd_frame *frame, struct ifreq *ifr,const char *ifname)
{
    unsigned char key[AES_BLOCK_SIZE];
    unsigned char MAC[AES_BLOCK_SIZE];
    int p_size=4;
    int s;
    int i;
    int mtu;
    int enable_canfd = 1;
    const int dropmonitor_on = 1;
    int nbytes;
    uint8_t plain [4];
    socklen_t len = sizeof(addr);
    can_err_mask_t err_mask = CAN_ERR_MASK;
    //struct msghdr msg;
    struct iovec iov = {
		.iov_base = frame,
	};
  struct msghdr msg = {
		.msg_iov = &iov,
	};
    strcpy(ifr->ifr_name, ifname);  
     ifr->ifr_ifindex = if_nametoindex(ifr->ifr_name);  
     if((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("Error while opening socket");
		return -1;
	}
  
  addr->can_family = AF_CAN;
  addr->can_ifindex = ifr->ifr_ifindex;
  mtu = ifr->ifr_mtu;

    if(setsockopt(s,SOL_CAN_RAW,CAN_RAW_FD_FRAMES,&enable_canfd,sizeof(enable_canfd)))
      {
       printf("error when enabling CAN FD support\n");
       return 1;
      }
  frame->len = can_fd_dlc2len(can_fd_len2dlc(frame->len));
  if (setsockopt(s, SOL_SOCKET, SO_RXQ_OVFL,
		       &dropmonitor_on, sizeof(dropmonitor_on)) < 0) {
		perror("setsockopt() SO_RXQ_OVFL not supported by your Linux Kernel");
	}

  if (setsockopt(s, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &err_mask, sizeof(err_mask))) {
		perror("setsockopt()");
		return 1;
	}
    if (bind(s, ((struct sockaddr *) addr), sizeof(*addr)) < 0) {
			perror("bind");
			return -1;
		}
    
    msg.msg_name = addr;
    msg.msg_flags = 0;
      nbytes = recvfrom(s, frame, sizeof(struct can_frame),0, (struct sockaddr*) addr, &len);
    for(i=0;i<p_size;++i)
    {
      plain[i] = frame->data[i];
    }
      printf("message : ");
    for(i=0;i<p_size;++i)
    {
      printf("%02x ",plain[i]);
    }
    
    printf("\nrecieved mac : ");
    for(i=p_size;i<8;++i)
    {
      printf("%02x ",frame->data[i]);
    }
    init_Rnd_cdm(0x01d0);
	for(int i=0;i<AES_BLOCK_SIZE;i++){
		key[i]=Rnd_byte();
	}
 // plain[2] = 0x11; //改ざん
  omac1_aes_128(key,plain,p_size,MAC);
     printf("\nchecked mac : ");
    for(i=p_size;i<8;++i)
    {
      printf("%02x ",MAC[i]);
    }
    putchar('\n');
    
    return nbytes;
}