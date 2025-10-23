#include "chan.h"
#include "gang.h"
#include "lib/debug.h"

using namespace lib;
using namespace lib::sync;

int main() {
    debug::init();

    Chan<int> myc1;
	Chan<int> myc2;
    Chan<int> myc3;
    Chan<int> done;

    sync::Gang g1, g2;
    
    for (int i = 0; i < 32; i++) {
        g1.go([&]{
            g2.go([&]{
                for (;;) {
                    int i = select(
                        Send(myc1, 0),
                        Send(myc2, 0),
                        Send(myc3, 0),
                        Recv(done)
                    );
                    // LOG("1 selected %v\n", i);
                    if (i == 3) {
                        break;
                    }
                }
            });
                
            
            for (int i = 0 ; i < 20000; i++) {
                select(
                    Recv(myc1),
                    Recv(myc2),
                    Recv(myc3)
                );
                // LOG("2 selected\n");
            }
        });
    }
    g1.join();
    done.close();
    g2.join();
}