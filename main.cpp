#include "sim_mem.h"
#include <iostream>

using namespace std;

int main(){
    char exe1[20] = "exe1_file";
    char exe2[20] = "exe2_file";
    char swap[20] = "swap_file";
    sim_mem mem_sim(exe1,exe2,swap,25,50,25,25,
                    25,5,1);
    // 0<= TEXT <25 , 25<= DATA < 75 , 75<= BSS < 100 , 100 <= HEAP&STACK < 125

    mem_sim.store(1,0,'x'); // FAIL
    mem_sim.store(1,27,'x'); // SUCCESS
    cout<<mem_sim.load(1,27)<<endl; // SUCCESS
    mem_sim.store(1,32,'y'); // SUCCESS
    cout<<mem_sim.load(1,32)<<endl; // SUCCESS
    cout<<mem_sim.load(1,75)<<endl; // SUCCESS
    cout<<mem_sim.load(1,100)<<endl; // FAIL
    mem_sim.store(1,100,'z'); // SUCCESS
    mem_sim.load(1,0); // FIRST PAGE
    mem_sim.load(1,5); // SECOND PAGE

    mem_sim.print_memory();
    mem_sim.print_swap();
    mem_sim.print_page_table();
}
