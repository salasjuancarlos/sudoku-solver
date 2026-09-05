#include <cstdint>

constexpr __uint128_t row_to_cells_mask_calc(uint64_t x) {
    return (__uint128_t)0x1FF << ((x << 3) + x);
}

constexpr __uint128_t col_to_cells_mask_calc(uint64_t x) {
    __uint128_t mask = 0;
    for (uint64_t i = x; i <= 80; i += 9)
        mask |= (__uint128_t)1 << i;
    return mask;
}

constexpr __uint128_t sqr_to_cells_mask_calc(uint64_t x) {
    __uint128_t mask = 0;
    const uint64_t x_mod_3 = x % 3;
    const uint64_t col_bits = 0b111 << ((x_mod_3 << 1) + x_mod_3);
    const uint64_t base = (x / 3) * 27;
    for (uint64_t r = 0; r < 3; ++r)
        mask |= (__uint128_t)col_bits << (base + (r << 3) + r);
    return mask;
}

constexpr __uint128_t row_to_cells_mask[] = {
    row_to_cells_mask_calc(0), row_to_cells_mask_calc(1), row_to_cells_mask_calc(2),
    row_to_cells_mask_calc(3), row_to_cells_mask_calc(4), row_to_cells_mask_calc(5),
    row_to_cells_mask_calc(6), row_to_cells_mask_calc(7), row_to_cells_mask_calc(8)
};

constexpr __uint128_t col_to_cells_mask[] = {
    col_to_cells_mask_calc(0), col_to_cells_mask_calc(1), col_to_cells_mask_calc(2),
    col_to_cells_mask_calc(3), col_to_cells_mask_calc(4), col_to_cells_mask_calc(5),
    col_to_cells_mask_calc(6), col_to_cells_mask_calc(7), col_to_cells_mask_calc(8)
};

constexpr __uint128_t sqr_to_cells_mask[] = {
    sqr_to_cells_mask_calc(0), sqr_to_cells_mask_calc(1), sqr_to_cells_mask_calc(2),
    sqr_to_cells_mask_calc(3), sqr_to_cells_mask_calc(4), sqr_to_cells_mask_calc(5),
    sqr_to_cells_mask_calc(6), sqr_to_cells_mask_calc(7), sqr_to_cells_mask_calc(8)
};