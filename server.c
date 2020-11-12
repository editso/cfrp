
#include "cfrp.h"
#include "clist.h"
#include "config.h"
#include "logger.h"
#include <stdio.h>


int main(){
   cfrp *frp = make_cfrp(server_mapper, SERVER);
   cfrp_run(frp);   
   return 0;
}