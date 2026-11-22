
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

int create_can_socket(const char *interface)
{
    int sock;
    struct ifreq ifr;
    struct sockaddr_can addr;

    sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);

    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("Interface lookup failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Socket bind failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    return sock;
}

int main(void)
{
    int rx_sock, tx_sock;
    struct can_frame frame;

    rx_sock = create_can_socket("vcan0");
    tx_sock = create_can_socket("vcan1");

    printf("Gateway ECU started\n");
    printf("Receiving from vcan0\n");
    printf("Forwarding allowed messages to vcan1\n\n");

    while (1) {
        int n = read(rx_sock, &frame, sizeof(frame));

        if (n < 0) {
            if (errno == EINTR)
                continue;

            perror("CAN read failed");
            break;
        }

        if (n != sizeof(frame))
            continue;

        if (frame.can_id & (CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_ERR_FLAG))
            continue;

        uint32_t can_id = frame.can_id & CAN_SFF_MASK;

        if ((can_id == 0x100 && frame.can_dlc == 3) ||
            (can_id == 0x200 && frame.can_dlc == 4)) {

            if (write(tx_sock, &frame, sizeof(frame)) < 0) {
                perror("CAN forwarding failed");
                continue;
            }

            printf("FORWARDED | ID: 0x%03X | DLC: %d\n",
                   can_id, frame.can_dlc);
        } else {
            printf("DROPPED | ID: 0x%03X | DLC: %d\n",
                   can_id, frame.can_dlc);
        }

        fflush(stdout);
    }

    close(rx_sock);
    close(tx_sock);

    return 0;
}
