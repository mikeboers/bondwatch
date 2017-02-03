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

    struct timeval tv;
    struct timezone tz;
    time_t time_;
    struct tm tm_;

    int uid = getuid();
    if (uid) {
        printf("Not root; won't be able to reset bondmode!\n");
    }

    while (1) {

        if (sock == -1) {
            sock = socket(AF_INET, SOCK_DGRAM, 0);
        }
        if (sock == -1) {
            printf("Could not open socket.\n");
            sleep(10);
            continue;
        }

        ret = ioctl(sock, SIOCGIFFLAGS, &ifr);
        if (ret == -1) {
            printf("Could not SIOCGIFFLAGS.\n");
            sleep(10);
            continue;
        }

        if (!(ifr.ifr_flags & IFF_UP)) {
            printf("IF is down.\n");
            sleep(10);
            continue;
        }

        ret = ioctl(sock, SIOCGIFMEDIA, (caddr_t)&ifmed);
        if (ret == -1) {
            printf("Could not SIOCGIFMEDIA.\n");
            sleep(10);
            continue;
        }

        if (!(ifmed.ifm_active & IFM_FDX || ifmed.ifm_active & IFM_HDX)) {
            
            time(&time_);
            localtime_r(&time_, &tm_);
            printf("%04d-%02d-%02dT%02d:%02d:%02d IF is not set up; setting bondmode to static...\n",
                tm_.tm_year + 1900,
                tm_.tm_mon + 1,
                tm_.tm_mday,
                tm_.tm_hour,
                tm_.tm_min,
                tm_.tm_sec
            );

            if (!uid) {

                ret = fork();
                if (ret == -1) {
                    printf("Could not fork!\n");
                    sleep(10);
                    continue;
                }
                if (!ret) {
                    execv("/sbin/ifconfig", command);
                }
                wait(&ret);
                if (ret) {
                    printf("ifconfig exited with code %d!\n", ret);
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