#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <omp.h>
#include <iostream>
#include <chrono>
#include "sudoku_solver2.hpp"

/*inline void print_sudoku(char* sudoku) {
    for (size_t fila = 0; fila < 9; ++fila) {
        for (size_t col = 0; col < 9; ++col)
            std::cout << sudoku[fila * 9 + col] << ' ';
        std::cout << '\n';
    }
}*/

// char sudoku[] = "4..85...3....34...683..9.54.4....72...634..9...16.2..5....684.9..8..31.27.4.21538";
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, const char **argv) {
    (void)argc; // Ignoramos el límite, tu solver asume unicidad
    (void)argv;

    char *puzzle = NULL;
    size_t size = 0;

    while (getline(&puzzle, &size, stdin) != -1) {
        if (strlen(puzzle) < 81 || puzzle[0] == '#') continue;
        //printf("%.81s:", puzzle);
        solve(puzzle);
        //printf("1:%.81s\n", puzzle);
    }

    return 0;
}

/*int main() {
    char sudoku[] = "...64.....3...5.2..8.2....4..9..8.1...7.96..5..6....4..1...9.5...8..27...........";
    print_sudoku(sudoku);

    auto start = std::chrono::high_resolution_clock::now();
    for (uint64_t i = 0; i < 1'000'000; ++i) {
        solve(sudoku);
        __asm__ __volatile__("" : : "r"(sudoku) : "memory");
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    std::cout << "Tiempo promedio: " << duration_ns / 1'000'000 << " nanosegundos.\n\nResultado:\n";
    print_sudoku(sudoku);
    return 0;
}*/

/*int main() {
    constexpr size_t NUM_PUZZLES   = 100'000;
    constexpr size_t LINE_SIZE     = 82;
    constexpr size_t PUZ_SIZE      = 81;
    constexpr size_t BUF_SIZE      = NUM_PUZZLES * LINE_SIZE;

    // Buffer único alineado a 64 bytes
    char* buf = nullptr;
    if (posix_memalign((void**)&buf, 64, BUF_SIZE) != 0) {
        perror("posix_memalign");
        return 1;
    }

    // --- 1) Lectura única
    size_t rd = 0;
    while (rd < BUF_SIZE) {
        ssize_t r = ::read(0, buf + rd, BUF_SIZE - rd);
        if (r <= 0) {
            perror("read");
            free(buf);
            return 1;
        }
        rd += r;
    }

    // --- 2) Procesado en paralelo
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < NUM_PUZZLES; ++i) {
        char* line = buf + i * LINE_SIZE;
        solve(line);
        line[PUZ_SIZE] = '\n';
    }

    // --- 3) Escritura única
    size_t wr = 0;
    while (wr < BUF_SIZE) {
        ssize_t w = ::write(1, buf + wr, BUF_SIZE - wr);
        if (w <= 0) {
            perror("write");
            free(buf);
            return 1;
        }
        wr += w;
    }

    free(buf);
    return 0;
}*/

/*int main() {
    std::ios::sync_with_stdio(false);
    char buffer[82];
    while(read(STDIN_FILENO, buffer, 82)) {
        solve(buffer);
        write(STDOUT_FILENO, buffer, 82);
    }
}*/