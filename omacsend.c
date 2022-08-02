#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include<string.h>
#include "lib.h"
#include "aes-omac1.h"
#define MESSAGE "1234567890ABCDEF"
#define MESSAGE_SIZE 8 
#define KEY "1234567890ABCDEF1234567890ABCDEF"
typedef unsigned char u8;


int main(void)
{
  int s; //can raw socket
  int mtu;
  int required_mtu;
  int enable_canfd = 1;
  char message_mac[512];
  char buff[32]; 
  char buff2[512];
  char key[35];
  int i;
  struct sockaddr_can addr;
  struct canfd_frame frame;
  struct ifreq ifr;
  u8 mac[16];
  sprintf(message_mac,"%s","123##0");
  sprintf(key,"%s",KEY);
  sprintf(buff2,"%s",MESSAGE);
  omac1_aes_128((u8 *)key,(u8 *)buff2,8,mac);
    for(i=0;i<8;++i)
    {    
        sprintf(buff,"%02x",mac[i]);
        strcat(message_mac,buff);
    }
    strcat(message_mac,buff2);
    puts(message_mac);
  
 
  required_mtu = parse_canframe(message_mac,&frame);
    if (!required_mtu){
        fprintf(stderr, "\nWrong CAN-frame format!\n\n");
        //print_usage(argv[0]);
        return 1;
    }
    
  if((s = socket(PF_CAN,SOCK_RAW,CAN_RAW)) < 0){
    perror("socket");
    return 1;
  }
  //ifr.ifr_name = 0x12345678;

   strcpy(ifr.ifr_name, "can0");
       ifr.ifr_ifindex = if_nametoindex(ifr.ifr_name);
       if (!ifr.ifr_ifindex) {
           perror("if_nametoindex");
           return 1;
       }

       memset(&addr,0, sizeof(addr));
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;
  mtu = ifr.ifr_mtu;

    if(setsockopt(s,SOL_CAN_RAW,CAN_RAW_FD_FRAMES,&enable_canfd,sizeof(enable_canfd)))
      {
       printf("error when enabling CAN FD support\n");
       return 1;
      }
  frame.len = can_fd_dlc2len(can_fd_len2dlc(frame.len));

  setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0);

  if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }
   if (write(s, &frame, required_mtu) != required_mtu) {
        perror("write");
        return 1;
    }

    close(s);
  
  return 0;
}
