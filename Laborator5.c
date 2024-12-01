#include <stdio.h> 
#include <stdlib.h> 
#include <fcntl.h> 
#include <sys/mman.h> 
#include <semaphore.h> 
#include <unistd.h> 
#include <time.h> 
#include <string.h> 

// Numele memoriei partajate și al semaforului
#define SHM_NAME "/shared_memory_example" 
#define SEM_NAME "/semaphore_example" 

// Dimensiunea memoriei partajate
#define SHM_SIZE sizeof(int) 

// Funcția care simulează aruncarea unei monede (0 sau 1)
int coin_toss() { 
    return rand() % 2; // Returnează 0 sau 1
} 

int main() { 
    int fd, *shared_num; 
    sem_t *sem; 

    // Inițializarea generatorului de numere aleatoare
    srand(time(NULL) ^ getpid()); 

    // Crearea unui fișier pentru memoria partajată
    fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666); 
    if (fd < 0) { 
        perror("shm_open"); 
        exit(1); 
    } 

    // Setarea dimensiunii fișierului pentru memoria partajată
    if (ftruncate(fd, SHM_SIZE) == -1) { 
        perror("ftruncate"); 
        exit(1); 
    } 

    // Mapați memoria partajată într-un pointer
    shared_num = (int*)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
    if (shared_num == MAP_FAILED) { 
        perror("mmap"); 
        exit(1); 
    } 

    // Crearea semaforului
    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1); 
    if (sem == SEM_FAILED) { 
        perror("sem_open"); 
        exit(1); 
    } 

    // Bucla principală
    while (1) { 
        // Blocăm semaforul pentru a avea acces exclusiv la memoria partajată
        sem_wait(sem); 

        int current = *shared_num; // Citim valoarea curentă din memoria partajată

        // Dacă am ajuns la 1000, încheiem programul
        if (current >= 1000){ 
            sem_post(sem); // Eliberăm semaforul
            break; // Iesim din buclă
        } 

        // Afișăm valoarea curentă citită
        printf("Process %d read: %d\n", getpid(), current); 

        // Aruncăm moneda. Dacă e 1, incrementăm numărul
        while (coin_toss() == 1) { 
            (*shared_num)++; // Incrementăm valoarea în memoria partajată
            printf("Process %d writes: %d\n", getpid(), *shared_num); 
        } 

        // Eliberăm semaforul pentru a permite altor procese să acceseze memoria
        sem_post(sem); 

        // Pauză pentru a reduce utilizarea procesorului
        usleep(100000); 
    } 

    // Eliberăm resursele
    munmap(shared_num, SHM_SIZE); 
    close(fd); 
    sem_close(sem); 

    // Dacă este procesul principal, curățăm resursele
    if (getpid() % 2 == 0) { 
        shm_unlink(SHM_NAME); 
        sem_unlink(SEM_NAME); 
    } 

    return 0; 
}
