#include <stdint.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "ddConfig.h"

struct ddAddressInfo
{
    struct addrinfo hints;
    struct addrinfo* options;
    struct addrinfo* selected;
    int32_t status;
    int32_t socket_fd;
};

void create_socket( struct ddAddressInfo* restrict address,
                    const char* ip,
                    const char* port,
                    const bool create_server );
