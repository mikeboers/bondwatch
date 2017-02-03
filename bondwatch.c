#include <net/if.h>
#include <net/if_media.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


#define LOG(pattern, ...) { \
    time_t __time; \
    struct tm __tm; \
    time(&__time); \
    localtime_r(&__time, &__tm); \
    printf("[bondwatch] %04d-%02d-%02dT%02d:%02d:%02d " pattern "\n", \
        __tm.tm_year + 1900, __tm.tm_mon + 1, __tm.tm_mday, __tm.tm_hour, __tm.tm_min, __tm.tm_sec, \
        ##__VA_ARGS__ \
    ); \
};


int main(int argc, char **argv) {

    int ret;
    int sock = -1;

    char *nic_name = "bond0";
    char *command[] = {
        "/sbin/ifconfig",
        nic_name,
        "bondmode",
        "static"
    };

    struct ifreq ifr;
    strncpy(ifr.ifr_name, nic_name, sizeof(ifr.ifr_name));

    struct ifmediareq ifmed;
    memset(&ifmed, 0, sizeof(struct ifmediareq));
    strlcpy(ifmed.ifm_name, nic_name, sizeof(ifmed.ifm_name));


    int uid = getuid();
    if (uid) {
        LOG("WARNING: Not root; we won't be able to reset bondmode!");
    }

    while (1) {

        if (sock == -1) {
            LOG("DEBUG: Opening socket...");
            sock = socket(AF_INET, SOCK_DGRAM, 0);
        }
        if (sock == -1) {
            LOG("ERROR: Could not open socket.");
            sleep(10);
            continue;
        }

        ret = ioctl(sock, SIOCGIFFLAGS, &ifr);
        if (ret == -1) {
            LOG("ERROR: Could not SIOCGIFFLAGS.");
            sleep(10);
            continue;
        }

        if (!(ifr.ifr_flags & IFF_UP)) {
            LOG("WARNING: IF is down.");
            sleep(10);
            continue;
        }

        ret = ioctl(sock, SIOCGIFMEDIA, (caddr_t)&ifmed);
        if (ret == -1) {
            LOG("ERROR: Could not SIOCGIFMEDIA.");
            sleep(10);
            continue;
        }

        if (!(ifmed.ifm_active & IFM_FDX || ifmed.ifm_active & IFM_HDX)) {
            
            LOG("INFO: Bond appears down; setting bondmode to static.");

            if (!uid) {

                ret = fork();
                if (ret == -1) {
                    LOG("ERROR: Could not fork!");
                    sleep(10);
                    continue;
                }
                if (!ret) {
                    execv("/sbin/ifconfig", command);
                }
                wait(&ret);
                if (ret) {
                    LOG("ERROR: ifconfig exited with non-zero code (%d)!", ret);
                    sleep(9);
                }

            }
            sleep(1);
        } else {
            usleep(250000);
        }

    }

    return 0;

}