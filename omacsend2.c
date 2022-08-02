#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <time.h>
#include<sys/time.h>

#include<net/if.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/ioctl.h>

#include<linux/can.h>
#include<linux/can/raw.h>
#include"aes-omac1.h"
#include"lib.h"

int main(void)
{
    int s;
	int nbytes;
    int mtu;
    int enable_canfd = 1;
	struct sockaddr_can addr;
	struct canfd_frame frame;
	struct ifreq ifr;
	struct timespec start_time, end_time;
	unsigned char key[AES_BLOCK_SIZE];
	unsigned char MAC[AES_BLOCK_SIZE];
	int p_size=4;
	int nsec;
	uint32_t seq=0;
	uint8_t plain []= {0x01,0x02,0x03,0x04};

	
	init_Rnd_cdm(0x01d0);
	for(int i=0;i<AES_BLOCK_SIZE;i++){
		key[i]=Rnd_byte();
	}
	
	printf("-------------------------------------------------\n");
	printf("key            ");
	for(int i=0;i<AES_BLOCK_SIZE;i++){
		printf("%x",key[i]);
		if((i%4) == 3)
			printf(" ");
	}
	printf("\n");
	const char *ifname = "can0";
    strcpy(ifr.ifr_name, ifname);
    ifr.ifr_ifindex = if_nametoindex(ifr.ifr_name);
	if((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("Error while opening socket");
		return -1;
	}

    if (ioctl(s, SIOCGIFMTU, &ifr) < 0) {
			perror("SIOCGIFMTU");
			return 1;
		}
		mtu = ifr.ifr_mtu;

		if (mtu != CANFD_MTU) {
			printf("CAN interface is not CAN FD capable - sorry.\n");
			return 1;
		}

    if (setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES,
			       &enable_canfd, sizeof(enable_canfd))){
			printf("error when enabling CAN FD support\n");
			return 1;
		}
    
    frame.len = can_fd_dlc2len(can_fd_len2dlc(frame.len));
        setsockopt(s, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0);

	strcpy(ifr.ifr_name, ifname);
	ioctl(s, SIOCGIFINDEX, &ifr);

	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	printf("%s at index %d\n", ifname, ifr.ifr_ifindex);
	if(bind(s,(struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Error in socket bind");
		return -2;
	}

	frame.can_id = 0x01;
	frame.len = 0x0c;
	frame.data[0] = plain[0];
	frame.data[1] = plain[1];
	frame.data[2] = plain[2];
	frame.data[3] = plain[3];
	clock_gettime(CLOCK_REALTIME, &start_time);
    omac1_aes_128(key,plain,p_size,MAC);//cal
	clock_gettime(CLOCK_REALTIME, &end_time);
	nsec = end_time.tv_nsec - start_time.tv_nsec;
	printf("nsec : %d\n",nsec);
	printf("MAC            ");
	for(int i=0;i<AES_BLOCK_SIZE;i++){
		printf("%x",MAC[i]);
		if((i%4) == 3)
			printf(" ");
	}
	
	printf("\n");
	int i;//insert
	for(i=p_size;i<8;i++){
		frame.data[i] = MAC[i]; //frame[0]~[3]にメッセージをframe[4]~[7]にmacの[4]~[7]を載せる
	}
	++seq;
	frame.data[8] = 0x000000ff&seq;
	frame.data[9] = ((0x0000ff00&seq)>>8) ;
	frame.data[10] = ((0x00ff0000&seq)>>12) ;
	frame.data[11] = ((0xff000000&seq)>>16) ;
	

	nbytes =write(s, &frame, CANFD_MTU);
    printf("%d\n",nbytes);
	printf("Wrote %d bytes\n", nbytes);
	for(i=0;i<8;i++){
	printf("frame.data[%d] = %02x\n",i,frame.data[i]);
	}
	
	printf("-------------------------------------------------\n");
	if(frame.data[7] >= 0xf1){
		for(int i=0;i<AES_BLOCK_SIZE;i++){
			key[i] = key[i] ^  Rnd_byte();
		}
	}
		// usleep(200000);
}