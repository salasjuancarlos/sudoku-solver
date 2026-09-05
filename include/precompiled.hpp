#include <cstdint>

constexpr __uint128_t check_bitmask(uint_fast8_t idx) noexcept {
    int r = idx / 9;
    int c = idx % 9;
    int s = (r / 3) * 3 + c / 3;

    __uint128_t row_bitmask = ((__uint128_t)0x1FF) << (r * 9);

    __uint128_t col_bitmask = 0;
    for (int rr = 0; rr < 9; ++rr)
        col_bitmask |= (__uint128_t)1 << (rr * 9 + c);

    __uint128_t sqr_bitmask = 0;
    int sr = (s / 3) * 3;
    int sc = (s % 3) * 3;
    for (int dr = 0; dr < 3; ++dr)
        for (int dc = 0; dc < 3; ++dc)
            sqr_bitmask |= (__uint128_t)1 << ((sr + dr) * 9 + (sc + dc));

    return row_bitmask | col_bitmask | sqr_bitmask;
}

constexpr __uint128_t check_bitmask_table[81] = {
    check_bitmask(0),  check_bitmask(1),  check_bitmask(2),  check_bitmask(3),
    check_bitmask(4),  check_bitmask(5),  check_bitmask(6),  check_bitmask(7),
    check_bitmask(8),  check_bitmask(9),  check_bitmask(10), check_bitmask(11),
    check_bitmask(12), check_bitmask(13), check_bitmask(14), check_bitmask(15),
    check_bitmask(16), check_bitmask(17), check_bitmask(18), check_bitmask(19),
    check_bitmask(20), check_bitmask(21), check_bitmask(22), check_bitmask(23),
    check_bitmask(24), check_bitmask(25), check_bitmask(26), check_bitmask(27),
    check_bitmask(28), check_bitmask(29), check_bitmask(30), check_bitmask(31),
    check_bitmask(32), check_bitmask(33), check_bitmask(34), check_bitmask(35),
    check_bitmask(36), check_bitmask(37), check_bitmask(38), check_bitmask(39),
    check_bitmask(40), check_bitmask(41), check_bitmask(42), check_bitmask(43),
    check_bitmask(44), check_bitmask(45), check_bitmask(46), check_bitmask(47),
    check_bitmask(48), check_bitmask(49), check_bitmask(50), check_bitmask(51),
    check_bitmask(52), check_bitmask(53), check_bitmask(54), check_bitmask(55),
    check_bitmask(56), check_bitmask(57), check_bitmask(58), check_bitmask(59),
    check_bitmask(60), check_bitmask(61), check_bitmask(62), check_bitmask(63),
    check_bitmask(64), check_bitmask(65), check_bitmask(66), check_bitmask(67),
    check_bitmask(68), check_bitmask(69), check_bitmask(70), check_bitmask(71),
    check_bitmask(72), check_bitmask(73), check_bitmask(74), check_bitmask(75),
    check_bitmask(76), check_bitmask(77), check_bitmask(78), check_bitmask(79),
    check_bitmask(80)
};