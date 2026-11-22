
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

int main()
{
    int sock;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame rx_frame;
    struct can_frame tx_frame;

    // Create CAN socket
    sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // Select vcan1
    strcpy(ifr.ifr_name, "vcan1");

    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("Interface lookup failed");
        close(sock);
        return 1;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    // Bind socket
    if (bind(sock, (struct sockaddr *)&addr,
             sizeof(addr)) < 0) {
        perror("Bind failed");
        close(sock);
        return 1;
    }

    printf("C Diagnostic ECU Started...\n");
    printf("Listening on vcan1\n");

    while (1) {

        int nbytes = read(sock, &rx_frame, sizeof(rx_frame));

        if (nbytes < 0) {
            perror("CAN read failed");
            break;
        }

        if (nbytes != sizeof(rx_frame)) {
            continue;
        }

        unsigned int can_id =
            rx_frame.can_id & CAN_SFF_MASK;

        uint8_t status_code = 0;
        int response_id = 0;

        // Engine ECU
        if (can_id == 0x100 && rx_frame.can_dlc >= 3) {

            uint16_t rpm =
                ((uint16_t)rx_frame.data[0] << 8)
                | rx_frame.data[1];

            uint8_t temperature = rx_frame.data[2];

            printf("\nEngine ECU\n");
            printf("RPM         : %u\n", rpm);
            printf("Temperature : %u C\n", temperature);

            if (temperature >= 105) {
                status_code = 2;
                printf("Status      : OVERHEATING\n");
            }
            else if (temperature >= 100) {
                status_code = 1;
                printf("Status      : HIGH TEMPERATURE\n");
            }
            else {
                status_code = 0;
                printf("Status      : NORMAL\n");
            }

            response_id = 0x300;
        }

        // Sensor ECU
        else if (can_id == 0x200 && rx_frame.can_dlc >= 4) {

            uint16_t vibration_raw =
                ((uint16_t)rx_frame.data[0] << 8)
                | rx_frame.data[1];

            uint16_t oil_raw =
                ((uint16_t)rx_frame.data[2] << 8)
                | rx_frame.data[3];

            float vibration = vibration_raw / 10.0f;
            float oil_pressure = oil_raw / 10.0f;

            printf("\nSensor ECU\n");
            printf("Vibration    : %.1f mm/s\n", vibration);
            printf("Oil Pressure : %.1f bar\n", oil_pressure);

            if (vibration >= 7.0f) {
                status_code = 1;
                printf("Status       : HIGH VIBRATION\n");
            }
            else if (oil_pressure <= 1.5f) {
                status_code = 2;
                printf("Status       : LOW OIL PRESSURE\n");
            }
            else {
                status_code = 0;
                printf("Status       : NORMAL\n");
            }

            response_id = 0x301;
        }

        else {
            continue;
        }

        // Send diagnostic response
        tx_frame.can_id = response_id;
        tx_frame.can_dlc = 1;
        tx_frame.data[0] = status_code;

        if (write(sock, &tx_frame, sizeof(tx_frame)) < 0) {
            perror("CAN transmit failed");
        }
        else {
            printf("Diagnostic CAN ID: 0x%03X\n", response_id);
            printf("Status Code      : %u\n", status_code);
        }
    }

    close(sock);
    return 0;
}
