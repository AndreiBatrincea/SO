#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>
#include <cstring>

bool isPrime(int n) {
    if (n <= 1) return false;
    for (int i = 2; i * i <= n; ++i) {
        if (n % i == 0) return false;
    }
    return true;
}

void findPrimes(int start, int end, int writeFd) {
    std::ostringstream primes;
    for (int i = start; i < end; ++i) {
        if (isPrime(i)) {
            primes << i << " ";
        }
    }

    // Scrie numerele prime în pipe
    std::string primesStr = primes.str();
    write(writeFd, primesStr.c_str(), primesStr.size());
    close(writeFd); // Închidem pipe-ul după scriere
}

int main() {
    const int numProcesses = 10;
    const int range = 10000;
    const int chunkSize = range / numProcesses;

    int pipes[numProcesses][2]; // Pipe-uri pentru fiecare proces

    pid_t pids[numProcesses];

    for (int i = 0; i < numProcesses; ++i) {
        // Creează pipe-ul
        if (pipe(pipes[i]) == -1) {
            std::cerr << "Eroare la crearea pipe-ului!" << std::endl;
            return 1;
        }

        // Creează procesul copil
        pids[i] = fork();
        if (pids[i] == -1) {
            std::cerr << "Eroare la crearea procesului!" << std::endl;
            return 1;
        }

        if (pids[i] == 0) {
            // Cod proces copil
            close(pipes[i][0]); // Închide capătul de citire al pipe-ului
            int start = i * chunkSize;
            int end = (i + 1) * chunkSize;
            findPrimes(start, end, pipes[i][1]);
            return 0; // Procesul copil se încheie
        } else {
            // Cod proces părinte
            close(pipes[i][1]); // Închide capătul de scriere al pipe-ului
        }
    }

    // Procesul părinte citește din pipe-uri
    for (int i = 0; i < numProcesses; ++i) {
        char buffer[4096];
        ssize_t bytesRead = read(pipes[i][0], buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0'; // Null-terminate
            std::cout << "Numere prime de la procesul " << i << ": " << buffer << std::endl;
        } else {
            std::cerr << "Eroare la citirea din pipe pentru procesul " << i << "!" << std::endl;
        }
        close(pipes[i][0]); // Închidem capătul de citire
    }

    // Așteptăm procesele copil să se termine
    for (int i = 0; i < numProcesses; ++i) {
        waitpid(pids[i], nullptr, 0);
    }

    return 0;
}

