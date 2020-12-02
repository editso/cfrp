#include "cfrp.h"
#include "config.h"

int main(){
   cfrp *frp = make_cfrp(server_mapping, SERVER);
   cfrp_run(frp);   
   return 0;
}
