
#include "cfrp.h"
#include "clist.h"
#include "config.h"
#include <stdio.h>


int main(){
   cfrp *frp = make_cfrp(server_mapper, SERVER);
   clist* list = make_list();
   cmap* map = make_map(20);
   printf("max: %d\n", frp->mappers._prime);
   return 0;
}