#include <array>
#include <bitset>
#include <iostream>

using std::array;
using std::bitset;
using std::cout;
using std::endl;
using std::ostream;

using i8 = signed char;

struct Color {
    i8 data;
    void Print(ostream& os = cout) const { os << (char)('A' + data); }
    void Display(ostream& os = cout) const {
        static constexpr auto kColors =
            array<const char*, 3>{"\e[44m", "\e[41m", "\e[43m"};
        os << kColors[data];
        Print(os);
        os << "\e[0m";
    }
};
[[maybe_unused]] static constexpr auto kColorA = Color{0};
[[maybe_unused]] static constexpr auto kColorB = Color{1};
static constexpr auto kColorC = Color{2};

struct Move {
    // 無印は反時計回り、p は時計回り
    enum struct MoveType : i8 { A, Ap, B, Bp };
    MoveType move_type;

    inline void Print(ostream& os = cout) const {
        static constexpr auto move_strings =
            array<const char*, 4>{"l", "-l", "r", "-r"};
        os << move_strings[(int)move_type];
    }
};

struct WreathPosition {
    enum struct Arc : i8 { AInside, AOutside, BInside, BOutside, Intersection };
    Arc arc;
    i8 index;

    inline auto operator<=>(const WreathPosition&) const = default;
};

template <int siz> struct Wreath {
    static constexpr auto ring_a_inside_size = (siz - 1) / 4;
    static constexpr auto ring_a_outside_size = siz - ring_a_inside_size - 2;
    static constexpr auto ring_b_inside_size = ring_a_inside_size + 1;
    static constexpr auto ring_b_outside_size = siz - ring_b_inside_size - 2;
    bitset<ring_a_inside_size> ring_a_inside;
    bitset<ring_a_outside_size> ring_a_outside;
    bitset<ring_b_inside_size> ring_b_inside;
    bitset<ring_b_outside_size> ring_b_outside;
    bitset<2> intersections;
    array<WreathPosition, 2> c_positions;

    inline void Reset() {
        ring_a_inside.reset();
        ring_a_outside.reset();
        ring_b_inside.set();
        ring_b_outside.set();
        intersections.reset();
        c_positions[0] = {WreathPosition::Arc::Intersection, 0};
        c_positions[1] = {WreathPosition::Arc::Intersection, 1};
    }

    inline Color Get(const WreathPosition pos) const {
        if (pos == c_positions[0] || pos == c_positions[1]) [[unlikely]]
            return kColorC;
        return Color{(i8)([this, &pos] {
            switch (pos.arc) {
            case WreathPosition::Arc::AInside:
                return ring_a_inside[pos.index];
            case WreathPosition::Arc::AOutside:
                return ring_a_outside[pos.index];
            case WreathPosition::Arc::BInside:
                return ring_b_inside[pos.index];
            case WreathPosition::Arc::BOutside:
                return ring_b_outside[pos.index];
            case WreathPosition::Arc::Intersection:
                return intersections[pos.index];
            }
        }())};
    }

    inline void Rotate(Move mov) {
        switch (mov.move_type) {
        case Move::MoveType::A: {
            const auto tmp = intersections.test(0);
            intersections.set(0, ring_a_inside.test(0));
            ring_a_inside >>= 1;
            ring_a_inside.set(ring_a_inside_size - 1, intersections.test(1));
            intersections.set(1, ring_a_outside.test(0));
            ring_a_outside >>= 1;
            ring_a_outside.set(ring_a_outside_size - 1, tmp);
            for (auto&& pos : c_positions) {
                switch (pos.arc) {
                case WreathPosition::Arc::AInside:
                    if (pos.index == 0)
                        pos.arc = WreathPosition::Arc::Intersection;
                    else
                        pos.index--;
                    break;
                case WreathPosition::Arc::AOutside:
                    if (pos.index == 0) {
                        pos.arc = WreathPosition::Arc::Intersection;
                        pos.index = 1;
                    } else
                        pos.index--;
                    break;
                case WreathPosition::Arc::Intersection:
                    if (pos.index == 0) {
                        pos.arc = WreathPosition::Arc::AOutside;
                        pos.index = ring_a_outside_size - 1;
                    } else {
                        pos.arc = WreathPosition::Arc::AInside;
                        pos.index = ring_a_inside_size - 1;
                    }
                    break;
                default:
                    break;
                }
            }
        } break;
        case Move::MoveType::Ap: {
            const auto tmp = intersections.test(1);
            intersections.set(1, ring_a_inside.test(ring_a_inside_size - 1));
            ring_a_inside <<= 1;
            ring_a_inside.set(0, intersections.test(0));
            intersections.set(0, ring_a_outside.test(ring_a_outside_size - 1));
            ring_a_outside <<= 1;
            ring_a_outside.set(0, tmp);
            for (auto&& pos : c_positions) {
                switch (pos.arc) {
                case WreathPosition::Arc::AInside:
                    if (pos.index == ring_a_inside_size - 1) {
                        pos.arc = WreathPosition::Arc::Intersection;
                        pos.index = 1;
                    } else
                        pos.index++;
                    break;
                case WreathPosition::Arc::AOutside:
                    if (pos.index == ring_a_outside_size - 1) {
                        pos.arc = WreathPosition::Arc::Intersection;
                        pos.index = 0;
                    } else
                        pos.index++;
                    break;
                case WreathPosition::Arc::Intersection:
                    if (pos.index == 0)
                        pos.arc = WreathPosition::Arc::AInside;
                    else {
                        pos.arc = WreathPosition::Arc::AOutside;
                        pos.index = 0;
                    }
                    break;
                default:
                    break;
                }
            }
        } break;
        case Move::MoveType::B: {
            const auto tmp = intersections.test(0);
            intersections.set(0, ring_b_outside.test(0));
            ring_b_outside >>= 1;
            ring_b_outside.set(ring_b_outside_size - 1, intersections.test(1));
            intersections.set(1, ring_b_inside.test(0));
            ring_b_inside >>= 1;
            ring_b_inside.set(ring_b_inside_size - 1, tmp);
            for (auto&& pos : c_positions) {
                switch (pos.arc) {
                case WreathPosition::Arc::BInside:
                    if (pos.index == 0) {
                        pos.arc = WreathPosition::Arc::Intersection;
                        pos.index = 1;
                    } else
                        pos.index--;
                    break;
                case WreathPosition::Arc::BOutside:
                    if (pos.index == 0)
                        pos.arc = WreathPosition::Arc::Intersection;
                    else
                        pos.index--;
                    break;
                case WreathPosition::Arc::Intersection:
                    if (pos.index == 0) {
                        pos.arc = WreathPosition::Arc::BInside;
                        pos.index = ring_b_inside_size - 1;
                    } else {
                        pos.arc = WreathPosition::Arc::BOutside;
                        pos.index = ring_b_outside_size - 1;
                    }
                    break;
                default:
                    break;
                }
            }
        } break;
        case Move::MoveType::Bp: {
            const auto tmp = intersections.test(1);
            intersections.set(1, ring_b_outside.test(ring_b_outside_size - 1));
            ring_b_outside <<= 1;
            ring_b_outside.set(0, intersections.test(0));
            intersections.set(0, ring_b_inside.test(ring_b_inside_size - 1));
            ring_b_inside <<= 1;
            ring_b_inside.set(0, tmp);
            for (auto&& pos : c_positions) {
                switch (pos.arc) {
                case WreathPosition::Arc::BInside:
                    if (pos.index == ring_b_inside_size - 1) {
                        pos.arc = WreathPosition::Arc::Intersection;
                        pos.index = 0;
                    } else
                        pos.index++;
                    break;
                case WreathPosition::Arc::BOutside:
                    if (pos.index == ring_b_outside_size - 1) {
                        pos.arc = WreathPosition::Arc::Intersection;
                        pos.index = 1;
                    } else
                        pos.index++;
                    break;
                case WreathPosition::Arc::Intersection:
                    if (pos.index == 0)
                        pos.arc = WreathPosition::Arc::BOutside;
                    else {
                        pos.arc = WreathPosition::Arc::BInside;
                        pos.index = 0;
                    }
                    break;
                default:
                    break;
                }
            }
        } break;
        }
    }

    inline void Display(ostream& os = cout) const {
        for (auto i = 0; i < ring_a_outside_size; i++)
            Get({WreathPosition::Arc::AOutside, (i8)i}).Display(os);
        os << '\n';
        for (auto i = 0; i < ring_a_inside_size; i++)
            os << ' ';
        for (auto i = ring_a_inside_size - 1; i >= 0; i--)
            Get({WreathPosition::Arc::AInside, (i8)i}).Display(os);
        os << '\n';
        Get({WreathPosition::Arc::Intersection, 1}).Display(os);
        for (auto i = 0; i < ring_a_outside_size - 2; i++)
            os << ' ';
        Get({WreathPosition::Arc::Intersection, 0}).Display(os);
        os << '\n';
        for (auto i = 0; i < ring_a_inside_size; i++)
            os << ' ';
        for (auto i = 0; i < ring_b_inside_size; i++)
            Get({WreathPosition::Arc::BInside, (i8)i}).Display(os);
        os << '\n';
        for (auto i = ring_b_outside_size - 1; i >= 0; i--)
            Get({WreathPosition::Arc::BOutside, (i8)i}).Display(os);
        os << endl;
    }
};

[[maybe_unused]] void TestWreath() {
    auto wreath = Wreath<12>();
    wreath.Reset();
    wreath.Display();
    cout << endl;
    wreath.Rotate({Move::MoveType::A});
    wreath.Display();
    cout << endl;
    wreath.Rotate({Move::MoveType::B});
    wreath.Display();
    cout << endl;
    wreath.Rotate({Move::MoveType::Ap});
    wreath.Display();
    cout << endl;
    wreath.Rotate({Move::MoveType::Bp});
    wreath.Display();
}

// clang++ -std=c++20 -Wall -Wextra -O3 wreath.cpp
int main() { TestWreath(); }