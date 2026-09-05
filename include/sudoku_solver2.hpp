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

#include "precomp.hpp"
#include <stdint.h>
#include <cstring>
#include <immintrin.h>

struct alignas(64) cell_coords_t {
    uint64_t row;
    uint64_t col;
    uint64_t sqr;
};

constexpr cell_coords_t index_to_coords[81] = {
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

struct alignas(64) sudoku_state_t {
    __uint128_t unresolved_mask;    // bit i = 1 si la casilla i está sin resolver
    uint64_t    row_used[9];        // valores usados en cada fila
    uint64_t    col_used[9];        // valores usados en cada columna
    uint64_t    sqr_used[9];        // valores usados en cada caja 3x3
    uint64_t    dirty_rows;         // bits a 1 si la fila necesita reprocesado
    uint64_t    dirty_cols;         // bits a 1 si la columna necesita reprocesado
    uint64_t    dirty_sqrs;         // bits a 1 si la caja necesita reprocesado
};

static inline void print_sudoku(char* sudoku) noexcept {
    for (size_t fila = 0; fila < 9; ++fila) {
        for (size_t col = 0; col < 9; ++col)
            std::cout << sudoku[fila * 9 + col] << ' ';
        std::cout << '\n';
    }
}

static inline bool apply_naked_single(const uint64_t idx, sudoku_state_t &state, char *sudoku) noexcept {
    const cell_coords_t &coords = index_to_coords[idx];
    const uint64_t candidates_mask = ~(state.row_used[coords.row] | state.col_used[coords.col] | state.sqr_used[coords.sqr]) & 0x1FFULL;

    // Si no hay candidatos el estado es inconsistente
    if (!candidates_mask) [[unlikely]] return false;

    // Si una casilla solo tiene un candidato se coloca
    if (!(candidates_mask & (candidates_mask - 1ULL))) [[unlikely]] {
        const unsigned digit = __builtin_ctzll(candidates_mask);
        state.unresolved_mask &= ~(__uint128_t(1) << idx);
        state.row_used[coords.row] |= candidates_mask;
        state.col_used[coords.col] |= candidates_mask;
        state.sqr_used[coords.sqr] |= candidates_mask;
        sudoku[idx] = char('1' + digit);
        state.dirty_rows |= 1ULL << coords.row;
        state.dirty_cols |= 1ULL << coords.col;
        state.dirty_sqrs |= 1ULL << coords.sqr;
    }

    // Sin inconsistencias detectadas
    return true;
}

static inline bool check_hidden_single(uint64_t seen_position[9], uint64_t &seen_once, uint64_t &seen_multi, const uint64_t idx, sudoku_state_t &state) noexcept {
    const cell_coords_t &coords = index_to_coords[idx];
    const uint64_t candidates_mask = ~(state.row_used[coords.row] | state.col_used[coords.col] | state.sqr_used[coords.sqr]) & 0x1FFULL;

    // Si no hay candidatos el estado es inconsistente
    if (!candidates_mask) [[unlikely]] return false;

    // Los candidatos que no se habían visto antes son los que se han visto una vez
    uint64_t new_once = candidates_mask & ~(seen_once | seen_multi);

    // Registrar la posición de cada valor que aparece por primera vez
    uint64_t bits = new_once;
    while (bits) {
        uint64_t value = __builtin_ctzll(bits);
        seen_position[value] = idx;
        bits &= bits - 1;
    }

    // Actualizar máscaras
    seen_multi |= (seen_once & candidates_mask);        // Valores que ya estaban en once y aparecen de nuevo
    seen_once  = (seen_once & ~seen_multi) | new_once;  // Mantener los que siguen siendo únicos y añadir los nuevos

    // Sin inconsistencias detectadas
    return true;
}

static inline void apply_hiden_singles(uint64_t &seen_once, uint64_t seen_position[9], sudoku_state_t &state, char *sudoku) noexcept {
    // Por cada valor visto solo en una casilla se añade y se marca en dicha casilla
    while (seen_once) [[likely]] {
        uint64_t value = __builtin_ctzll(seen_once);
        seen_once &= seen_once-1;
        uint64_t idx = seen_position[value];
        const cell_coords_t &coords = index_to_coords[idx];
        const uint64_t candidates_mask = 1ULL << value;
        sudoku[idx] = char('1' + __builtin_ctz(candidates_mask));
        state.unresolved_mask &= ~(__uint128_t(1) << idx);
        state.row_used[coords.row] |= candidates_mask;
        state.col_used[coords.col] |= candidates_mask;
        state.sqr_used[coords.sqr] |= candidates_mask;
        state.dirty_rows |= 1ULL << coords.row;
        state.dirty_cols |= 1ULL << coords.col;
        state.dirty_sqrs |= 1ULL << coords.sqr;
    }
}

static inline bool propagate_row(const uint64_t row, sudoku_state_t &state, char *sudoku) noexcept {
    // Desmarcar como sucia
    state.dirty_rows &= ~(1ULL << row);

    // Buscar y aplicar naked singles
    const __uint128_t &cells_mask = row_to_cells_mask[row];
    __uint128_t cells_to_process = state.unresolved_mask & cells_mask;
    while (cells_to_process) [[likely]] {
        const uint64_t low = uint64_t(cells_to_process);
        const uint64_t idx = low ? __builtin_ctzll(low) : __builtin_ctzll(uint64_t(cells_to_process >> 64)) + 64;
        cells_to_process &= cells_to_process- (__uint128_t)1;
        if (!apply_naked_single(idx, state, sudoku)) [[unlikely]] return false;
    }

    // Buscar hidden singles
    uint64_t seen_position[9];
    uint64_t seen_once = 0;
    uint64_t seen_multi = 0;
    cells_to_process = state.unresolved_mask & cells_mask;
    while (cells_to_process) [[likely]] {
        const uint64_t low = uint64_t(cells_to_process);
        const uint64_t idx = low ? __builtin_ctzll(low) : __builtin_ctzll(uint64_t(cells_to_process >> 64)) + 64;
        cells_to_process &= cells_to_process - (__uint128_t)1;
        if (!check_hidden_single(seen_position, seen_once, seen_multi, idx, state)) return false;
    }

    // Aplicar hidden singles
    apply_hiden_singles(seen_once, seen_position, state, sudoku);

    // No se encontraron inconsistencias
    return true;
}

static inline bool propagate_col(const uint64_t col, sudoku_state_t &state, char *sudoku) noexcept {
    // Desmarcar como sucia
    state.dirty_cols &= ~(1ULL << col);

    // Buscar y aplicar naked singles
    const __uint128_t &cells_mask = col_to_cells_mask[col];
    __uint128_t cells_to_process = state.unresolved_mask & cells_mask;
    while (cells_to_process) [[likely]] {
        const uint64_t low = uint64_t(cells_to_process);
        const uint64_t idx = low ? __builtin_ctzll(low) : __builtin_ctzll(uint64_t(cells_to_process >> 64)) + 64;
        cells_to_process &= cells_to_process - (__uint128_t)1;
        if (!apply_naked_single(idx, state, sudoku)) [[unlikely]] return false;
    }

    // Buscar hidden singles
    uint64_t seen_position[9];
    uint64_t seen_once = 0ULL;
    uint64_t seen_multi = 0ULL;
    cells_to_process = state.unresolved_mask & cells_mask;
    while (cells_to_process) [[likely]] {
        const uint64_t low = uint64_t(cells_to_process);
        const uint64_t idx = low ? __builtin_ctzll(low) : __builtin_ctzll(uint64_t(cells_to_process >> 64)) + 64;
        cells_to_process &= cells_to_process - (__uint128_t)1;
        if (!check_hidden_single(seen_position, seen_once, seen_multi, idx, state)) [[unlikely]] return false;
    }

    // Aplicar hidden singles
    apply_hiden_singles(seen_once, seen_position, state, sudoku);

    // No se encontraron inconsistencias
    return true;
}

static inline bool propagate_sqr(const uint64_t sqr, sudoku_state_t &state, char *sudoku) noexcept {
    // Desmarcar como sucia
    state.dirty_sqrs &= ~(1ULL << sqr);

    // Buscar y aplicar naked singles
    const __uint128_t &cells_mask = sqr_to_cells_mask[sqr];
    __uint128_t cells_to_process = state.unresolved_mask & cells_mask;
    while (cells_to_process) [[likely]] {
        const uint64_t low = uint64_t(cells_to_process);
        const uint64_t idx = low ? __builtin_ctzll(low) : __builtin_ctzll(uint64_t(cells_to_process >> 64)) + 64;
        cells_to_process &= cells_to_process - (__uint128_t)1;
        if (!apply_naked_single(idx, state, sudoku)) [[unlikely]] return false;
    }

    // Buscar hidden singles
    uint64_t seen_position[9];
    uint64_t seen_once = 0;
    uint64_t seen_multi = 0;
    cells_to_process = state.unresolved_mask & cells_mask;
    while (cells_to_process) [[likely]] {
        const uint64_t low = uint64_t(cells_to_process);
        const uint64_t idx = low ? __builtin_ctzll(low) : __builtin_ctzll(uint64_t(cells_to_process >> 64)) + 64;
        cells_to_process &= cells_to_process - (__uint128_t)1;
        if (!check_hidden_single(seen_position, seen_once, seen_multi, idx, state)) [[unlikely]] return false;
    }

    // Aplicar hidden singles
    apply_hiden_singles(seen_once, seen_position, state, sudoku);

    // No se encontraron inconsistencias
    return true;
}

static inline bool propagate_singles(sudoku_state_t &state, char *sudoku) noexcept {
    // Mientras no esté resuelto y queden unidades sucias
    while (state.unresolved_mask && (state.dirty_cols | state.dirty_rows | state.dirty_sqrs)) [[likely]] {
        // Recorrer filas
        while (state.dirty_rows && state.unresolved_mask) [[likely]] {
            const uint64_t row = __builtin_ctzll(state.dirty_rows);
            if (!propagate_row(row, state, sudoku)) [[unlikely]] return false;
        }

        // Recorrer columnas
        while (state.dirty_cols && state.unresolved_mask) [[likely]] {
            const uint64_t col = __builtin_ctzll(state.dirty_cols);
            if (!propagate_col(col, state, sudoku)) [[unlikely]] return false;
        }

        // Recorrer cajas
        while (state.dirty_sqrs && state.unresolved_mask) [[likely]] {
            const uint64_t sqr = __builtin_ctzll(state.dirty_sqrs);
            if (!propagate_sqr(sqr, state, sudoku)) [[unlikely]] return false;
        }
    }

    return true;
}

static inline bool find_solution(sudoku_state_t &state, char *sudoku) noexcept {
    // Si ya no hay celdas libres, sudoku resuelto
    if (!state.unresolved_mask) return true;

    // 1) BÚSQUEDA DE SINGLES (ANTES DE "PROBAR" VALORES)
    if (!propagate_singles(state, sudoku)) return false;

    // Volver a comprobar, si ya no hay celdas libres, sudoku resuelto
    if (!state.unresolved_mask) return true;

    // 2) BÚSQUEDA DE LA CASILLA LIBRE CON MENOS CANDIDATOS
    __uint128_t remaining_mask = state.unresolved_mask;
    uint64_t min_candidates = uint64_t(10);
    uint64_t best_idx;
    while (remaining_mask) {
        // Obtener el índice de casilla y sus coordenadas y marcar como procesada
        const uint64_t low_mask = uint64_t(remaining_mask);
        const uint64_t idx = low_mask ? __builtin_ctzll(low_mask) : __builtin_ctzll(uint64_t(remaining_mask >> 64)) + 64;
        remaining_mask &= remaining_mask - (__uint128_t)1;
        const cell_coords_t &coords = index_to_coords[idx];

        // Máscara de posibilidades: 1 = aún disponible
        const uint64_t candidates_mask = ~(state.row_used[coords.row] | state.col_used[coords.col] | state.sqr_used[coords.sqr]) & 0x1FFULL;

        // Contar número de posibilidades y comparar con el mejor (si ya son 2 no puede haber menos porque sino sería naked single)
        const uint64_t n_candidates = __builtin_popcount(candidates_mask);
        if (n_candidates < min_candidates) {
            min_candidates = n_candidates;
            best_idx = idx;
            if (n_candidates == 2) break;
        }
    }

    // Desmarcar la casilla como libre, obtener sus coordenadas y anotar sus unidades como sucias
    state.unresolved_mask &= ~(__uint128_t(1) << best_idx);
    const cell_coords_t &coords = index_to_coords[best_idx];
    state.dirty_rows |= 1ULL << coords.row;
    state.dirty_cols |= 1ULL << coords.col;
    state.dirty_sqrs |= 1ULL << coords.sqr;
    auto &row_used = state.row_used[coords.row];
    auto &col_used = state.col_used[coords.col];
    auto &sqr_used = state.sqr_used[coords.sqr];

    // Reconstruir la máscara de candidatos para la mejor casilla
    uint64_t candidates_mask = ~(row_used | col_used | sqr_used) & 0x1FFULL;

    // Guardar estado actual antes de probar candidatos
    const sudoku_state_t state_before_guess = state;

    // 3) PROBAR CANDIDATOS
    while (candidates_mask) {
        uint64_t pos = __builtin_ctzll(candidates_mask);    // Posición del bit: [0,8]

        // Actualizar estado de unidades para este candidato
        uint64_t candidate_mask = uint64_t(1) << pos;
        row_used |= candidate_mask;
        col_used |= candidate_mask;
        sqr_used |= candidate_mask;

        // Comprobar si se puede resolver con ese candidato
        if (find_solution(state, sudoku)) {
            sudoku[best_idx] = char('1' + pos);
            return true;
        }

        // Revertir estado al de antes de probar candidatos
        state = state_before_guess;

        // Candidato descartado
        candidates_mask &= candidates_mask - 1;
    }

    // Si todos los candidatos producen un estado inconsistente, significa que partimos de un estado inconsistente
    return false;
}

static inline void solve(char *sudoku) noexcept {
    sudoku_state_t state; // Estado inicial

    // 1) INICIALIZAR MÁSCARAS
    constexpr uint64_t fill_size = sizeof(state) - (sizeof(uint64_t) << 1) + sizeof(uint64_t); // Todo lo que tenemos que iniciar a 1
    memset(&state, 0, fill_size);
    state.dirty_rows = 0x1FF;
    state.dirty_cols = 0x1FF;
    state.dirty_sqrs = 0x1FF;

    // 2) MARCAR VALORES OCUPADOS Y CASILLAS LIBRES
    for (uint64_t i = 0; i < 81; ++i) {
        const char c = sudoku[i];
        if (c != '.') {
            const uint64_t m = uint64_t(1) << (c - '1');

            // Marcar casilla como usada (luego invertiremos toda la máscara)
            state.unresolved_mask |= (__uint128_t(1) << i);

            // Marcar el valor de la casilla como usado en fila, columna y caja
            const cell_coords_t &coords = index_to_coords[i];
            state.row_used[coords.row] |= m;
            state.col_used[coords.col] |= m;
            state.sqr_used[coords.sqr] |= m;
        }
    }

    constexpr __uint128_t bit_mask_81 = (__uint128_t(1) << 81) - 1;
    state.unresolved_mask = (~state.unresolved_mask) & bit_mask_81; // Invertimos porque queremos (1 = libre)

    // 3) RESOLVER SUDOKU
    find_solution(state, sudoku);
}

#endif // SUDOKU_SOLVER_HPP