#include "sudoku_solver.hpp"

constexpr uint8_t first_index_per_row[9] = {0, 9, 18, 27, 36, 45, 54, 63, 72};
constexpr uint8_t first_index_per_col[9] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
constexpr uint8_t first_index_per_sqr[9] = {0, 3, 6, 27, 30, 33, 54, 57, 60};

constexpr uint8_t delta_row[9] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
constexpr uint8_t delta_col[9] = {0, 9, 18, 27, 36, 45, 54, 63, 72};
constexpr uint8_t delta_sqr[9] = {0, 1, 2, 9, 10, 11, 18, 19, 20};

inline void check_row(sketch_notes_t& s, uint32_t row) {
    uint_fast32_t idx = first_index_per_row[row];
    bool changed = true;
    while (changed) {
        changed = false;
        for (uint_fast32_t i = 0; i < 9; ++i) {
            idx += delta_row[i];
            const auto& rcs = indices_table[idx];
            auto& rb = s.rows_bitmask[rcs.row];
            auto& cb = s.cols_bitmask[rcs.col];
            auto& sb = s.squares_bitmask[rcs.sqr];
            const uint64_t pm = ~(rb | cb | sb) & 0x1FFULL;
            if (pm && !(pm & (pm - 1))) {
                s.values_bitmask &= ~(__uint128_t(1) << idx);
                rb |= pm;
                cb |= pm;
                sb |= pm;
                s.sudoku[idx] = char('1' + __builtin_ctz(pm));
                changed = true;
                cols_to_check |= 1 << rcs.col;
                sqrs_to_check |= 1 << rcs.sqr;
                break;
            }
        }
    }
}

inline void check_col(sketch_notes_t& s, uint32_t col) {
    uint_fast32_t idx = first_index_per_col[col];
    bool changed = true;
    while (changed) {
        changed = false;
        for (uint_fast32_t i = 0; i < 9; ++i) {
            idx += delta_col[i];
            const auto& rcs = indices_table[idx];
            auto& rb = s.rows_bitmask[rcs.row];
            auto& cb = s.cols_bitmask[rcs.col];
            auto& sb = s.squares_bitmask[rcs.sqr];
            const uint64_t pm = ~(rb | cb | sb) & 0x1FFULL;
            if (pm && !(pm & (pm - 1))) {
                s.values_bitmask &= ~(__uint128_t(1) << idx);
                rb |= pm;
                cb |= pm;
                sb |= pm;
                s.sudoku[idx] = char('1' + __builtin_ctz(pm));
                changed = true;
                break;
            }
        }
    }
}

inline void check_sqr(sketch_notes_t& s, uint32_t sqr) {
    uint_fast32_t idx = first_index_per_sqr[sqr];
    bool changed = true;
    while (changed) {
        changed = false;
        for (uint_fast32_t i = 0; i < 9; ++i) {
            idx += delta_sqr[i];
            const auto& rcs = indices_table[idx];
            auto& rb = s.rows_bitmask[rcs.row];
            auto& cb = s.cols_bitmask[rcs.col];
            auto& sb = s.squares_bitmask[rcs.sqr];
            const uint64_t pm = ~(rb | cb | sb) & 0x1FFULL;
            if (pm && !(pm & (pm - 1))) {
                s.values_bitmask &= ~(__uint128_t(1) << idx);
                rb |= pm;
                cb |= pm;
                sb |= pm;
                s.sudoku[idx] = char('1' + __builtin_ctz(pm));
                changed = true;

                break;
            }
        }
    }
}

inline void check_col(const uint8_t col, sketch_notes_t& s) {

    uint64_t saw_position[9];
    uint64_t once  = 0;
    uint64_t multi = 0;

    for (uint64_t i = 0; i < 9; ++i) {

        uint64_t idx    = col + i * 9;

        if (!(s.values_bitmask & ((__uint128_t)1 << idx))) continue;

        const auto& rcs = indices_table[idx];
        auto& rb        = s.rows_bitmask[rcs.row];
        auto& cb        = s.cols_bitmask[rcs.col];
        auto& sb        = s.squares_bitmask[rcs.sqr];

        const uint64_t pm = ~(rb | cb | sb) & 0x1FFULL;

        // Obtener las primeras apariciones
        uint32_t first_occurrence = pm & ~(once | multi);

        // Anotar las primeras apariciones y guardar la posición
        while (first_occurrence) {
            uint64_t value      = __builtin_ctzll(first_occurrence);
            saw_position[value] = idx;
            first_occurrence    &= first_occurrence - 1;
        }

        // Actualizar contadores de candidatos
        multi |= (once & pm);
        once  &= ~multi;
        once  |= pm & ~(multi | once);

        // Naked single
        if (pm && !(pm & (pm - 1))) {
            s.values_bitmask    &= ~(__uint128_t(1) << idx);
            rb                  |= pm;
            cb                  |= pm;
            sb                  |= pm;
            s.sudoku[idx]       = char('1' + __builtin_ctz(pm));
        }
    }

    while (once) {
        // Hidden single
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