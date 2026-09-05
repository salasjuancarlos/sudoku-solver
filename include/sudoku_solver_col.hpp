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

struct rcs_per_index_t {
    uint8_t row;
    uint8_t col;
    uint8_t sqr;
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

__uint128_t values_to_check = ~(__uint128_t)0;
uint32_t    rows_to_check   = 0;
uint32_t    cols_to_check   = 0;
uint32_t    sqrs_to_check   = 0;

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

inline void propagate(uint32_t row, uint32_t col, uint32_t sqr) {
    // propagar con SIMD calculando las máscaras con SIMD de cada uno y haciendo | y si solo hay una posibilidad (y no estaba ya resuelto) propaga desde ese.
}

inline void check_row(const uint8_t row, sketch_notes_t& s) {
    __m256i vrow = _mm256_set1_epi32(s.rows_bitmask[row]);
    __m256i vcol = _mm256_loadu_si256((const __m256i*)s.cols_bitmask);

    int sq0 = (row / 3) * 3;
    __m256i vsq0 = _mm256_set1_epi32(s.squares_bitmask[sq0]);
    __m256i vsq1 = _mm256_set1_epi32(s.squares_bitmask[sq0 + 1]);
    __m256i vsq2 = _mm256_set1_epi32(s.squares_bitmask[sq0 + 2]);

    __m256i temp  = _mm256_blend_epi32(vsq0, vsq1, 0b00111000);
    __m256i vsqr  = _mm256_blend_epi32(temp, vsq2, 0b11000000);

    __m256i vunion = _mm256_or_si256(_mm256_or_si256(vrow, vcol), vsqr);

    __m256i vcand  = _mm256_andnot_si256(vunion, _mm256_set1_epi32(0x1FF));

    __m256i v_nonzero  = _mm256_cmpgt_epi32(vcand, _mm256_setzero_si256());
    __m256i v_pm_minus = _mm256_sub_epi32(vcand, _mm256_set1_epi32(1));
    __m256i v_and      = _mm256_and_si256(vcand, v_pm_minus);
    __m256i v_is_single= _mm256_cmpeq_epi32(v_and, _mm256_setzero_si256());
    __m256i v_single   = _mm256_and_si256(v_nonzero, v_is_single);
    
    int mask = _mm256_movemask_ps(_mm256_castsi256_ps(v_single));
}

inline void check_col(const uint8_t col, sketch_notes_t& s) {

    // Buscar y aplicar naked singles
    for (uint64_t i = 0; i < 9; ++i) {
        uint64_t idx = col + i * 9;

        if (!(s.values_bitmask & ((__uint128_t)1 << idx))) continue;

        const auto& rcs     = indices_table[idx];
        auto& rb            = s.rows_bitmask[rcs.row];
        auto& cb            = s.cols_bitmask[rcs.col];
        auto& sb            = s.squares_bitmask[rcs.sqr];

        const uint64_t pm   = ~(rb | cb | sb) & 0x1FFULL;

        if (pm && !(pm & (pm - 1))) {
            s.values_bitmask &= ~(__uint128_t(1) << idx);
            rb |= pm;
            cb |= pm;
            sb |= pm;
            s.sudoku[idx] = char('1' + __builtin_ctz(pm));
        }
    }
    
    // Buscar hidden singles
    uint64_t saw_position[9];
    uint64_t once  = 0;
    uint64_t multi = 0;

    for (uint64_t i = 0; i < 9; ++i) {
        uint64_t idx = col + i * 9;

        if (!(s.values_bitmask & ((__uint128_t)1 << idx))) continue;

        const auto& rcs     = indices_table[idx];
        auto& rb            = s.rows_bitmask[rcs.row];
        auto& cb            = s.cols_bitmask[rcs.col];
        auto& sb            = s.squares_bitmask[rcs.sqr];

        const uint64_t pm   = ~(rb | cb | sb) & 0x1FFULL;

        uint32_t first_occurrence = pm & ~(once | multi);

        while (first_occurrence) {
            uint64_t value      = __builtin_ctzll(first_occurrence);
            saw_position[value] = idx;
            first_occurrence    &= first_occurrence - 1;
        }

        multi |= (once & pm);
        once  &= ~multi;
        once  |= pm & ~(multi | once);
    }

    // Aplicar hidden singles
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
        s.sudoku[idx]       = char('1' + __builtin_ctz(pm));
        once                &= once - 1;
    }
}

inline bool insta_solve_tmp(sketch_notes_t& s) noexcept {

    while (s.values_bitmask) {

        __uint128_t free_copy = s.values_bitmask & values_to_check;

        //values_to_check = 0;

        while (free_copy) [[LICKELY]] {

            const uint64_t low  = (uint64_t)free_copy;
            const uint64_t idx  = low ? __builtin_ctzll(low)
                                      : __builtin_ctzll((uint64_t)(free_copy >> 64)) + 64;
            free_copy &= free_copy - 1;

            const auto& rcs = indices_table[idx];
            auto& rb        = s.rows_bitmask[rcs.row];
            auto& cb        = s.cols_bitmask[rcs.col];
            auto& sb        = s.squares_bitmask[rcs.sqr];

            const uint64_t pm = ~(rb | cb | sb) & 0x1FFULL;

            if (pm && !(pm & (pm - 1))) {
                s.values_bitmask &= ~(__uint128_t(1) << idx);
                rb |= pm;
                cb |= pm;
                sb |= pm;
                s.sudoku[idx] = char('1' + __builtin_ctz(pm));
                /*printf("row:%u, col:%u, sqr:%u, value:%c\n", rcs.row, rcs.col, rcs.sqr, s.sudoku[idx]);
                print_sudoku(s.sudoku);
                write(STDOUT_FILENO, "\n", 1);*/
                //values_to_check |= check_bitmask_table[idx];
                check_col(rcs.col, s);
                break;
            }
        }
    }

    return s.values_bitmask == 0;
}

inline bool insta_solve(sketch_notes_t& s) noexcept {
    bool changed = true;
    int trys = 0;

    while (s.values_bitmask) {
        changed = false;
        __uint128_t free_copy = s.values_bitmask;  // recargar en cada pasada
        trys++;

        while (free_copy) {
            const uint64_t low  = (uint64_t)free_copy;
            const uint64_t idx  = low ? __builtin_ctzll(low)
                                      : __builtin_ctzll((uint64_t)(free_copy >> 64)) + 64;
            free_copy &= free_copy - 1;

            const auto& rcs = indices_table[idx];
            auto& rb = s.rows_bitmask[rcs.row];
            auto& cb = s.cols_bitmask[rcs.col];
            auto& sb = s.squares_bitmask[rcs.sqr];

            // Candidatos posibles (bits 0..8)
            const uint64_t pm = ~(rb | cb | sb) & 0x1FFULL;

            // Solo un candidato (y no es 0)
            if (pm && !(pm & (pm - 1))) {
                s.values_bitmask &= ~(__uint128_t(1) << idx);
                rb |= pm;
                cb |= pm;
                sb |= pm;
                s.sudoku[idx] = char('1' + __builtin_ctz(pm));
                //printf("row:%u, col:%u, sqr:%u\n", rcs.row, rcs.col, rcs.sqr);
                //print_sudoku(s.sudoku);
                //write(STDOUT_FILENO, "\n", 1);
                changed = true;
                break;  // reiniciar la pasada con las máscaras actualizadas
            }
        }
    }

    printf("trys:%u\n", trys);
    return s.values_bitmask == 0;  // true si todo resuelto
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
            const rcs_per_index_t rcs               = indices_table[i];
            s.rows_bitmask[rcs.row]                 |= m;
            s.cols_bitmask[rcs.col]                 |= m;
            s.squares_bitmask[rcs.sqr]               |= m;
        }
    }

    // 2) Invertir values_bitmask: ahora 1 = libre
    s.values_bitmask = (~s.values_bitmask) & bit_mask_81;

    // 3) Resolver sudoku
    insta_solve_tmp(s);
}

#endif // SUDOKU_SOLVER_HPP