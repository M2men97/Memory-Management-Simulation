#include <cstdio>
#include <fcntl.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <bits/stdc++.h>
#include "sim_mem.h"

#define RO 0 // READONLY
#define RW 1 // READWRITE

char main_memory[MEMORY_SIZE];
/*---------------------------------------------CONSTRUCTOR---------------------------------------------*/
sim_mem::sim_mem(char exe_file_name1[],char exe_file_name2[],char swap_file_name[],int text_size,int data_size
                 ,int bss_size,int heap_stack_size,int num_of_pages , int page_size ,int num_of_process ){
    // FIRST WE SET ALL THE VARIABLES
    this->text_size = text_size;
    this->data_size = data_size;
    this->bss_size = bss_size;
    this->heap_stack_size = heap_stack_size;
    this->num_of_pages = num_of_pages;
    this->page_size = page_size;
    this->num_of_proc = num_of_process;
    int text_pages = this->text_size / this->page_size ;
    this->SWAPINDEX = 0;
    // OPEN/CREATE FILES
    // INITIALIZE EXE FILE
    if(num_of_process == 2){
        // THEN WE NEED TO OPEN THE 2 EXE FILES
        this->program_fd[0] = open(exe_file_name1,O_RDONLY, 0);
        if(this->program_fd[0] == -1){
            perror("EXE1 FILE FAILED TO OPEN");
            this->~sim_mem();
            exit(EXIT_FAILURE);
        }
        this->program_fd[1] = open(exe_file_name2,O_RDONLY, 0);
        if(this->program_fd[0] == -1){
            perror("EXE2 FILE FAILED TO OPEN");
            this->~sim_mem();
            exit(EXIT_FAILURE);
        }
    }
    else{
        // IN THIS CASE WE OPEN ONLY ONE EXE FILE
        this->program_fd[0] = open(exe_file_name1,O_RDONLY, 0);
        if(this->program_fd[0] == -1){
            perror("EXE1 FILE FAILED TO OPEN");
            this->~sim_mem();
            exit(EXIT_FAILURE);
        }
    }
    // INITIALIZE SWAP FILE
    FILE* clear ;
    clear = fopen(swap_file_name, "w+");
    if (clear == nullptr){
        perror("COULD NOT OPEN THE SWAP FILE");
        this->~sim_mem();
        exit(EXIT_FAILURE);
    }
    fclose(clear); /* THIS LINE WILL CLEAR THE SWAPFILE EVERY RUN
            * SO WE CAN FILL IT AGAIN WITH THE SUITABLE SIZE */
    this->swapfile_fd = open(swap_file_name, O_CREAT|O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
    if(this->swapfile_fd == -1){
        perror("SWAP FILE FAILED TO OPEN");
        this->~sim_mem();
        exit(EXIT_FAILURE);
    }
        // NOW WE NEED TO FILL IT WITH ZEROS
    char* zero = (char*)calloc(page_size, sizeof(char));
    if(zero == nullptr){
        perror("ALLOCATION FAILED");
        this->~sim_mem();
        exit(EXIT_FAILURE);
    }
    int write_swap;
    for(int i=0;i<(this->page_size*(this->num_of_pages-text_pages)*this->num_of_proc);i++){
        write_swap = (int)write(this->swapfile_fd,"0",1);
        if(write_swap == -1){
            perror("FAILED TO WRITE TO SWAP");
        }
    }
    // INITIALIZE MAIN MEMORY
    memset(main_memory,'0',MEMORY_SIZE);
    // INITIALIZE PAGE TABLE
    this->page_table = (page_descriptor**) calloc(this->num_of_proc, sizeof(page_descriptor*));
    if(this->page_table == nullptr){
        perror("PAGE TABLE ALLOCATION FAILED");
        this->~sim_mem();
        exit(EXIT_FAILURE);
    }
    for(int i=0;i<num_of_process;i++){
        this->page_table[i] = (page_descriptor*) calloc(this->num_of_pages, sizeof(page_descriptor));
        if(this->page_table[i] == nullptr){
            perror("PAGE TABLE ALLOCATION FAILED");
            this->~sim_mem();
            exit(EXIT_FAILURE);
        }
        for(int j=0;j<this->num_of_pages;j++){
            if(j<text_pages)
                this->page_table[i][j].P = RO ; // READONLY
            else
                this->page_table[i][j].P = RW ; // READWRITE
            this->page_table[i][j].V = 0 ;
            this->page_table[i][j].D = 0 ;
            this->page_table[i][j].frame = -1;
            this->page_table[i][j].swap_index = -1;
        }
    }
}
/*---------------------------------------------DESTRUCTOR---------------------------------------------*/
sim_mem::~sim_mem(){
    // MORE DETAILS
    for(int i=0;i<num_of_proc;i++){
        free(this->page_table[i]);
    }
    free(page_table);
    close(swapfile_fd);
    if(num_of_proc == 2){
        close(program_fd[0]);
        close(program_fd[1]);
    }
    else
        close(program_fd[0]);
}
/*---------------------------------------------LOAD---------------------------------------------*/
char sim_mem::load(int process_id, int address){
   int proc = process_id-1; // PROCESS NUM (0 OR 1)
   char bPage[this->page_size]; // BUFFER FOR PAGES
   int page = address/this->page_size; // TO GET PAGE NUM
   int offset = address%this->page_size; // TO GET THE PLACE IN THE PAGE
   if(this->page_table[proc][page].V == 1){ // THE PAGE IS VALID (IN MAIN MEMORY)
       return main_memory[(this->page_table[proc][page].frame * this->page_size) + offset];
   }
   else{ // THE PAGE IS NOT VALID (NOT IN MAIN MEMORY)
        int nFrame; // NEW FRAME
        int RD; // READER FOR FILES
        if(this->page_table[proc][page].P == RO){ // TEXT PAGE -> READONLY
            create_frame(); // CREATE FRAME
            nFrame = aFrame.front(); // AVAILABLE FRAME
            aFrame.pop(); // CLEAR THE aFrame QUEUE
            uFrame.push(nFrame); // INSERT THE NEW FRAME INSIDE THE USED FRAMES QUEUE
            lseek(program_fd[proc],page*this->page_size,SEEK_SET);
            memset(bPage,'0',this->page_size);
            RD = read(program_fd[proc],bPage,this->page_size); // READ FROM EXE FILE
            if(RD == -1){
                perror("READ PROGRAM_FD FAILED \n");
                this->~sim_mem();
                exit(EXIT_FAILURE);
            }
            for(int i=(nFrame*this->page_size),j=0;j<this->page_size;i++,j++)
                main_memory[i] = bPage[j];
            this->page_table[proc][page].V = 1 ;
            this->page_table[proc][page].frame = nFrame ;
            return main_memory[(this->page_table[proc][page].frame * this->page_size) + offset];
        }
        else{ // IF THE PAGE IS DATA , BSS OR HEAP&STACK -> READWRITE
            if(this->page_table[proc][page].D == 0){ // IF THE PAGE IS NOT DIRTY
                if(page >= ((this->text_size+this->data_size+this->bss_size)/this->page_size)){ // HEAP&STACK PAGE
                    fprintf(stderr,"CAN NOT LOAD UNSTORED HEAP&STACK PAGE \n");
                    return '\0';
                }
                else if(page >= ((this->text_size+this->data_size)/this->page_size) &&
                page < ((this->text_size+this->data_size+this->bss_size)/this->page_size)){ // BSS PAGE
                    memset(bPage,'0',this->page_size); // CREATE EMPTY PAGE
                    create_frame(); // CREATE FRAME
                    nFrame = aFrame.front(); // AVAILABLE FRAME
                    aFrame.pop(); // CLEAR THE aFrame QUEUE
                    uFrame.push(nFrame); // INSERT THE NEW FRAME INSIDE THE USED FRAMES QUEUE
                    for(int i=(nFrame*this->page_size),j=0;j<this->page_size;i++,j++)
                        main_memory[i] = bPage[j];
                    this->page_table[proc][page].V = 1 ;
                    this->page_table[proc][page].frame = nFrame ;
                    return main_memory[(this->page_table[proc][page].frame * this->page_size) + offset];
                }
                else{ // DATA PAGE
                    create_frame(); // CREATE FRAME
                    nFrame = aFrame.front(); // AVAILABLE FRAME
                    aFrame.pop(); // CLEAR THE aFrame QUEUE
                    uFrame.push(nFrame); // INSERT THE NEW FRAME INSIDE THE USED FRAMES QUEUE
                    lseek(program_fd[proc],page*this->page_size,SEEK_SET);
                    memset(bPage,'0',this->page_size);
                    RD = read(program_fd[proc],bPage,this->page_size); // READ FROM EXE FILE
                    if(RD == -1){
                        perror("READ PROGRAM_FD FAILED \n");
                        this->~sim_mem();
                        exit(EXIT_FAILURE);
                    }
                    for(int i=(nFrame*this->page_size),j=0;j<this->page_size;i++,j++)
                        main_memory[i] = bPage[j];
                    this->page_table[proc][page].V = 1 ;
                    this->page_table[proc][page].frame = nFrame ;
                    return main_memory[(this->page_table[proc][page].frame * this->page_size) + offset];
                }
            }
            else{ // IF THE PAGE IS DIRTY
                create_frame(); // CREATE FRAME
                nFrame = aFrame.front(); // AVAILABLE FRAME
                aFrame.pop(); // CLEAR THE aFrame QUEUE
                uFrame.push(nFrame); // INSERT THE NEW FRAME INSIDE THE USED FRAMES QUEUE
                lseek(swapfile_fd,this->page_table[proc][page].swap_index*this->page_size,SEEK_SET); // READ FROM SWAP FILE
                memset(bPage,'0',this->page_size);
                RD = read(swapfile_fd,bPage,this->page_size);
                if(RD == -1){
                    perror("READ PROGRAM_FD FAILED \n");
                    this->~sim_mem();
                    exit(EXIT_FAILURE);
                }
                for(int i=(nFrame*this->page_size),j=0;j<this->page_size;i++,j++)
                    main_memory[i] = bPage[j];
                this->page_table[proc][page].V = 1 ;
                this->page_table[proc][page].frame = nFrame ;
                return main_memory[(this->page_table[proc][page].frame * this->page_size) + offset];
            }
        }
   }
}
/*---------------------------------------------STORE---------------------------------------------*/
void sim_mem::store(int process_id, int address, char value){
    int proc = process_id-1; // PROCESS NUM (0 OR 1)
    char bPage[this->page_size]; // BUFFER FOR PAGES
    int page = address/this->page_size; // TO GET PAGE NUM
    int offset = address%this->page_size; // TO GET THE PLACE IN THE PAGE
    if(this->page_table[proc][page].P == RO){// CAN NOT WRITE TO READONLY FILE
        fprintf(stderr,"CAN NOT STORE IN READONLY FILE \n");
        return;
    }
    if(this->page_table[proc][page].V == 1){ // PAGE IS VALID (IN MAIN MEMORY)
        main_memory[(this->page_table[proc][page].frame * this->page_size) + offset] = value ;
        this->page_table[proc][page].D = 1; // THIS PAGE IS DIRTY NOW
    }
    else{ // PAGE NOT VALID (NOT IN MAIN MEMORY)
        int nFrame; // NEW FRAME
        int RD; // READER FOR FILES
        if(this->page_table[proc][page].D == 0){ // PAGE IS NOT DIRTY
            if(page >= ((this->text_size + this->data_size)/this->page_size)){ // BSS OR HEAP&STACK PAGE
                create_frame(); // CREATE FRAME
                nFrame = aFrame.front(); // AVAILABLE FRAME
                aFrame.pop(); // CLEAR THE aFrame QUEUE
                uFrame.push(nFrame); // INSERT THE NEW FRAME INSIDE THE USED FRAMES QUEUE
                memset(bPage,'0',this->page_size); // CREATE EMPTY NEW PAGE
                for(int i=(nFrame*this->page_size),j=0;j<this->page_size;i++,j++)
                    main_memory[i] = bPage[j];
                main_memory[(nFrame * this->page_size) + offset] = value ;
                this->page_table[proc][page].D = 1; // THIS PAGE IS DIRTY NOW
                this->page_table[proc][page].V = 1; // NOW IT IS VALID
                this->page_table[proc][page].frame = nFrame;
            }
            else{ // DATA PAGE
                create_frame(); // CREATE FRAME
                nFrame = aFrame.front(); // AVAILABLE FRAME
                aFrame.pop(); // CLEAR THE aFrame QUEUE
                uFrame.push(nFrame); // INSERT THE NEW FRAME INSIDE THE USED FRAMES QUEUE
                lseek(program_fd[proc],page*this->page_size,SEEK_SET);
                memset(bPage,'0',this->page_size);
                RD = read(program_fd[proc],bPage,this->page_size); // READ FROM EXE FILE
                if(RD == -1){
                    perror("READ PROGRAM_FD FAILED \n");
                    this->~sim_mem();
                    exit(EXIT_FAILURE);
                }
                for(int i=(nFrame*this->page_size),j=0;j<this->page_size;i++,j++)
                    main_memory[i] = bPage[j];
                main_memory[(nFrame * this->page_size) + offset] = value ;
                this->page_table[proc][page].D = 1; // THIS PAGE IS DIRTY NOW
                this->page_table[proc][page].V = 1; // NOW IT IS VALID
                this->page_table[proc][page].frame = nFrame ;
            }
        }
        else{ // PAGE IS DIRTY
            create_frame(); // CREATE FRAME
            nFrame = aFrame.front(); // AVAILABLE FRAME
            aFrame.pop(); // CLEAR THE aFrame QUEUE
            uFrame.push(nFrame); // INSERT THE NEW FRAME INSIDE THE USED FRAMES QUEUE
            lseek(swapfile_fd,page*this->page_size,SEEK_SET); // READ FROM SWAP FILE
            memset(bPage,'0',this->page_size);
            RD = read(swapfile_fd,bPage,this->page_size); // READ FROM EXE FILE
            if(RD == -1){
                perror("READ PROGRAM_FD FAILED \n");
                this->~sim_mem();
                exit(EXIT_FAILURE);
            }
            for(int i=(nFrame*this->page_size),j=0;j<this->page_size;i++,j++)
                main_memory[i] = bPage[j];
            main_memory[(nFrame * this->page_size) + offset] = value ;
            this->page_table[proc][page].V = 1; // NOW IT IS VALID
            this->page_table[proc][page].frame = nFrame ;
        }
    }
}
/*---------------------------------------------PRINT FUNCTIONS---------------------------------------------*/
void sim_mem::print_memory() {
    int i;
    printf("\n Physical memory\n");
    for(i = 0; i < MEMORY_SIZE; i++) {
        printf("[%c]\n", main_memory[i]);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void sim_mem::print_swap() const {
    char* str = (char*)malloc(this->page_size *sizeof(char));
    int i;
    printf("\n Swap memory\n");
    lseek(swapfile_fd, 0, SEEK_SET); // go to the start of the file
    while(read(swapfile_fd, str, this->page_size) == this->page_size) {
        for(i = 0; i <page_size; i++) {
            printf("%d - [%c]\t", i, str[i]);
        }
        printf("\n");
    }
    free(str);
}
/*---------------------------------------------------------------------------------------------------------*/
void sim_mem::print_page_table() {
    for (int j = 0; j < num_of_proc; j++) {
        printf("\n page table of process: %d \n", j+1);
        printf("Valid\t Dirty\t Permission \t Frame\t Swap index\n");
        for(int i = 0; i <num_of_pages; i++) {
           printf("[%d]\t[%d]\t[%d]\t[%d]\t[%d]\n",
           page_table[j][i].V,
           page_table[j][i].D,
           page_table[j][i].P,
           page_table[j][i].frame ,
           page_table[j][i].swap_index);
        }
    }
}
/*--------------------------------------------- HELP FUNCTION ---------------------------------------------*/
void sim_mem :: create_frame(){
    if(aFrame.empty()){
        // WE WANT TO CHECK WHERE IS THE FIRST EMPTY FRAME , SO WE CAN COPY THE PAGE INTO IT IN THE MAIN MEMORY
        int fFrame = 0; // WHEN WE FIND AN AVAILABLE FRAME WE WILL SWITCH IT TO 1
        for(int i=0;i<MEMORY_SIZE;i+=this->page_size){ // THIS WILL CHECK THE MAIN MEMORY FRAME BY FRAME
            int check = 0;
            for(int j=0;j<this->page_size;j++){ // THIS WILL CHECK IF THIS FRAME IS EMPTY
                if(main_memory[j] == '0'){
                    check++;
                }
            }
            for(int y = 0; y < this->num_of_proc;y++){ // IN CASE WE FOUND A FRAME , WE NEED TO CHECK THAT IT'S NOT USED BY EMPTY BSS PAGE ...
                for(int x=(this->text_size+ this->data_size)/this->page_size;x < (this->text_size + this->data_size + this->bss_size)/this->page_size ;x++){
                    if(this->page_table[y][x].frame == i){
                        check = 0;
                    }
                }
            }
            if(check == this->page_size){ // THEN WE HAVE A FREE FRAME HERE
                aFrame.push(i/this->page_size); // WE WILL SAVE IT INSIDE THE AVAILABLE FRAMES
                fFrame = 1;
            }
        }
        // IN CASE WE DID NOT FIND ANY AVAILABLE FRAME
        if(fFrame == 0){// THEN WE WILL USE A FRAME FROM THE USED FRAME QUEUE ,
            int frame = uFrame.front();
            aFrame.push(frame);
            uFrame.pop();
            char bPage[this->page_size]; // BUFFER FOR PAGE
            memset(bPage,'0',this->page_size);
            int WR; // WRITER
            for(int i=0;i<this->num_of_proc;i++){ // IN CASE WE HAVE 2 PROCESSES
                for(int j=0;j<this->num_of_pages;j++){
                    if(this->page_table[i][j].frame == frame){ // WE FOUND THE PAGE THAT WE WANT TO REMOVE
                        if(this->page_table[i][j].D == 0){ // THE PAGE IS NOT DIRTY
                            for(int k=frame*this->page_size;k<(frame*this->page_size)+this->page_size;k++)
                                main_memory[k] = '0';
                        }
                        else{ // WE NEED TO SEND THE PAGE TO SWAP FILE
                            for(int k=frame*this->page_size,x=0;x<this->page_size;k++,x++){
                                bPage[x] = main_memory[k];
                                main_memory[k] = '0';
                            }
                            if(this->page_table[i][j].swap_index == -1){ // NEVER BEEN IN SWAP FILE
                                lseek(this->swapfile_fd,this->SWAPINDEX*page_size,SEEK_SET);
                                this->page_table[i][j].swap_index = this->SWAPINDEX;
                                this->SWAPINDEX++;
                                WR = (int)write(this->swapfile_fd,bPage,this->page_size);
                                if(WR == -1){
                                    perror("COULD NOT WRITE TO THE SWAP FILE");
                                    this->~sim_mem();
                                    exit(EXIT_FAILURE);
                                }
                            }
                            else{
                                lseek(this->swapfile_fd,this->page_table[i][j].swap_index*page_size,SEEK_SET);
                                WR = (int)write(this->swapfile_fd,bPage,this->page_size);
                                if(WR == -1){
                                    perror("COULD NOT WRITE TO THE SWAP FILE");
                                    this->~sim_mem();
                                    exit(EXIT_FAILURE);
                                }
                            }
                        }
                        this->page_table[i][j].V = 0;
                        this->page_table[i][j].frame = -1;
                    }
                }
            }
        }
    }
}
/*---------------------------------------------------------------------------------------------------------*/
