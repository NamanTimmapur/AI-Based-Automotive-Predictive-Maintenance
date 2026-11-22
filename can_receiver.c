
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#include <sys/socket.h>
#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>

int main()
{
    int sock;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    // Create a raw CAN socket
    sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // Select CAN interface
    strcpy(ifr.ifr_name, "vcan0");
    ioctl(sock, SIOCGIFINDEX, &ifr);

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    // Bind socket to vcan0
    if (bind(sock, (struct sockaddr *)&addr,
             sizeof(addr)) < 0) {
        perror("Socket bind failed");
        close(sock);
        return 1;
    }

    printf("C-based CAN Receiver Started...\n");
    printf("Listening on vcan0\n");

    while (1) {

        int nbytes = read(sock, &frame, sizeof(frame));

        if (nbytes < 0) {
            perror("CAN read failed");
            break;
        }

        if (nbytes != sizeof(frame)) {
            printf("Incomplete CAN frame\n");
            continue;
        }

        // Extract standard CAN ID
        unsigned int can_id =
            frame.can_id & CAN_SFF_MASK;

        printf("\n-----------------------------\n");
        printf("CAN ID: 0x%03X\n", can_id);

        // Decode Engine ECU
        if (can_id == 0x100 && frame.can_dlc >= 3) {

            uint16_t rpm =
                ((uint16_t)frame.data[0] << 8)
                | frame.data[1];

            uint8_t temperature = frame.data[2];

            printf("Engine ECU Data\n");
            printf("RPM         : %u\n", rpm);
            printf("Temperature : %u C\n", temperature);
        }

        // Decode Sensor ECU
        else if (can_id == 0x200 && frame.can_dlc >= 4) {

            uint16_t vibration_raw =
                ((uint16_t)frame.data[0] << 8)
                | frame.data[1];

            uint16_t oil_raw =
                ((uint16_t)frame.data[2] << 8)
                | frame.data[3];

            float vibration = vibration_raw / 10.0f;
            float oil_pressure = oil_raw / 10.0f;

            printf("Sensor ECU Data\n");
            printf("Vibration    : %.1f mm/s\n", vibration);
            printf("Oil Pressure : %.1f bar\n", oil_pressure);
        }

        else {
            printf("Unknown CAN message or invalid DLC\n");
        }
    }

    close(sock);

    return 0;
}

