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

constexpr __uint128_t bit_mask_128  = (__uint128_t(0)) - 1;
constexpr __uint128_t bit_mask_81   = (__uint128_t(1) << 81) - 1;
constexpr __uint128_t bit_mask_64   = (__uint128_t(1) << 64) - 1;

struct alignas(64) sudoku_state_t {
    __uint128_t unresolved_mask;   // bit i = 1 si la casilla i está sin resolver
    uint64_t    row_candidates[9]; // valores disponibles en cada fila
    uint64_t    col_candidates[9]; // valores disponibles en cada columna
    uint64_t    sqr_candidates[9]; // valores disponibles en cada caja 3x3
    uint64_t    dirty_rows;        // bits a 1 si la fila necesita reprocesado
    uint64_t    dirty_cols;        // bits a 1 si la columna necesita reprocesado
    uint64_t    dirty_sqrs;        // bits a 1 si la caja necesita reprocesado
};

static inline void print_sudoku(char* sudoku) noexcept {
    for (size_t fila = 0; fila < 9; ++fila) {
        for (size_t col = 0; col < 9; ++col)
            std::cout << sudoku[fila * 9 + col] << ' ';
        std::cout << '\n';
    }
}

static inline bool apply_naked_single(const uint64_t cell_idx, sudoku_state_t &state, char *sudoku) noexcept {
    // Obtener unidades y candidatos
    const cell_coords_t &coords = index_to_coords[cell_idx];
    auto &row_candidates = state.row_candidates[coords.row];
    auto &col_candidates = state.col_candidates[coords.col];
    auto &sqr_candidates = state.sqr_candidates[coords.sqr];
    uint64_t candidates_mask = state.row_candidates[coords.row] & state.col_candidates[coords.col] & state.sqr_candidates[coords.sqr];

    // Si no hay candidatos el estado es inconsistente
    if (!candidates_mask) [[unlikely]] return false;

    // Si solo hay un candidato lo marcamos y añadimos las unidades a reprocesar
    if (!(candidates_mask & (candidates_mask - 1ULL))) [[unlikely]] {
        const unsigned candidate = __builtin_ctzll(candidates_mask);
        state.unresolved_mask &= ~(__uint128_t(1) << cell_idx);
        const uint64_t clear_mask  = ~(uint64_t(1) << candidate); // Bitmask con bit de candidato a 0
        row_candidates &= clear_mask;
        col_candidates &= clear_mask;
        sqr_candidates &= clear_mask;
        sudoku[cell_idx] = char('1' + candidate);
        state.dirty_rows |= 1ULL << coords.row;
        state.dirty_cols |= 1ULL << coords.col;
        state.dirty_sqrs |= 1ULL << coords.sqr;
    }

    // No hay inconsistencia detectada
    return true;
}

static inline bool track_hidden_single(uint64_t seen_pos[9], uint64_t &seen_once, uint64_t &seen_multi, const uint64_t cell_idx, sudoku_state_t &state) noexcept {
    // Obtener candidatos
    const cell_coords_t &coords = index_to_coords[cell_idx];
    const uint64_t candidates_mask = state.row_candidates[coords.row] & state.col_candidates[coords.col] & state.sqr_candidates[coords.sqr];

    // Si no hay candidatos el estado es inconsistente
    if (!candidates_mask) [[unlikely]] return false;

    // Si los candidatos no se han visto ni una vez ni varias es la primera aparición
    uint64_t first_occurrence = candidates_mask & ~(seen_once | seen_multi);

    // Anotar donde se vieron por primera vez
    while (first_occurrence) [[unlikely]] {
        uint64_t value = __builtin_ctzll(first_occurrence);
        seen_pos[value] = cell_idx;
        first_occurrence &= first_occurrence - 1;
    }

    seen_multi |= (seen_once & candidates_mask);                // Si se habían visto y se velven a ver se vieron múltiples veces
    seen_once &= ~seen_multi;                                   // Si se han visto multiples veces no se han visto solo una vez
    seen_once |= candidates_mask & ~(seen_multi | seen_once);   // Añadir los candidatos que no se habían visto antes

    // No hay inconsistencia detectada
    return true;
}

static inline void apply_hiden_singles(uint64_t &seen_once, uint64_t seen_pos[9], sudoku_state_t &state, char *sudoku) noexcept {
    while (seen_once) [[likely]] {
        uint64_t digit = __builtin_ctzll(seen_once);
        uint64_t cell_idx = seen_pos[digit];
        const uint64_t candidates_mask = 1ULL << digit;
        const cell_coords_t &coords = index_to_coords[cell_idx];
        auto &row_candidates = state.row_candidates[coords.row];
        auto &col_candidates = state.col_candidates[coords.col];
        auto &sqr_candidates = state.sqr_candidates[coords.sqr];
        state.unresolved_mask &= ~(__uint128_t(1) << cell_idx);
        uint64_t clear_mask = ~(candidates_mask);
        row_candidates &= clear_mask;
        col_candidates &= clear_mask;
        sqr_candidates &= clear_mask;
        sudoku[cell_idx] = char('1' + __builtin_ctz(candidates_mask));
        state.dirty_rows |= 1ULL << coords.row;
        state.dirty_cols |= 1ULL << coords.col;
        state.dirty_sqrs |= 1ULL << coords.sqr;
        seen_once &= seen_once - 1;
    }
}

static inline bool propagate_row(const uint64_t row, sudoku_state_t &state, char *sudoku) noexcept {
    // Desmarcar como sucia
    state.dirty_rows &= ~(1ULL << row);

    // Buscar y aplicar naked singles
    const __uint128_t &row_cells_masks = row_to_cells_masks[row];
    __uint128_t cells_to_process = state.unresolved_mask & row_cells_masks;
    while (cells_to_process) [[likely]] {
        const uint64_t low = uint64_t(cells_to_process);
        const uint64_t cell_idx = low ? __builtin_ctzll(low) : __builtin_ctzll(uint64_t(cells_to_process >> 64)) + 64;
        if (!apply_naked_single(cell_idx, state, sudoku)) [[unlikely]] return false;
        cells_to_process &= cells_to_process - (__uint128_t)1;
    }

    // Buscar hidden singles
    uint64_t seen_pos[9];
    uint64_t seen_once = 0;
    uint64_t seen_multi = 0;
    cells_to_process = state.unresolved_mask & row_cells_masks;
    while (cells_to_process) [[likely]] {
        const uint64_t low = uint64_t(cells_to_process);
        const uint64_t cell_idx = low ? __builtin_ctzll(low) : __builtin_ctzll(uint64_t(cells_to_process >> 64)) + 64;
        if (!track_hidden_single(seen_pos, seen_once, seen_multi, cell_idx, state)) return false;
        cells_to_process &= cells_to_process - (__uint128_t)1;
    }

    // Aplicar hidden singles
    apply_hiden_singles(seen_once, seen_pos, state, sudoku);

    return true;
}

static inline bool propagate_col(const uint64_t col, sudoku_state_t &state, char *sudoku) noexcept {
    // Desmarcar como sucia
    state.dirty_cols &= ~(1ULL << col);

    // Buscar y aplicar naked singles
    const __uint128_t &col_cells_masks = col_to_cells_masks[col];
    __uint128_t cells_to_process = state.unresolved_mask & col_cells_masks;
    while (cells_to_process) [[likely]] {
        const uint64_t low = uint64_t(cells_to_process);
        const uint64_t cell_idx = low ? __builtin_ctzll(low) : __builtin_ctzll(uint64_t(cells_to_process >> 64)) + 64;
        if (!apply_naked_single(cell_idx, state, sudoku)) [[unlikely]] return false;
        cells_to_process &= cells_to_process - (__uint128_t)1;
    }

    // Buscar hidden singles
    uint64_t seen_pos[9];
    uint64_t seen_once = 0ULL;
    uint64_t seen_multi = 0ULL;
    cells_to_process = state.unresolved_mask & col_cells_masks;
    while (cells_to_process) [[likely]] {
        const uint64_t low = uint64_t(cells_to_process);
        const uint64_t cell_idx = low ? __builtin_ctzll(low) : __builtin_ctzll(uint64_t(cells_to_process >> 64)) + 64;
        if (!track_hidden_single(seen_pos, seen_once, seen_multi, cell_idx, state)) [[unlikely]] return false;
        cells_to_process &= cells_to_process - (__uint128_t)1;
    }

    // Aplicar hidden singles
    apply_hiden_singles(seen_once, seen_pos, state, sudoku);

    // No se encontraron inconsistencias
    return true;
}

static inline bool propagate_sqr(const uint64_t sqr, sudoku_state_t &state, char *sudoku) noexcept {
    // Desmarcar como sucia
    state.dirty_sqrs &= ~(1ULL << sqr);

    // Buscar y aplicar naked singles
    const __uint128_t &sqr_cells_mask = sqr_to_cells_masks[sqr];
    __uint128_t cells_to_process = state.unresolved_mask & sqr_cells_mask;
    while (cells_to_process) [[likely]] {
        const uint64_t low = uint64_t(cells_to_process);
        const uint64_t cell_idx = low ? __builtin_ctzll(low) : __builtin_ctzll(uint64_t(cells_to_process >> 64)) + 64;
        if (!apply_naked_single(cell_idx, state, sudoku)) [[unlikely]] return false;
        cells_to_process &= cells_to_process - (__uint128_t)1;
    }

    // Buscar hidden singles
    uint64_t saw_position[9];
    uint64_t seen_once = 0;
    uint64_t seen_multiple = 0;
    cells_to_process = state.unresolved_mask & sqr_cells_mask;
    while (cells_to_process) [[likely]] {
        const uint64_t low = uint64_t(cells_to_process);
        const uint64_t cell_idx = low ? __builtin_ctzll(low) : __builtin_ctzll(uint64_t(cells_to_process >> 64)) + 64;
        if (!track_hidden_single(saw_position, seen_once, seen_multiple, cell_idx, state)) [[unlikely]] return false;
        cells_to_process &= cells_to_process - (__uint128_t)1;
    }

    // Aplicar hidden singles
    apply_hiden_singles(seen_once, saw_position, state, sudoku);

    // No se encontraron inconsistencias
    return true;
}

static inline bool propagate_singles(sudoku_state_t &state, char *sudoku) noexcept {
    // Mientras no este resuelto y haya unidades sucias resolver singles
    while (state.unresolved_mask && (state.dirty_rows | state.dirty_cols | state.dirty_sqrs)) [[likely]] {
        while (state.unresolved_mask && state.dirty_sqrs) [[likely]]
            if (!propagate_sqr(__builtin_ctzll(state.dirty_sqrs), state, sudoku)) [[unlikely]] return false; // Detectó inconsistencia

        while (state.unresolved_mask && state.dirty_rows) [[likely]]
            if (!propagate_row(__builtin_ctzll(state.dirty_rows), state, sudoku)) [[unlikely]] return false; // Detectó inconsistencia

        while (state.unresolved_mask && state.dirty_cols) [[likely]]
            if (!propagate_col(__builtin_ctzll(state.dirty_cols), state, sudoku)) [[unlikely]] return false; // Detectó inconsistencia
    }

    return true;
}

static inline bool find_solution(sudoku_state_t &state, char *sudoku) noexcept {
    // Si ya no hay celdas libres, sudoku resuelto
    if (!state.unresolved_mask) return true;

    // 1) BÚSQUEDA DE SINGLES
    if (!propagate_singles(state, sudoku)) return false;

    // Volver a comprobar, si ya no hay celdas libres, sudoku resuelto
    if (!state.unresolved_mask) return true;

    // 2) BÚSQUEDA DE LA CASILLA LIBRE CON MENOS CANDIDATOS
    __uint128_t remaining_mask = state.unresolved_mask; // Casillas libres aún procesar
    uint64_t min_candidates = uint64_t(10);             // Menor número de candidatos encontrado
    uint64_t best_cell_idx;                             // Casilla más prometedora
    while (remaining_mask) {
        // Obtener el índice de casilla y sus coordenadas y luego marcar como procesada
        const uint64_t low = uint64_t(remaining_mask);
        const uint64_t cell_idx = low ? __builtin_ctzll(low) : __builtin_ctzll(uint64_t(remaining_mask >> 64)) + 64;
        const cell_coords_t &coords = index_to_coords[cell_idx];

        // Máscara de posibilidades: 1 = aún disponible
        const uint64_t candidates_mask = state.row_candidates[coords.row] & state.col_candidates[coords.col] & state.sqr_candidates[coords.sqr];

        // Contar número de posibilidades y comparar con el mejor (si ya son 2 no puede haber menos porque sino sería naked single)
        const uint64_t n_candidates = __builtin_popcount(candidates_mask);
        if (n_candidates < min_candidates) {
            min_candidates = n_candidates;
            best_cell_idx = cell_idx;
            if (n_candidates == 2) break;
        }

        // Marcar casilla como procesada
        remaining_mask &= remaining_mask - (__uint128_t)1;
    }

    // Desmarcar la casilla como libre, obtener sus coordenadas y anotar sus unidades como sucias
    state.unresolved_mask &= ~(__uint128_t(1) << best_cell_idx);
    const cell_coords_t &coords = index_to_coords[best_cell_idx];
    auto &row_candidates = state.row_candidates[coords.row];
    state.dirty_rows |= 1ULL << coords.row;
    auto &col_candidates = state.col_candidates[coords.col];
    state.dirty_cols |= 1ULL << coords.col;
    auto &sqr_candidates = state.sqr_candidates[coords.sqr];
    state.dirty_sqrs |= 1ULL << coords.sqr;

    // Reconstruir la máscara de candidatos para la mejor casilla
    uint64_t candidates_mask = state.row_candidates[coords.row] & state.col_candidates[coords.col] & state.sqr_candidates[coords.sqr];

    // Guardar estado actual antes de probar candidatos
    const sudoku_state_t state_before_guess = state;

    // 3) PROBAR CANDIDATOS
    while (candidates_mask) {
        // Extraer el siguiente candidato
        uint64_t pos = __builtin_ctzll(candidates_mask);    // Posición del bit: [0,8]
        uint64_t clear_mask  = ~(uint64_t(1) << pos);       // Bitmask con bit de candidato a 0

        // Actualizar estado de unidades para este candidato
        row_candidates &= clear_mask;
        col_candidates &= clear_mask;
        sqr_candidates &= clear_mask;

        // Comprobar si se puede resolver con ese candidato
        if (find_solution(state, sudoku)) {
            sudoku[best_cell_idx] = char('1' + pos);
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
    // Notas que nos servirán para resolverlo rápido
    sudoku_state_t state;

    // 1) Inicializar todas las máscaras
    state.unresolved_mask = (__uint128_t(1) << 81) - 1;
    for (uint64_t i = 0; i < 9; ++i) {
        state.row_candidates[i] = 0x1FF;
        state.col_candidates[i] = 0x1FF;
        state.sqr_candidates[i] = 0x1FF;
    }
    state.dirty_rows = 0x1FF;
    state.dirty_cols = 0x1FF;
    state.dirty_sqrs = 0x1FF;

    // 2) Inicializar bitmasks con los valores y casillas ocupados
    for (uint64_t i = 0; i < 81; ++i) {
        const char c = sudoku[i];
        if (c != '.') {
            const uint64_t m = uint64_t(1) << (c - '1');

            // Marcar casilla resuelta (bit a 0)
            state.unresolved_mask &= ~(__uint128_t(1) << i);

            // Quitar el número de los candidatos disponibles en fila, columna y caja
            const cell_coords_t &coords = index_to_coords[i];
            state.row_candidates[coords.row] &= ~m;
            state.col_candidates[coords.col] &= ~m;
            state.sqr_candidates[coords.sqr] &= ~m;
        }
    }

    // 3) Resolver sudoku
    find_solution(state, sudoku);
}

#endif // SUDOKU_SOLVER_HPP