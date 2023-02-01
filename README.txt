Memory Simulation 
Authored by Mo'men Abu Gharbieh
318314853

==Description==
This program is a memory simulation for linux , and we are using the page mapping in it , we will have a small explanation here , (all the load and store calls are in the main.cpp file) .
our program get a virtual address and we have 2 kinds of calls - store or load - store means we want to save a specific value in that address in the MAIN MEMORY , and the load call means we want to get a specific value from the MAIN MEMORY , now we have many options here . 

-->lets start with load calls : when we want to load an address there is 2 options :
-> 1: its available in the MAIN MEMORY (if its yes AMAZING we just load the value that we want to the screen).
-> 2: if its not available in the MAIN MEMORY (now we have some work to do) , if the page is READONLY we bring the page from the exe file , if its READWRITE page , we need to check if the page is dirty -> if yes thats mean we already wrote on it and we will find it in swap file , if its not dirty -> we bring the page from the exe file EXCEPT if the page is from the BSS/STACK/HEAP type , in this case we can't load the page from this type if its not dirty .

--> lets talk about store calls : its almost the same as load except for 2 changes : if we have a store call for a READONLY page , we can't Obviously  cause its READONLY we write to it 
the second change is , now if the page is not dirty and its from the type of BSS/STACK/HEAP we can create a new page and store in it , in the specific address the value that we want .

now we will talk a little bit about FULL MEMORY situation , now if we want to load/store but the memory is full , we have few options , first lets talk about how we will empty the memory .
we will use FIFO , thats mean the first page that were store/load in the MAIN MEMORY we need to remove it , if the page is (READONLY or NOT DIRTY) we remove it without doing anything cause we can get it from the exe file , else : if the page is DIRTY - in another words we wrote on it , we move it to SWAP FILE and we save the index in the SWAP FILE so we can get it easily when we need it .

simply thats how the program works .


==Functions==
we have 8 functions :
the constructor function , it opens all the needed files and initials all the pages Informations 
the destructor function , it closes all the opened files and free all the allocated memory 
load function : it send the number of the process and the virtual address that we want to load , and we print it to the screen .
store function : it send the number of the process and the virtual address and the value that we want to store in this address but in the MAIN MEMORY
print memory function : prints the MAIN MEMORY
print swap file : prints the SWAP FILE content 
print page table : it print the page table of the processes 
create_frame : this function i added , it help me to do a lot of things in one call , first it check if there is space in the MAIN MEMORY to load the page that we want , if there is no space it will tell me which frame inside the MAIN MEMORY i need to remove to replace it with the new page that i want , and if i want to remove the page it helps me to send it to the swap file if needed.



==Program Files==
README.txt
main.cpp -> we have all the load/store calls in it 
sim_mem.cpp -> all the coding section is in this file
sim_mem.h -> its the header from the coding file .


==How to compile?==
compile : g++ main.cpp sim_mem.cpp -o ex5 .
run : ./ex5 .


==Input:==
THERE IS NO INPUT .


==Output:==
all the load function will be printed on the screen , and in the end will print the MAIN MEMORY , SWAP FILE and the page table for the processes .


