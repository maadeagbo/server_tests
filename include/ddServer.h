#include <stdio.h>
#include <stdint.h>
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
};

void get_server_info( struct ddAddressInfo* restrict address,
                      const char* ip,
                      const char* port );
