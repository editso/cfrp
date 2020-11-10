
#include <iostream>
#include "logging.h"
#include "cfrp.h"
#include "mber.h"
using namespace cfrp;
#include "config.h"


int main(){
   Mapper* server = make_server(server_mapper);
   server->start();
   return 0;
}