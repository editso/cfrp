
#include "cfrp.h"
#include "config.h"
#include <stdio.h>

int main(){
   cfrp *frp = make_cfrp(server_mapper, SERVER);
   int pid = cfrp_run(frp);
   
   printf("pid: %d\n", pid);
   return 0;
}