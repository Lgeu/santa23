#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using std::array;
using std::bitset;
using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::fill;
using std::format;
using std::make_shared;
using std::min;
using std::ofstream;
using std::ostream;
using std::ostringstream;
using std::shared_ptr;
using std::string;
using std::swap;
using std::vector;

using i8 = signed char;
using u64 = unsigned long long;

struct Problem {
    int id;
    int size;
    string initial_state;
    int n_wildcards;
};

// clang-format off
static auto kProblemData = vector<Problem>{
    {284,6,"A;A;A;A;B;C;B;C;B;B",0},
    {285,6,"B;A;B;A;B;A;C;A;B;C",2},
    {286,6,"A;B;C;A;A;C;B;B;B;A",2},
    {287,6,"B;C;B;C;A;A;B;A;B;A",2},
    {288,6,"A;A;C;B;C;A;B;B;A;B",0},
    {289,6,"A;B;A;C;A;B;B;B;A;C",0},
    {290,6,"A;B;B;A;B;A;B;A;C;C",0},
    {291,6,"A;B;C;B;A;C;A;A;B;B",2},
    {292,6,"C;B;B;A;B;B;A;A;A;C",0},
    {293,6,"A;B;B;C;A;B;B;C;A;A",0},
    {294,6,"A;C;A;B;B;A;B;C;B;A",0},
    {295,6,"A;A;C;A;B;B;B;B;C;A",0},
    {296,6,"C;A;B;C;B;A;B;B;A;A",0},
    {297,6,"B;A;C;A;B;A;A;C;B;B",0},
    {298,6,"A;C;B;A;A;B;B;B;C;A",2},
    {299,6,"A;B;C;A;C;A;B;A;B;B",0},
    {300,6,"A;B;B;B;A;C;A;A;B;C",0},
    {301,6,"B;B;B;B;A;C;A;C;A;A",0},
    {302,6,"C;A;B;A;A;B;B;C;B;A",0},
    {303,6,"B;A;A;B;A;C;B;A;C;B",0},
    {304,7,"A;B;B;B;C;A;A;B;C;B;A;A",0},
    {305,7,"B;C;B;B;A;C;A;A;B;B;A;A",0},
    {306,7,"B;A;A;B;A;A;A;C;B;C;B;B",0},
    {307,7,"B;A;A;B;A;C;B;A;C;B;B;A",0},
    {308,7,"A;B;C;B;B;C;B;A;B;A;A;A",0},
    {309,7,"B;A;A;C;B;C;B;B;A;A;B;A",2},
    {310,7,"B;A;C;B;A;A;A;C;B;B;B;A",2},
    {311,7,"B;A;B;A;A;C;B;A;C;B;A;B",0},
    {312,7,"B;B;A;B;C;A;B;C;A;B;A;A",0},
    {313,7,"B;B;A;C;B;A;C;A;A;A;B;B",0},
    {314,7,"A;C;A;A;B;B;B;A;C;B;A;B",0},
    {315,7,"B;A;A;A;B;C;B;A;A;C;B;B",0},
    {316,7,"C;B;A;B;B;A;A;C;A;B;B;A",0},
    {317,7,"A;B;A;C;A;C;A;B;A;B;B;B",0},
    {318,7,"C;A;A;C;B;A;A;B;B;B;B;A",0},
    {319,12,"A;B;A;B;B;B;B;A;A;B;B;A;B;B;A;B;C;C;A;A;A;A",2},
    {320,12,"A;A;C;B;B;C;A;B;A;B;A;A;B;B;A;B;A;B;B;B;A;A",0},
    {321,12,"B;B;C;B;A;A;A;B;B;B;A;B;A;A;C;A;B;B;B;A;A;A",2},
    {322,12,"A;C;A;B;B;A;A;B;B;B;C;A;B;A;B;A;B;B;A;A;B;A",0},
    {323,12,"B;B;A;B;A;A;B;B;C;B;A;A;B;B;A;B;A;A;A;A;B;C",0},
    {324,12,"A;A;B;A;A;B;B;A;C;B;A;A;B;B;C;A;B;A;B;B;A;B",0},
    {325,12,"B;A;B;A;A;A;B;A;B;B;B;A;A;B;A;B;C;C;A;B;A;B",0},
    {326,12,"B;A;B;A;A;B;B;C;A;A;B;A;A;A;B;B;B;A;C;B;A;B",0},
    {327,12,"B;A;C;A;A;A;A;C;A;A;B;B;B;B;A;B;B;A;B;A;B;B",0},
    {328,12,"B;A;C;A;B;A;A;B;A;B;B;A;B;B;B;A;B;A;A;A;C;B",2},
    {329,21,"B;B;A;B;A;A;A;B;A;B;A;B;B;A;A;B;A;B;A;A;B;A;C;B;B;A;B;B;A;C;A;B;A;B;B;B;A;B;A;A",0},
    {330,21,"A;A;B;B;A;A;B;B;B;A;B;B;B;B;B;B;B;B;A;A;A;A;C;B;A;B;A;A;A;B;A;A;B;A;B;A;A;A;C;B",0},
    {331,21,"B;B;A;B;B;A;B;B;C;A;B;B;B;A;B;A;A;A;A;B;A;B;B;A;A;B;B;A;A;B;B;C;B;A;A;A;A;A;A;B",0},
    {332,21,"A;B;A;B;B;A;B;B;A;B;A;A;A;B;B;A;B;B;A;B;B;B;B;A;A;B;B;A;A;C;B;B;C;A;B;A;A;A;A;A",0},
    {333,21,"B;A;A;B;B;B;B;A;B;A;B;B;B;B;A;B;B;A;A;A;A;A;B;B;A;A;B;B;B;A;A;A;B;A;C;C;A;A;B;A",0},
    {334,33,"B;B;A;A;B;B;B;B;A;A;B;A;B;A;A;B;A;A;B;B;A;A;C;A;B;A;B;B;B;B;A;A;B;B;B;B;A;A;B;A;A;B;B;A;A;A;B;A;A;C;B;B;B;B;A;B;A;A;A;A;B;A;A;B",0},
    {335,33,"A;B;A;A;A;A;A;A;A;B;B;A;B;A;B;B;A;B;B;B;B;B;B;A;B;A;A;B;C;B;A;B;B;A;B;C;A;A;A;A;A;B;B;B;A;A;B;B;A;B;A;B;B;B;B;A;A;B;A;A;A;B;B;A",0},
    {336,33,"A;B;B;B;A;A;B;B;A;A;A;A;A;B;C;A;A;B;A;B;A;C;A;B;A;B;A;B;B;B;A;A;B;A;B;B;B;A;B;A;A;B;B;B;B;B;A;B;B;B;A;B;A;B;A;A;A;B;A;A;B;A;A;B",8},
    {337,100,"A;A;B;B;B;A;B;B;A;B;A;B;B;B;B;B;A;A;A;A;B;B;B;A;B;A;A;A;B;A;B;B;A;B;A;B;B;B;A;A;A;B;A;B;B;B;B;A;B;A;B;A;B;A;B;A;B;A;B;A;B;A;B;A;B;A;A;B;B;A;B;A;B;B;B;B;B;A;A;A;A;A;B;B;A;A;A;A;B;A;A;B;B;A;B;A;A;A;B;B;A;C;B;B;A;B;A;B;A;B;A;A;B;A;A;A;A;B;B;A;A;A;A;A;A;B;A;A;B;B;B;A;B;B;B;A;A;A;B;B;B;A;A;B;A;A;B;B;B;A;A;B;A;A;A;A;B;A;A;B;B;B;B;B;A;B;B;B;A;A;B;C;A;A;A;A;B;B;A;B;A;B;A;B;B;A;A;B;A;B;A;B;A;B;B;B;A;B",8},
};
// clang-format on

struct RandomNumberGenerator {
  private:
    u64 seed;

  public:
    inline RandomNumberGenerator(const u64 seed) : seed(seed) {}
    inline auto Next() {
        seed ^= seed << 9;
        seed ^= seed >> 7;
        return seed;
    }
};

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

    inline Move Inv() const { return Move{(MoveType)((int)move_type ^ 1)}; }

    inline void Print(ostream& os = cout) const {
        static constexpr auto move_strings =
            array<const char*, 4>{"l", "-l", "r", "-r"};
        os << move_strings[(int)move_type];
    }

    auto operator<=>(const Move&) const = default;
};
[[maybe_unused]] static constexpr auto kMoveA = Move{Move::MoveType::A};
[[maybe_unused]] static constexpr auto kMoveAp = Move{Move::MoveType::Ap};
[[maybe_unused]] static constexpr auto kMoveB = Move{Move::MoveType::B};
[[maybe_unused]] static constexpr auto kMoveBp = Move{Move::MoveType::Bp};

struct WreathPosition {
    enum struct Arc : i8 { AInside, AOutside, BInside, BOutside, Intersection };
    Arc arc;
    i8 index;

    inline auto operator<=>(const WreathPosition&) const = default;

    inline bool IsOnRingA() const {
        return arc == Arc::AInside || arc == Arc::AOutside ||
               arc == Arc::Intersection;
    }

    inline bool IsOnRingB() const {
        return arc == Arc::BInside || arc == Arc::BOutside ||
               arc == Arc::Intersection;
    }
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

    inline auto ComputeScore() const {
        using Arc = WreathPosition::Arc;
        // 外側で揃っていない玉の数
        const auto a_outside_score = (int)ring_a_outside.count();
        const auto b_outside_score =
            ring_b_outside_size - (int)ring_b_outside.count() -
            (c_positions[0].arc == Arc::BOutside ? 1 : 0) -
            (c_positions[1].arc == Arc::BOutside ? 1 : 0);

        // 内側で揃っていない玉の数
        const auto a_inside_score = (int)ring_a_inside.count();
        const auto b_inside_score =
            ring_b_inside_size - (int)ring_b_inside.count() -
            (c_positions[0].arc == Arc::BInside ? 1 : 0) -
            (c_positions[1].arc == Arc::BInside ? 1 : 0);

        // C の位置
        auto c_same_ring_score = 0;
        auto c_same_ring_penalty = 0;
        auto c_different_ring_penalty = 0;
        // 両方ともリング A 上の場合
        if (c_positions[0].IsOnRingA() && c_positions[1].IsOnRingA()) {
            const auto p0 = c_positions[0];
            const auto p1 = c_positions[1];
            auto idx0 = p0.arc == Arc::AInside ? p0.index + 1
                        : p0.arc == Arc::AOutside
                            ? p0.index + ring_a_inside_size + 2
                            : p0.index * (ring_a_inside_size + 1);
            auto idx1 = p1.arc == Arc::AInside ? p1.index + 1
                        : p1.arc == Arc::AOutside
                            ? p1.index + ring_a_inside_size + 2
                            : p1.index * (ring_a_inside_size + 1);
            if (idx0 > idx1)
                swap(idx0, idx1);
            static_assert((ring_a_inside_size + 1) * 2 != siz);
            // 間隔が適切 (idx0 が 0 側)
            if (idx1 - idx0 == ring_a_inside_size + 1)
                c_same_ring_score = min(idx0, siz - idx0);
            // 間隔が適切 (idx1 が 0 側)
            else if (siz - (idx1 - idx0) == ring_a_inside_size + 1)
                c_same_ring_score = min(idx1, siz - idx1);
            // 間隔が適切でない
            else
                c_same_ring_penalty = 1;
        }
        // 両方ともリング B 上の場合
        else if (c_positions[0].IsOnRingB() && c_positions[1].IsOnRingB()) {
            const auto p0 = c_positions[0];
            const auto p1 = c_positions[1];
            auto idx0 = p0.arc == Arc::BInside ? p0.index + 1
                        : p0.arc == Arc::BOutside
                            ? p0.index + ring_b_inside_size + 2
                            : p0.index * (ring_b_inside_size + 1);
            auto idx1 = p1.arc == Arc::BInside ? p1.index + 1
                        : p1.arc == Arc::BOutside
                            ? p1.index + ring_b_inside_size + 2
                            : p1.index * (ring_b_inside_size + 1);
            if (idx0 > idx1)
                swap(idx0, idx1);
            if constexpr ((ring_b_inside_size + 1) * 2 == siz) {
                // 間隔が適切
                if (idx1 - idx0 == ring_b_inside_size + 1) {
                    assert(siz - (idx1 - idx0) == ring_b_inside_size + 1);
                    c_same_ring_score =
                        min({idx0, siz - idx0, idx1, siz - idx1});
                } else // 間隔が適切でない
                    c_same_ring_penalty = 1;
            } else {
                // 間隔が適切 (idx0 が 0 側)
                if (idx1 - idx0 == ring_b_inside_size + 1)
                    c_same_ring_score = min(idx0, siz - idx0);
                // 間隔が適切 (idx1 が 0 側)
                else if (siz - (idx1 - idx0) == ring_b_inside_size + 1)
                    c_same_ring_score = min(idx1, siz - idx1);
                // 間隔が適切でない
                else
                    c_same_ring_penalty = 1;
            }
        }
        // 一方がリング A 上、一方がリング B 上の場合
        else {
            c_different_ring_penalty = 1;
        }
        return (a_outside_score + b_outside_score) * 10 +
               (a_inside_score + b_inside_score) * 10 + c_same_ring_score +
               c_same_ring_penalty * 200 + c_different_ring_penalty * 100;
    }

    inline void Read(const string s) {
        Reset();
        auto i = 0;
        auto c_counts = 0;
        for (const auto& c : s) {
            if (c != 'A' && c != 'B' && c != 'C')
                continue;
            if (i == 0) {
                intersections.set(0, c == 'B');
                if (c == 'C')
                    c_positions[c_counts++] = {WreathPosition::Arc::AOutside,
                                               0};
            } else if (i < ring_a_inside_size + 1) {
                ring_a_inside.set(i - 1, c == 'B');
                if (c == 'C')
                    c_positions[c_counts++] = {WreathPosition::Arc::AInside,
                                               (i8)(i - 1)};
            } else if (i == ring_a_inside_size + 1) {
                intersections.set(1, c == 'B');
                if (c == 'C')
                    c_positions[c_counts++] = {WreathPosition::Arc::AOutside,
                                               1};
            } else if (i < ring_a_inside_size + ring_a_outside_size + 2) {
                ring_a_outside.set(i - ring_a_inside_size - 2, c == 'B');
                if (c == 'C')
                    c_positions[c_counts++] = {
                        WreathPosition::Arc::AOutside,
                        (i8)(i - ring_a_inside_size - 2)};
            } else if (i < ring_a_inside_size + ring_a_outside_size + 2 +
                               ring_b_outside_size) {
                ring_b_outside.set(
                    i - ring_a_inside_size - ring_a_outside_size - 2, c == 'B');
                if (c == 'C')
                    c_positions[c_counts++] = {
                        WreathPosition::Arc::BOutside,
                        (i8)(i - ring_a_inside_size - ring_a_outside_size - 2)};
            } else if (i < ring_a_inside_size + ring_a_outside_size + 2 +
                               ring_b_outside_size + ring_b_inside_size) {
                ring_b_inside.set(i - ring_a_inside_size - ring_a_outside_size -
                                      ring_b_outside_size - 2,
                                  c == 'B');
                if (c == 'C')
                    c_positions[c_counts++] = {WreathPosition::Arc::BInside,
                                               (i8)(i - ring_a_inside_size -
                                                    ring_a_outside_size -
                                                    ring_b_outside_size - 2)};
            } else {
                assert(false);
            }
            i++;
        }
        assert(c_counts == 2);
        assert(i == siz * 2 - 2);
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

    inline auto operator<=>(const Wreath&) const = default;
};

using Action = Move;

template <int siz, int scoring_depth> struct State {
    Wreath<siz> wreath;
    i8 last_move;                         // 初期状態では -1
    array<int, scoring_depth + 1> scores; // 各深さでのスコア
    int n_moves;

    inline State(const Wreath<siz>& wreath, const i8 last_move)
        : wreath(wreath), last_move(last_move), n_moves() {
        ComputeScores();
    }

    // inplace に変更する
    inline void Apply(const Action& action) {
        wreath.Rotate(action);
        last_move = (i8)action.move_type;
        const auto tmp_wreath = wreath;
        ComputeScores();
        assert(tmp_wreath == wreath);
        n_moves++;
    }

    // DFS でスコアを計算する
    inline void ComputeScores() {
        fill(scores.begin(), scores.end(), (int)1e9);
        auto info = DFSInfo{};
        Dfs(info);
    }

  private:
    struct DFSInfo {
        int depth;
        array<Move, scoring_depth> move_history;
    };
    inline void Dfs(DFSInfo& info) {
        scores[info.depth] = min(scores[info.depth], wreath.ComputeScore());
        if (info.depth == scoring_depth)
            return;
        for (auto mov : {kMoveA, kMoveAp, kMoveB, kMoveBp}) {
            const auto inv_mov = mov.Inv();
            if (info.depth == 0) {
                if (last_move == (i8)inv_mov.move_type)
                    continue;
            } else {
                if (info.move_history[info.depth - 1] == inv_mov)
                    continue;
            }
            info.move_history[info.depth++] = mov;
            wreath.Rotate(mov);
            Dfs(info);
            wreath.Rotate(inv_mov);
            info.depth--;
        }
    }
};

template <int siz, int scoring_depth> struct Node {
    using State = ::State<siz, scoring_depth>;
    State state;
    shared_ptr<Node> parent;
    bool children_expanded;
    inline Node(const State& state, shared_ptr<Node> parent)
        : state(state), parent(parent), children_expanded() {}
};

template <int siz, int scoring_depth> struct BeamSearchSolver {
    using Wreath = ::Wreath<siz>;
    using State = ::State<siz, scoring_depth>;
    using Node = ::Node<siz, scoring_depth>;

    int beam_width_for_each_depth;
    vector<vector<shared_ptr<Node>>> nodes;

    inline BeamSearchSolver(int beam_width_for_each_depth)
        : beam_width_for_each_depth(beam_width_for_each_depth) {}

    inline shared_ptr<Node> Solve(const Wreath& start_wreath, int n_wildcards) {
        auto rng = RandomNumberGenerator(42);

        const auto start_state = State(start_wreath, -1);
        const auto start_node = make_shared<Node>(start_state, nullptr);
        nodes.clear();
        nodes.resize(1);
        nodes[0].push_back(start_node);

        for (auto current_cost = 0; current_cost < 1000; current_cost++) {
            auto current_minimum_score = (int)1e9;
            for (auto&& node : nodes[current_cost]) {
                if (node == nullptr || node->children_expanded)
                    continue;
                node->children_expanded = true;
                current_minimum_score =
                    min(current_minimum_score, node->state.scores[0]);
                if (node->state.scores[0] == 0) {
                    // TODO: wildcard
                    cerr << "Solved!" << endl;
                    return node;
                }
                for (const auto& action : {kMoveA, kMoveAp, kMoveB, kMoveBp}) {
                    if ((i8)action.Inv().move_type == node->state.last_move)
                        continue;
                    auto new_state = node->state;
                    new_state.Apply(action);
                    if (new_state.n_moves >= (int)nodes.size()) {
                        nodes.resize(new_state.n_moves + 1);
                        // 全ての depth で同じビーム幅なの効率悪そう
                        nodes[new_state.n_moves].resize(
                            beam_width_for_each_depth * (scoring_depth + 1));
                    }
                    for (auto depth = 0; depth <= scoring_depth; depth++) {
                        const auto idx =
                            depth * beam_width_for_each_depth +
                            (int)(rng.Next() % beam_width_for_each_depth);
                        if (nodes[new_state.n_moves][idx] == nullptr ||
                            new_state.scores[depth] <
                                nodes[new_state.n_moves][idx]
                                    ->state.scores[depth])
                            nodes[new_state.n_moves][idx].reset(
                                new Node(new_state, node));
                    }
                }
            }
            cout << format("current_cost={} current_minimum_score={}",
                           current_cost, current_minimum_score)
                 << endl;
            if (nodes[current_cost][0] != nullptr)
                nodes[current_cost][0]->state.wreath.Display();
            nodes[current_cost].clear();
        }
        cerr << "Failed." << endl;
        return nullptr;
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

[[maybe_unused]] void InteractiveWreath() {
    auto wreath = Wreath<33>();
    wreath.Reset();
    auto rng = RandomNumberGenerator(42);
    // ランダムに初期化
    wreath.Reset();
    for (auto i = 0; i < 10000; i++)
        wreath.Rotate({(Move::MoveType)(rng.Next() % 4)});
    vector<Move> moves;
    while (true) {
        wreath.Display();
        cout << "total moves = " << moves.size() << endl;

        if (wreath.ComputeScore() == 0) {
            cout << "Solved!" << endl;
            break;
        }

        int n;
        while (true) {
            cin >> n;
            n--;
            if (n >= 0 && n <= 3)
                break;
            if (n == -1 && moves.size() >= 1) {
                n = (int)moves.back().Inv().move_type;
                break;
            }
        }
        Move mov = {(Move::MoveType)n};
        wreath.Rotate(mov);
        if (moves.size() >= 1 && moves.back().move_type == mov.Inv().move_type)
            moves.pop_back();
        else
            moves.push_back(mov);
    }
    cout << "moves = " << endl;
    for (auto mov : moves) {
        mov.Print();
        cout << " ";
    }
    cout << endl;
}

[[maybe_unused]] void TestBeamSearch() {
    static constexpr auto kWreathSize = 33;
    static constexpr auto kBeamWidthForEachDepth = 1;
    static constexpr auto kScoringDepth = 12;
    auto solver =
        BeamSearchSolver<kWreathSize, kScoringDepth>(kBeamWidthForEachDepth);
    auto wreath = Wreath<kWreathSize>();
    auto rng = RandomNumberGenerator(42);
    // ランダムに初期化
    wreath.Reset();
    for (auto i = 0; i < 1000; i++)
        wreath.Rotate({(Move::MoveType)(rng.Next() % 4)});
    wreath.Display();
    cout << endl;

    const auto node = solver.Solve(wreath, 0);
    if (node != nullptr) {
        cout << node->state.n_moves << endl;
        auto moves = vector<Move>();
        for (auto p = node; p->parent != nullptr; p = p->parent)
            moves.push_back(Move{(Move::MoveType)p->state.last_move});
        reverse(moves.begin(), moves.end());
        for (auto i = 0; i < (int)moves.size(); i++) {
            const auto mov = moves[i];
            mov.Print();
            if (i != (int)moves.size() - 1)
                cout << ".";
            wreath.Rotate(mov);
        }
        cout << endl;
        wreath.Display();
    }
}

template <int siz> static void Solve(const Problem problem) {
    // TODO: 何故か 290 が解けなかったりするので原因を探す

    static constexpr auto kBeamWidthForEachDepth = 4;
    static constexpr auto kScoringDepth = 12;

    assert(problem.size == siz);
    auto wreath = Wreath<siz>();
    wreath.Read(problem.initial_state);
    wreath.Display();
    cout << endl;

    auto solver = BeamSearchSolver<siz, kScoringDepth>(kBeamWidthForEachDepth);
    const auto node = solver.Solve(wreath, problem.n_wildcards);
    if (node != nullptr) {
        cout << node->state.n_moves << endl;
        auto moves = vector<Move>();
        for (auto p = node; p->parent != nullptr; p = p->parent)
            moves.push_back(Move{(Move::MoveType)p->state.last_move});
        reverse(moves.begin(), moves.end());
        auto oss = ostringstream();
        for (auto i = 0; i < (int)moves.size(); i++) {
            const auto mov = moves[i];
            mov.Print(oss);
            if (i != (int)moves.size() - 1)
                oss << ".";
            wreath.Rotate(mov);
        }
        const auto solution_string = oss.str();
        cout << solution_string << endl;
        wreath.Display();

        // ファイルに結果を書き出す
        auto ofs = ofstream(format("result/wreath/{}.txt", problem.id));
        ofs << format("# n_moves={}\n", node->state.n_moves);
        ofs << format("# kBeamWidthForEachDepth={}\n", kBeamWidthForEachDepth);
        ofs << format("# kScoringDepth={}\n", kScoringDepth);
        ofs << solution_string << endl;
    }
}

[[maybe_unused]] static void Solve(const int problem_id) {
    const auto it = find_if(kProblemData.begin(), kProblemData.end(),
                            [problem_id](const Problem& problem) {
                                return problem.id == problem_id;
                            });
    if (it == kProblemData.end()) {
        cerr << "Problem not found." << endl;
        exit(1);
    }
    const auto problem = *it;
    switch (problem.size) {
    case 6:
        Solve<6>(problem);
        break;
    case 7:
        Solve<7>(problem);
        break;
    case 12:
        Solve<12>(problem);
        break;
    case 21:
        Solve<21>(problem);
        break;
    case 33:
        Solve<33>(problem);
        break;
    default:
        assert(false);
    }
}

// clang++ -std=c++20 -Wall -Wextra -O3 wreath.cpp -DTEST_WREATH
#ifdef TEST_WREATH
int main() { TestWreath(); }
#endif

// clang++ -std=c++20 -Wall -Wextra -O3 wreath.cpp -DINTERACTIVE_WREATH
#ifdef INTERACTIVE_WREATH
int main() { InteractiveWreath(); }
#endif

// clang++ -std=c++20 -Wall -Wextra -O3 wreath.cpp -DTEST_BEAM_SEARCH
#ifdef TEST_BEAM_SEARCH
int main() { TestBeamSearch(); }
#endif

// clang++ -std=c++20 -Wall -Wextra -O3 wreath.cpp -DSOLVE
#ifdef SOLVE
int main(const int argc, const char* const* const argv) {
    if (argc != 2) {
        cerr << format("Usage: {} <problem_id>", argv[0]) << endl;
        return 1;
    }
    Solve(atoi(argv[1]));
}
#endif