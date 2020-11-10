
#include <iostream>
#include "logging.h"
#include "rpr.h"
#include "mber.h"
using namespace rpr;
#include "config.h"


int main(){
   Mapper* server = make_server(server_mapper);
   server->start();
   return 0;
}