#ifndef SUDOKU_SOLVER_HPP
#define SUDOKU_SOLVER_HPP

/**
 * @file sudoku.hpp
 * @author Juan Carlos Salas Ariza (juancarlossalasariza@gmail.com)
 * @brief Fast Sudoku solver using constraint propagation and backtracking.
 * @version 2.0
 * @date 2025-05-17
 * 
 * @copyright Copyright (c) 2025 Juan Carlos Salas Ariza
 * 
 * @note All rights reserved. Unauthorized copying or use is prohibited.
 */

#include <stdint.h>
#include <cstring>

constexpr size_t row_per_index[81] = {
     0, 0, 0, 0, 0, 0, 0, 0, 0,
     1, 1, 1, 1, 1, 1, 1, 1, 1,
     2, 2, 2, 2, 2, 2, 2, 2, 2,
     3, 3, 3, 3, 3, 3, 3, 3, 3,
     4, 4, 4, 4, 4, 4, 4, 4, 4,
     5, 5, 5, 5, 5, 5, 5, 5, 5,
     6, 6, 6, 6, 6, 6, 6, 6, 6,
     7, 7, 7, 7, 7, 7, 7, 7, 7,
     8, 8, 8, 8, 8, 8, 8, 8, 8
};

constexpr size_t col_per_index[81] = {
     0, 1, 2, 3, 4, 5, 6, 7, 8,
     0, 1, 2, 3, 4, 5, 6, 7, 8,
     0, 1, 2, 3, 4, 5, 6, 7, 8,
     0, 1, 2, 3, 4, 5, 6, 7, 8,
     0, 1, 2, 3, 4, 5, 6, 7, 8,
     0, 1, 2, 3, 4, 5, 6, 7, 8,
     0, 1, 2, 3, 4, 5, 6, 7, 8,
     0, 1, 2, 3, 4, 5, 6, 7, 8,
     0, 1, 2, 3, 4, 5, 6, 7, 8
};

constexpr size_t square_per_index[81] = {
     0, 0, 0, 1, 1, 1, 2, 2, 2,
     0, 0, 0, 1, 1, 1, 2, 2, 2,
     0, 0, 0, 1, 1, 1, 2, 2, 2,
     3, 3, 3, 4, 4, 4, 5, 5, 5,
     3, 3, 3, 4, 4, 4, 5, 5, 5,
     3, 3, 3, 4, 4, 4, 5, 5, 5,
     6, 6, 6, 7, 7, 7, 8, 8, 8,
     6, 6, 6, 7, 7, 7, 8, 8, 8,
     6, 6, 6, 7, 7, 7, 8, 8, 8
};

constexpr __uint128_t bit_mask_128  = (__uint128_t(0)) - 1;
constexpr __uint128_t bit_mask_81   = (__uint128_t(1) << 81) - 1;
constexpr __uint128_t bit_mask_64   = (__uint128_t(1) << 64) - 1;

struct sketch_notes_t {
    __uint128_t values_bitmask;
    uint64_t    rows_bitmask[9];
    uint64_t    cols_bitmask[9];
    uint64_t    squares_bitmask[9];
    char*       sudoku;
};

inline void print_sudoku(char* sudoku) {
    for (size_t fila = 0; fila < 9; ++fila) {
        for (size_t col = 0; col < 9; ++col)
            std::cout << sudoku[fila * 9 + col] << ' ';
        std::cout << '\n';
    }
}

inline bool find_solution(sketch_notes_t& s) noexcept {
    // Si ya no hay celdas libres, sudoku resuelto
    if (!s.values_bitmask)
        return true;

    // Copia para iterar sin modificar el original
    __uint128_t free_copy   = s.values_bitmask;
    uint64_t    best_count  = uint64_t(10);
    uint64_t    best_idx;

    // 1) Iterar sobre las casillas libres
    while (free_copy) {
        const uint64_t low  = uint64_t(free_copy);
        const uint64_t idx  = low ? __builtin_ctzll(low) : __builtin_ctzll(uint64_t(free_copy >> 64)) + 64;
        free_copy           &= free_copy - (__uint128_t)1;

        // Máscara de posibilidades: 1 = aún disponible
        const uint64_t pm = ~(s.rows_bitmask[row_per_index[idx]]
                            | s.cols_bitmask[col_per_index[idx]]
                            | s.squares_bitmask[square_per_index[idx]]) & uint64_t(0x1FF);

        // Si no hay posibilidades la hipótesis es incorrecta
        if (!pm)
            return false;

        // Si solo hay una posibilidad no va a haber ninguno con menos
        if (!(pm & (pm - 1))) {
            best_idx = idx;
            break;
        }

        // Contar número de posibilidades y comparar con el mejor
        const uint64_t cnt = __builtin_popcount(pm);
        if (cnt < best_count) {
            best_count = cnt;
            best_idx   = idx;
        }
    }

    // 2) Desmarcar la casilla como libre
    s.values_bitmask &= ~(__uint128_t(1) << best_idx);

    auto& rb = s.rows_bitmask[row_per_index[best_idx]];
    auto& cb = s.cols_bitmask[col_per_index[best_idx]];
    auto& sb = s.squares_bitmask[square_per_index[best_idx]];

    // Reconstruimos la máscara de posibilidades para la mejor casilla
    uint64_t pm = ~(rb | cb | sb) & uint64_t(0x1FF);

    // 3) Intentamos cada valor posible
    while (pm) {
        int         pos = __builtin_ctz(pm);    // Posición del bit: [0,8]
        uint64_t    m   = uint64_t(1) << pos;   // Bitmask única

        // Aplicar cambio
        rb |= m;
        cb |= m;
        sb |= m;

        // Si la hipótesis es correcta podemos anotarla y devolver true
        if (find_solution(s)) {
            s.sudoku[best_idx] = char('1' + pos);
            return true;
        }

        // Revertir cambio
        rb &= ~m;
        cb &= ~m;
        sb &= ~m;

        // Desmarcar bit para pasar al siguiente
        pm &= pm - 1;
    }

    // 4) Si ninguna fué posible, marcar casilla como libre de nuevo y devolver false
    s.values_bitmask |= (__uint128_t(1) << best_idx);
    return false;
}

inline void solve(char * sudoku) noexcept {
    sketch_notes_t s;   // Notas que nos servirán para resolverlo rápido
    memset(&s, 0, 232); // Inicializar todas las máscaras a 0
    s.sudoku = sudoku;  // Guardar puntero al array de caracteres

    // 1) Inicializar bitmasks con los valores y casillas ocupados
    for (size_t i = 0; i < 81; ++i) {
        const char c = sudoku[i];
        if (c != '.') {
            uint64_t m                              = uint64_t(1) << (c - '1');
            s.values_bitmask                        |= (__uint128_t(1) << i);
            s.rows_bitmask[row_per_index[i]]        |= m;
            s.cols_bitmask[col_per_index[i]]        |= m;
            s.squares_bitmask[square_per_index[i]]  |= m;
        }
    }

    // 2) Invertir values_bitmask: ahora 1 = libre
    s.values_bitmask = (~s.values_bitmask) & bit_mask_81;

    // 3) Resolver sudoku
    find_solution(s);
}

#endif // SUDOKU_SOLVER_HPP