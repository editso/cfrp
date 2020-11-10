#include <iostream>
#include "cfrp.h"
#include <error.h>
#include <sys/socket.h>
#include <string.h>
using namespace cfrp;
#include "config.h"

int main(){
    Mapper *client = make_client(client_mapper);
    client->start();
    return 0;
}
