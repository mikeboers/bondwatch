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
    fflush(stdout); \
};


int main(int argc, char **argv) {

    int ret;
    int sock = -1;

    // Options.
    int allow_rootless = 0;
    char *nic_name = "bond0";

    // Parse CLI arguments into options.
    int flag;
    while ((flag = getopt(argc, argv, "hwi:")) != -1) {
        switch(flag) {

            case 'h':
                printf("Usage: %s [-w] [-i interface]\n", argv[0]);
                printf("\n");
                printf("Options:\n");
                printf("-h            Print this help, an exit.\n");
                printf("-i interface  Which interface to watch; defaults to \"bond0\".\n");
                printf("-w            Watch for changes, even if we aren't root.\n");
                exit(0);

            case 'w':
                allow_rootless = 1;
                break;

            case 'i':
                nic_name = strdup(optarg);
                break;

        }
    }


    // Assert we are root.
    int uid = getuid();
    if (uid) {
        LOG("WARNING: Not root; we won't be able to reset bondmode!");
        if (!allow_rootless) {
            LOG("ERROR: Use -w to watch without being able to take action.");
            exit(1);
        }
    }

    // Prep the command we will run to set the bondmode.
    char command[256];
    snprintf(command, 255, "/sbin/ifconfig %s bondmode static", nic_name);

    // Prep the ifreq and ifmediareq structs. In my testing these only
    // need to be setup once, and can be re-used.
    struct ifreq ifr;
    strncpy(ifr.ifr_name, nic_name, sizeof(ifr.ifr_name));
    struct ifmediareq ifmed;
    memset(&ifmed, 0, sizeof(struct ifmediareq));
    strlcpy(ifmed.ifm_name, nic_name, sizeof(ifmed.ifm_name));

    while (1) {

        if (sock == -1) {
            // I've never seen this hit twice.
            LOG("DEBUG: Opening socket...");
            sock = socket(AF_INET, SOCK_DGRAM, 0);
        }
        if (sock == -1) {
            LOG("ERROR: Could not open socket.");
            sleep(10);
            continue;
        }

        // Assert the interface is up.
        ret = ioctl(sock, SIOCGIFFLAGS, &ifr);
        if (ret == -1) {
            LOG("ERROR: Could not SIOCGIFFLAGS; are you sure the interface exists?");
            sleep(10);
            continue;
        }
        if (!(ifr.ifr_flags & IFF_UP)) {
            LOG("WARNING: Interface is down.");
            sleep(10);
            continue;
        }

        ret = ioctl(sock, SIOCGIFMEDIA, (caddr_t)&ifmed);
        if (ret == -1) {
            LOG("ERROR: Could not SIOCGIFMEDIA.");
            sleep(10);
            continue;
        }

        // We are checking if it appears to be either fully or half duplexed.
        // In my testing, my bond had reverted to LACP when this had stopped
        // being true.
        if (!(ifmed.ifm_active & IFM_FDX || ifmed.ifm_active & IFM_HDX)) {

            if (uid) {
                // Just watching.
                LOG("INFO: Bond appears inactive.");
                sleep(5);

            } else {
                LOG("INFO: Bond appears inactive; setting bondmode to static.");
                ret = system(command);
                if (ret) {
                    LOG("ERROR: ifconfig exited with non-zero code (%d)!", ret);
                    sleep(10);
                }
            }

        } else {
            // 4Hz seems to be pretty good in my usage.
            // Even at a much higher frequency there isn't a visible
            // load on the system.
            usleep(250000);
        }

    }

    return 0;

}