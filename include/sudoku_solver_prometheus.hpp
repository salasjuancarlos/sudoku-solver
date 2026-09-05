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
#include <immintrin.h>
#include "precompiled.hpp"

struct alignas(64) rcs_per_index_t {
    uint64_t row;
    uint64_t col;
    uint64_t sqr;
};

constexpr rcs_per_index_t indices_table[81] = {
    {0, 0, 0}, {0, 1, 0}, {0, 2, 0},
    {0, 3, 1}, {0, 4, 1}, {0, 5, 1},
    {0, 6, 2}, {0, 7, 2}, {0, 8, 2},
    {1, 0, 0}, {1, 1, 0}, {1, 2, 0},
    {1, 3, 1}, {1, 4, 1}, {1, 5, 1},
    {1, 6, 2}, {1, 7, 2}, {1, 8, 2},
    {2, 0, 0}, {2, 1, 0}, {2, 2, 0},
    {2, 3, 1}, {2, 4, 1}, {2, 5, 1},
    {2, 6, 2}, {2, 7, 2}, {2, 8, 2},
    {3, 0, 3}, {3, 1, 3}, {3, 2, 3},
    {3, 3, 4}, {3, 4, 4}, {3, 5, 4},
    {3, 6, 5}, {3, 7, 5}, {3, 8, 5},
    {4, 0, 3}, {4, 1, 3}, {4, 2, 3},
    {4, 3, 4}, {4, 4, 4}, {4, 5, 4},
    {4, 6, 5}, {4, 7, 5}, {4, 8, 5},
    {5, 0, 3}, {5, 1, 3}, {5, 2, 3},
    {5, 3, 4}, {5, 4, 4}, {5, 5, 4},
    {5, 6, 5}, {5, 7, 5}, {5, 8, 5},
    {6, 0, 6}, {6, 1, 6}, {6, 2, 6},
    {6, 3, 7}, {6, 4, 7}, {6, 5, 7},
    {6, 6, 8}, {6, 7, 8}, {6, 8, 8},
    {7, 0, 6}, {7, 1, 6}, {7, 2, 6},
    {7, 3, 7}, {7, 4, 7}, {7, 5, 7},
    {7, 6, 8}, {7, 7, 8}, {7, 8, 8},
    {8, 0, 6}, {8, 1, 6}, {8, 2, 6},
    {8, 3, 7}, {8, 4, 7}, {8, 5, 7},
    {8, 6, 8}, {8, 7, 8}, {8, 8, 8}
};

constexpr __uint128_t bit_mask_128  = (__uint128_t(0)) - 1;
constexpr __uint128_t bit_mask_81   = (__uint128_t(1) << 81) - 1;
constexpr __uint128_t bit_mask_64   = (__uint128_t(1) << 64) - 1;

struct alignas(64) sketch_notes_t {
    __uint128_t values_bitmask;
    uint64_t    rows_bitmask[9];
    uint64_t    cols_bitmask[9];
    uint64_t    squares_bitmask[9];
    uint64_t    dirty_rows = 0x1FF;
    uint64_t    dirty_cols = 0x1FF;
    uint64_t    dirty_sqrs = 0x1FF;
};

static inline void print_sudoku(char* sudoku) noexcept {
    for (size_t fila = 0; fila < 9; ++fila) {
        for (size_t col = 0; col < 9; ++col)
            std::cout << sudoku[fila * 9 + col] << ' ';
        std::cout << '\n';
    }
}

static inline void check_naked_single(const uint64_t idx, sketch_notes_t& s, char* sudoku) noexcept {
    if (!(s.values_bitmask & ((__uint128_t)1 << idx))) return;

    const auto& rcs     = indices_table[idx];
    auto& rb            = s.rows_bitmask[rcs.row];
    auto& cb            = s.cols_bitmask[rcs.col];
    auto& sb            = s.squares_bitmask[rcs.sqr];

    const uint64_t pm   = ~(rb | cb | sb) & 0x1FFULL;

    // Comprobar si es naked single
    if (pm && !(pm & (pm - 1ULL))) {
        const unsigned digit = __builtin_ctzll(pm);
        s.values_bitmask &= ~(__uint128_t(1) << idx);
        rb |= pm;
        cb |= pm;
        sb |= pm;
        sudoku[idx] = char('1' + digit);

        // Marcar fila y caja como sucias
        s.dirty_rows |= 1ULL << rcs.row;
        s.dirty_cols |= 1ULL << rcs.col;
        s.dirty_sqrs |= 1ULL << rcs.sqr;
    }
}

static inline void check_hidden_single(uint64_t saw_position[9], uint64_t& once, uint64_t& multi, const uint64_t idx, sketch_notes_t& s) noexcept {
    if (!(s.values_bitmask & ((__uint128_t)1 << idx))) return;

    const auto& rcs     = indices_table[idx];
    auto& rb            = s.rows_bitmask[rcs.row];
    auto& cb            = s.cols_bitmask[rcs.col];
    auto& sb            = s.squares_bitmask[rcs.sqr];

    const uint64_t pm   = ~(rb | cb | sb) & 0x1FFULL;

    uint64_t first_occurrence = pm & ~(once | multi);

    while (first_occurrence) {
        uint64_t value      = __builtin_ctzll(first_occurrence);
        saw_position[value] = idx;
        first_occurrence    &= first_occurrence - 1;
    }

    multi |= (once & pm);
    once  &= ~multi;
    once  |= pm & ~(multi | once);
}

static inline void apply_hiden_singles(uint64_t& once, uint64_t saw_position[9], sketch_notes_t& s, char* sudoku) noexcept {
    while (once) {
        uint64_t value      = __builtin_ctzll(once);
        uint64_t idx        = saw_position[value];
        const uint64_t pm   = 1ULL << value;
        const auto& rcs     = indices_table[idx];
        auto& rb            = s.rows_bitmask[rcs.row];
        auto& cb            = s.cols_bitmask[rcs.col];
        auto& sb            = s.squares_bitmask[rcs.sqr];
        s.values_bitmask    &= ~(__uint128_t(1) << idx);
        rb                  |= pm;
        cb                  |= pm;
        sb                  |= pm;
        sudoku[idx]       = char('1' + __builtin_ctz(pm));
        once                &= once - 1;

        // Marcar fila y caja como sucias
        s.dirty_rows |= 1ULL << rcs.row;
        s.dirty_cols |= 1ULL << rcs.col;
        s.dirty_sqrs |= 1ULL << rcs.sqr;
    }
}

static inline void check_row(const uint64_t row, sketch_notes_t& s, char* sudoku) noexcept {
    // Desmarcar como sucia
    s.dirty_rows &= ~(1ULL << row);

    // Buscar y aplicar naked singles
    const uint64_t idx_min = row * 9ULL;
    const uint64_t idx_max = idx_min + 8ULL;
    for (uint64_t idx = idx_min; idx <= idx_max; ++idx)
        check_naked_single(idx, s, sudoku);
    
    // Buscar hidden singles
    uint64_t saw_position[9];
    uint64_t once  = 0;
    uint64_t multi = 0;

    for (uint64_t idx = idx_min; idx <= idx_max; ++idx)
        check_hidden_single(saw_position, once, multi, idx, s);

    // Aplicar hidden singles
    apply_hiden_singles(once, saw_position, s, sudoku);
}

static inline void check_col(const uint64_t col, sketch_notes_t& s, char* sudoku) noexcept {
    // Desmarcar como sucia
    s.dirty_cols &= ~(1ULL << col);

    // Buscar y aplicar naked singles
    const uint64_t idx_max = col + 72ULL;
    for (uint64_t idx = col; idx <= idx_max; idx += 9ULL)
        check_naked_single(idx, s, sudoku);
    
    // Buscar hidden singles
    uint64_t saw_position[9];
    uint64_t once  = 0ULL;
    uint64_t multi = 0ULL;

    for (uint64_t idx = col; idx <= idx_max; idx += 9ULL)
        check_hidden_single(saw_position, once, multi, idx, s);

    // Aplicar hidden singles
    apply_hiden_singles(once, saw_position, s, sudoku);
}

static inline void check_sqr(const uint64_t sqr, sketch_notes_t& s, char* sudoku) noexcept {
    // Desmarcar como sucia
    s.dirty_sqrs &= ~(1ULL << sqr);

    // Buscar y aplicar naked singles
    static constexpr uint64_t idx_mins[]    = { 0, 3, 6, 27, 30, 33, 54, 57, 60 };
    static constexpr uint64_t deltas[]      = { 0, 1, 2, 9, 10, 11, 18, 19, 20 };

    const uint64_t idx_min = idx_mins[sqr];
    for (uint64_t i = 0; i <= 8; ++i) {
        const uint64_t idx = idx_min + deltas[i];
        check_naked_single(idx, s, sudoku);
    }
    
    // Buscar hidden singles
    uint64_t saw_position[9];
    uint64_t once  = 0;
    uint64_t multi = 0;

    for (uint64_t i = 0; i <= 8; ++i) {
        const uint64_t idx = idx_min + deltas[i];
        check_hidden_single(saw_position, once, multi, idx, s);
    }

    // Aplicar hidden singles
    apply_hiden_singles(once, saw_position, s, sudoku);
}

static inline void search_singles(sketch_notes_t& s, char* sudoku) noexcept {
    while (s.dirty_cols && s.values_bitmask) [[likely]] {
        uint64_t col = __builtin_ctzll(s.dirty_cols);
        check_col(col, s, sudoku);
    }

    while (s.dirty_rows && s.values_bitmask) [[likely]] {
        uint64_t row = __builtin_ctzll(s.dirty_rows);
        check_row(row, s, sudoku);
    }

    while (s.dirty_sqrs && s.values_bitmask) [[likely]] {
        uint64_t sqr = __builtin_ctzll(s.dirty_sqrs);
        check_sqr(sqr, s, sudoku);
    }
}

static inline bool find_solution(sketch_notes_t& s, char* sudoku) noexcept {
    // Si ya no hay celdas libres, sudoku resuelto
    if (!s.values_bitmask)
        return true;

    // Guadar el estado actual
    const sketch_notes_t sketch_notes_copy = s;

    // Buscar todos los hidden y naked singles antes de "probar" valores
    search_singles(s, sudoku);

    // Volve a comprobar, si ya no hay celdas libres, sudoku resuelto
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

        const rcs_per_index_t rcs = indices_table[idx];

        // Máscara de posibilidades: 1 = aún disponible
        const uint64_t pm = ~(s.rows_bitmask[rcs.row]
                            | s.cols_bitmask[rcs.col]
                            | s.squares_bitmask[rcs.sqr]) & 0x1FFULL;

        // Si no hay posibilidades la hipótesis es incorrecta
        if (!pm)
            return false;

        // Contar número de posibilidades y comparar con el mejor
        const uint64_t cnt = __builtin_popcount(pm);
        if (cnt < best_count) {
            best_count = cnt;
            best_idx   = idx;
        }
    }

    // 2) Desmarcar la casilla como libre
    s.values_bitmask &= ~(__uint128_t(1) << best_idx);

    const rcs_per_index_t rcs = indices_table[best_idx];

    auto& rb = s.rows_bitmask[rcs.row];
    auto& cb = s.cols_bitmask[rcs.col];
    auto& sb = s.squares_bitmask[rcs.sqr];

    // Reconstruimos la máscara de posibilidades para la mejor casilla
    uint64_t pm = ~(rb | cb | sb) & 0x1FFULL;

    // 3) Intentamos cada valor posible
    while (pm) {
        uint64_t pos = __builtin_ctzll(pm); // Posición del bit: [0,8]
        uint64_t m   = uint64_t(1) << pos;   // Bitmask única

        // Aplicar cambio
        rb |= m;
        cb |= m;
        sb |= m;

        // Si la hipótesis es correcta podemos anotarla y devolver true
        if (find_solution(s, sudoku)) {
            sudoku[best_idx] = char('1' + pos);
            return true;
        }

        // Revertir cambio
        rb &= ~m;
        cb &= ~m;
        sb &= ~m;

        // Desmarcar bit para pasar al siguiente
        pm &= pm - 1;
    }

    // 4) Si ninguna fué posible, recuperar el estado inicial y devolver false (el estado es inconsistente)
    s = sketch_notes_copy;
    return false;
}

static inline void solve(char * sudoku) noexcept {
    sketch_notes_t s;           // Notas que nos servirán para resolverlo rápido
    memset(&s, 0, sizeof(s));   // Inicializar todas las máscaras a 0
    s.dirty_rows = 0x1FF;
    s.dirty_cols = 0x1FF;
    s.dirty_sqrs = 0x1FF;

    // 1) Inicializar bitmasks con los valores y casillas ocupados
    for (size_t i = 0; i < 81; ++i) {
        const char c = sudoku[i];
        if (c != '.') {
            uint64_t m                              = uint64_t(1) << (c - '1');
            s.values_bitmask                        |= (__uint128_t(1) << i);
            const rcs_per_index_t rcs               = indices_table[i];
            s.rows_bitmask[rcs.row]                 |= m;
            s.cols_bitmask[rcs.col]                 |= m;
            s.squares_bitmask[rcs.sqr]              |= m;
        }
    }

    // 2) Invertir values_bitmask: ahora 1 = libre
    s.values_bitmask = (~s.values_bitmask) & bit_mask_81;

    // 3) Resolver sudoku
    find_solution(s, sudoku);
}

#endif // SUDOKU_SOLVER_HPP