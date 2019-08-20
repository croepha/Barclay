

#include <stdio.h>

#include "barclay_client_lib.h"

int main () {
    
    auto& window = *barclay_window_init();
    
    for (;;) {
        
        
        barclay_window_frame(&window);
    }
    
    
}

