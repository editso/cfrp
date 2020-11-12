
#include "cfrp.h"
#include "config.h"
#include "logger.h"

int main(){
    cfrp *frp = make_cfrp(client_mapper, CLIENT);
    cfrp_run(frp);
    return 0;
}
