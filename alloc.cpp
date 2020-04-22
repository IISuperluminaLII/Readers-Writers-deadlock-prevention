#include <iostream>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fstream>
#include <sys/ipc.h>         /* general SysV IPC structures                */
#include <sys/sem.h>	     /* semaphore functions and structs.           */

#define FILE_PATH "res.txt"

union semuns {
    int		val;		/* value for SETVAL */
    struct semid_ds	*buf;		/* buffer for IPC_STAT & IPC_SET */
    unsigned short	*array;		/* array for GETALL & SETALL */
}arg;


//Resource Allocator Program
int main() {
    struct stat buffer{};
    int fd = open(FILE_PATH, O_RDWR); //6 = read+write for me!

    //File Stats
    fstat(fd,&buffer);
    //End File stats
    off_t size = buffer.st_size;

    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    //printf("Mapped at %p\n", addr);
    close(fd);

    if (addr == (void*) -1 ) exit(SIGBUS);

    char * shared = (char *)addr;
    //___________________________________SEM CREAT______________________________________
    int sem_id;
    union semuns sem_val{};

    int sys_call_ret;
    sem_id = semget(2000,1,IPC_CREAT | 0666);
    if(sem_id == -1){
        perror("Error creating sem");
        exit(1);
    }

    //mut val 1
    sem_val.val = 1;

    //Set system sem to sem_val
    sys_call_ret = semctl(sem_id,0,SETVAL,sem_val);
    if(sys_call_ret == -1){
        perror("Error initializing system sem");
        exit(1);
    }
    printf("sem val of %d initialized in system\n",2000);

//___________________________________________________________________________________
    while(true){
        std::string resource;
        int units = 0;
        std::string val;

        std::cout << "\nNew Resource to be allocated\n" << std::endl;
        std::cin >> val;
        struct sembuf sem_op{};
        if(val == "y") {
            std::cout << "Enter type and amount." << std::endl;
            std::cin >> resource;
            std::cin >> units;
            sem_op.sem_num = 0;
            sem_op.sem_op = -1;
            sem_op.sem_flg = 0;
            semop(sem_id,&sem_op,1);
            for (off_t j = 0; j < size; j++) {
                char temp = shared[j];
                if (temp == resource[0] && resource.size() == 1) {
                    char units_t = shared[j + 2];
                    shared[j + 2] = units_t - units;
                }
                if (temp == resource[0] && shared[j + 1] == resource[1] && resource.size() == 2) {
                    char units_t = shared[j + 3];
                    shared[j + 3] = units_t - units;
                }
            }
            if (::msync((void *) shared, size, MS_SYNC) < 0) {
                perror("msync failed with error");
            } else {
                printf("%s", "Synced File from memory");
            }
            sem_op.sem_num = 0;
            sem_op.sem_op = 1;
            sem_op.sem_flg = 0;
            semop(sem_id,&sem_op,1);

        }else{
            munmap(addr, size);
            exit(1);
        }
    }
}