#include <array>
#include <cassert>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

using std::array;
using std::cout;
using std::endl;
using std::format;
using std::ofstream;
using std::ostream;
using std::swap;
using std::unordered_set;
using std::vector;

using i8 = signed char;
using u8 = unsigned char;

struct UnitMove {
    enum struct Direction : i8 { R, Rp, F }; // p は反時計回り Fp = F
    Direction direction;
    i8 depth;

    inline UnitMove() = default;

    inline UnitMove(const Direction direction, const i8 depth)
        : direction(direction), depth(depth) {
        if (direction != Direction::F)
            assert(depth == 0 || depth == 1);
    }

    UnitMove Inv() const {
        if (direction == Direction::F)
            return *this;
        return UnitMove(
            direction == Direction::R ? Direction::Rp : Direction::R, depth);
    }

    inline bool IsClockWise() const { return direction != Direction::Rp; }

    inline bool IsOpposite(const UnitMove& m) const {
        if (direction == Direction::R && m.direction == Direction::Rp &&
            depth == m.depth)
            return true;
        if (direction == Direction::Rp && m.direction == Direction::R &&
            depth == m.depth)
            return true;
        if (direction == Direction::F && m.direction == Direction::F &&
            depth == m.depth)
            return true;
        return false;
    }

    inline auto operator<=>(const UnitMove&) const = default;

    inline void Print(ostream& os = cout) const {
        static constexpr auto direction_strings =
            array<const char*, 3>{"r", "-r", "f"};
        os << direction_strings[int(direction)] << int(depth);
    }
};

struct Move {
    UnitMove unit_move;
    i8 unit_id;

    inline Move(const UnitMove unit_move, const i8 unit_id)
        : unit_move(unit_move), unit_id(unit_id) {}

    template <int height> inline void Print(ostream& os = cout) const {
        const auto depth =
            unit_move.depth == 0 ? unit_id : height - 1 - unit_id;
        static constexpr auto direction_strings =
            array<const char*, 3>{"r", "-r", "f"};
        os << direction_strings[int(unit_move.direction)] << int(depth);
    }

    inline auto operator<=>(const Move&) const = default;
};

struct UnitFormula {
    vector<UnitMove> unit_moves;

    inline UnitFormula() = default;

    inline UnitFormula(const vector<UnitMove>& unit_moves)
        : unit_moves(unit_moves) {}

    inline void Print(ostream& os = cout) const {
        for (auto i = 0; i < (int)unit_moves.size(); i++) {
            unit_moves[i].Print(os);
            if (i != (int)unit_moves.size() - 1)
                os << ".";
        }
    }
};

struct Color {
    u8 data;

    inline Color() = default;
    inline Color(const u8 data) : data(data){};

    inline void Display(ostream& os = cout,
                        const bool use_alphabet = true) const {
        if (use_alphabet)
            assert(false);
        else {
            os << int(data);
        }
    }

    inline auto operator<=>(const Color&) const = default;
};

template <int width> struct UnitGlobe {
    static_assert(width % 2 == 0);
    array<array<Color, width>, 2> facelets;
    inline UnitGlobe(const int n_colors) {
        assert(width * 2 % n_colors == 0);
        const int n = width * 2 / n_colors;
        for (int i = 0; i < width * 2; i++)
            facelets[i / width][i % width] = Color((u8)(i / n));
    }
    inline void Rotate(const UnitMove& mov) {
        if (mov.direction == UnitMove::Direction::R) {
            RotateRight(mov.depth);
        } else if (mov.direction == UnitMove::Direction::Rp) {
            RotateLeft(mov.depth);
        } else {
            Flip(mov.depth);
        }
    }
    inline void Rotate(const UnitFormula& formula) {
        for (const auto& unit_move : formula.unit_moves)
            Rotate(unit_move);
    }
    inline void RotateRight(const int depth) {
        assert(depth == 0 || depth == 1);
        const auto tmp = facelets[depth][0];
        memmove(&facelets[depth][0], &facelets[depth][1],
                sizeof(Color) * (width - 1));
        facelets[depth][width - 1] = tmp;
    }
    inline void RotateLeft(const int depth) {
        assert(depth == 0 || depth == 1);
        const auto tmp = facelets[depth][width - 1];
        memmove(&facelets[depth][1], &facelets[depth][0],
                sizeof(Color) * (width - 1));
        facelets[depth][0] = tmp;
    }
    inline void Flip(const int depth) {
        assert(depth >= 0 && depth < width);
        constexpr auto half_width = width / 2;
        for (int i = 0; i < half_width; i++) {
            auto x0 = depth + i;
            if (x0 >= width)
                x0 -= width;
            auto x1 = depth + half_width - 1 - i;
            if (x1 >= width)
                x1 -= width;
            swap(facelets[0][x0], facelets[1][x1]);
        }
    }
    inline auto operator<=>(const UnitGlobe&) const = default;
    struct Hash {
        // using result_type = size_t;
        inline size_t operator()(const UnitGlobe& unit_globe) const {
            auto hash = (size_t)0xcbf29ce484222325ull;
            for (const auto& facelet : unit_globe.facelets) {
                for (const auto& color : facelet) {
                    hash ^= (size_t)color.data;
                    hash *= 0x100000001b3ull;
                }
            }
            return hash;
        }
    };
    inline void Display(ostream& os = cout) const {
        for (int y = 0; y < 2; y++) {
            for (int x = 0; x < width; x++) {
                facelets[y][x].Display(os, false);
                os << " ";
            }
            os << endl;
        }
    }
};

template <int height, int width> struct Globe {
    array<UnitGlobe<width>, height / 2> units;
    inline Globe(const int n_colors) {
        for (auto&& unit : units)
            unit = UnitGlobe<width>(n_colors);
    }
    inline void Rotate(const Move& mov) {
        if (mov.unit_id < 0) {
            assert(mov.unit_move.direction == UnitMove::Direction::F);
            for (auto&& unit : units)
                unit.Flip(mov.unit_move.depth);
        } else {
            assert(mov.unit_move.direction != UnitMove::Direction::F);
            units[mov.unit_id].Rotate(mov.unit_move);
        }
    }
    inline void Display(ostream& os = cout) const {
        for (int y = 0; y < height / 2; y++) {
            for (int x = 0; x < width; x++) {
                units[y].facelets[0][x].Display(os, false);
                os << " ";
            }
            os << endl;
        }
        for (int y = height / 2 - 1; y >= 0; y--) {
            for (int x = 0; x < width; x++) {
                units[y].facelets[1][x].Display(os, false);
                os << " ";
            }
            os << endl;
        }
    }
};

template <int width> struct GlobeFormulaSearcher {
    static constexpr auto kMaxFlipDepth = 4;

    using UnitGlobe = UnitGlobe<width>;
    int max_cost;
    int max_depth;
    UnitGlobe unit_globe;

    inline GlobeFormulaSearcher()
        : max_cost(), max_depth(), unit_globe(width * 2), results(), depth(),
          move_history(), flips(), flip_depth(), n_total_flips() {
        assert(max_depth <= (int)move_history.size());
    }

    auto Search(const int max_cost_, const int max_depth_ = 10,
                const int max_conjugate_depth = 4) {
        max_cost = max_cost_;
        max_depth = max_depth_;
        results.clear();

        // DFS で探索する
        cout << "Searching..." << endl;
        Dfs();
        cout << format("Done. {} formulas found.", results.size()) << endl;

        // 重複を削除する
        cout << "Removing duplicates..." << endl;
        RemoveDuplicates();
        cout << format("Done. {} formulas left.", results.size()) << endl;

        // 最初の Flip を全通りにする
        cout << "Augmenting (flip / slide)..." << endl;
        AugmentFirstFlipRotation();
        cout << format("Done. {} formulas found.", results.size()) << endl;

        // 重複を削除する
        cout << "Removing duplicates..." << endl;
        RemoveDuplicates(true);
        cout << format("Done. {} formulas left.", results.size()) << endl;

        for (auto i = 0; i < max_conjugate_depth; i++) {
            // A と A' で挟んで水増しする
            cout << format("Augmenting (conjugate {}/{})...", i + 1,
                           max_conjugate_depth)
                 << endl;
            AugmentConjugate();
            cout << format("Done. {} formulas found.", results.size()) << endl;

            // 重複を削除する
            cout << "Removing duplicates..." << endl;
            RemoveDuplicates(true);
            cout << format("Done. {} formulas left.", results.size()) << endl;
        }

        return results;
    }

  private:
    vector<UnitFormula> results;
    int depth;
    array<UnitMove, 16> move_history;
    array<UnitMove, kMaxFlipDepth> flips;
    int flip_depth;
    int n_total_flips;

    // 注: 奇数長の手筋は虹で使えない
    inline auto CheckValid() const {
        // 手筋の長さは 1 以上
        if (depth == 0)
            return false;
        // Flip の戻し残しがない
        if (flip_depth != 0)
            return false;
        // 最初と最後が逆操作の関係でない
        // そのようなものは AugmentConjugate() で水増しする
        if (move_history[0].IsOpposite(move_history[depth - 1]))
            return false;
        return true;
    }

    inline void Dfs() {
        const auto valid = CheckValid();
        if (valid) {
            const auto moves = vector<UnitMove>(move_history.begin(),
                                                move_history.begin() + depth);
            results.push_back(UnitFormula{moves});
        }
        if (depth == max_depth) {
            assert(flip_depth == 0);
            return;
        }

        // Flip
        const auto can_increase =
            flip_depth < kMaxFlipDepth && flip_depth < max_depth - depth - 1;
        const auto can_decrease =
            depth >= 2 && flip_depth >= 1 && flip_depth <= max_depth - depth;
        // ここで width / 2 にしていいかは結構非自明っぽいが多分大丈夫
        for (auto i = 0; i < width / 2; i++) {
            // 最初の Flip は 0 で固定して、後で回す
            if (n_total_flips == 0 && i != 0)
                break;
            const auto mov = UnitMove(UnitMove::Direction::F, i);
            // 回転を即戻してはいけない
            if (depth != 0 && move_history[depth - 1] == mov)
                continue;
            const auto increase =
                flip_depth == 0 || flips[flip_depth - 1] != mov;
            if (increase) {
                if (!can_increase)
                    continue;
            } else {
                if (!can_decrease)
                    continue;
            }
            unit_globe.Flip(i);
            n_total_flips++;
            move_history[depth++] = mov;
            if (increase) {
                flips[flip_depth++] = mov;
                Dfs();
                flip_depth--;
            } else {
                flip_depth--;
                Dfs();
                flips[flip_depth++] = mov;
            }
            depth--;
            n_total_flips--;
            unit_globe.Flip(i);
        }

        // Rotate
        if (flip_depth < max_depth - depth)
            for (const auto mov : {UnitMove(UnitMove::Direction::R, 0),
                                   UnitMove(UnitMove::Direction::R, 1),
                                   UnitMove(UnitMove::Direction::Rp, 0),
                                   UnitMove(UnitMove::Direction::Rp, 1)}) {
                // 最初の回転は固定して、後で回す
                if (depth == n_total_flips &&
                    mov != UnitMove(UnitMove::Direction::R, 0))
                    break;
                // 回転を即戻してはいけない
                const auto inv_mov = mov.Inv();
                if (depth != 0 && move_history[depth - 1] == inv_mov)
                    continue;
                unit_globe.Rotate(mov);
                move_history[depth++] = mov;
                Dfs();
                depth--;
                unit_globe.Rotate(inv_mov);
            }
    }

    // 同じ変化の手筋は短いほうだけを残す
    // また、変化の多すぎる手筋も削除する
    inline void
    RemoveDuplicates(const bool do_sanity_check_after_augment = false) {
        assert(unit_globe == UnitGlobe(width * 2));
        sort(results.begin(), results.end(),
             [](const UnitFormula& a, const UnitFormula& b) {
                 if (a.unit_moves.size() != b.unit_moves.size())
                     return a.unit_moves.size() < b.unit_moves.size();
                 return a.unit_moves < b.unit_moves;
             });

        auto top = 0;
        auto found_permutations =
            unordered_set<UnitGlobe, typename UnitGlobe::Hash>();
        for (auto idx_results = 0; idx_results < (int)results.size();
             idx_results++) {
            const auto formula = results[idx_results];
            // 実際に手筋を使って回転させる
            auto tmp_globe = unit_globe;
            tmp_globe.Rotate(formula);
            // 変化が無いか多すぎるなら削除する
            auto n_changes = 0;
            for (auto i = 0; i < width * 2; i++)
                if (tmp_globe.facelets[i / width][i % width].data != i)
                    n_changes++;
            if (n_changes == 0 ||
                n_changes * (int)formula.unit_moves.size() > max_cost)
                continue;
            // 重複があるなら削除する
            const auto [it, inserted] = found_permutations.insert(tmp_globe);
            if (inserted)
                results[top++] = formula;
        }
        results.resize(top);

        if (do_sanity_check_after_augment)
            for (const auto& formula : results) {
                auto tmp_globe = unit_globe;
                tmp_globe.Rotate(UnitMove(UnitMove::Direction::R, 0));
                tmp_globe.Rotate(UnitMove(UnitMove::Direction::R, 1));
                tmp_globe.Rotate(formula);
                tmp_globe.Rotate(UnitMove(UnitMove::Direction::Rp, 0));
                tmp_globe.Rotate(UnitMove(UnitMove::Direction::Rp, 1));
                if (!found_permutations.contains(tmp_globe)) {
                    cout << "Sanity check failed." << endl;
                    cout << "formula: ";
                    formula.Print();
                    cout << endl;
                    tmp_globe.Display();
                    abort();
                }
            }
    }

    inline void AugmentFirstFlipRotation() {
        auto original_results_size = (int)results.size();
        results.reserve(original_results_size * (4 * width));

        // 左右の Augmentation
        for (auto idx_results = 0; idx_results < original_results_size;
             idx_results++) {
            auto formula = results[idx_results];
            for (auto&& unit_move : formula.unit_moves)
                switch (unit_move.direction) {
                case UnitMove::Direction::F:
                    unit_move.depth = width - 1 - unit_move.depth;
                    break;
                case UnitMove::Direction::R:
                    unit_move.direction = UnitMove::Direction::Rp;
                    break;
                case UnitMove::Direction::Rp:
                    unit_move.direction = UnitMove::Direction::R;
                    break;
                }
            results.push_back(formula);
        }

        // 上下の Augmentation
        original_results_size = (int)results.size();
        for (auto idx_results = 0; idx_results < original_results_size;
             idx_results++) {
            auto formula = results[idx_results];
            for (auto&& unit_move : formula.unit_moves)
                if (unit_move.direction != UnitMove::Direction::F)
                    unit_move.depth = 1 - unit_move.depth;
            results.push_back(formula);
        }

        // Flip 箇所の Augmentation
        original_results_size = (int)results.size();
        for (auto i = 1; i < width; i++) {
            for (auto idx_results = 0; idx_results < original_results_size;
                 idx_results++) {
                auto formula = results[idx_results];
                for (auto&& unit_move : formula.unit_moves)
                    if (unit_move.direction == UnitMove::Direction::F)
                        unit_move.depth = (unit_move.depth + i) % width;
                results.push_back(formula);
            }
        }
    }

    inline void AugmentConjugate() {
        const auto original_results_size = (int)results.size();
        results.reserve(original_results_size * (width / 2 + 5));
        // Flip で挟む
        for (auto i = 0; i < width / 2; i++) {
            const auto mov = UnitMove(UnitMove::Direction::F, i);
            for (auto idx_results = 0; idx_results < original_results_size;
                 idx_results++) {
                auto formula = results[idx_results];
                formula.unit_moves.insert(formula.unit_moves.begin(), mov);
                formula.unit_moves.push_back(mov);
                results.push_back(formula);
            }
        }
        // Rotation で挟む
        for (const auto mov : {UnitMove(UnitMove::Direction::R, 0),
                               UnitMove(UnitMove::Direction::R, 1),
                               UnitMove(UnitMove::Direction::Rp, 0),
                               UnitMove(UnitMove::Direction::Rp, 1)}) {
            for (auto idx_results = 0; idx_results < original_results_size;
                 idx_results++) {
                auto formula = results[idx_results];
                formula.unit_moves.insert(formula.unit_moves.begin(), mov);
                formula.unit_moves.push_back(mov.Inv());
                results.push_back(formula);
            }
        }
    }
};

[[maybe_unused]] static void TestGlobe() {
    const auto unit_moves = {
        UnitMove(UnitMove::Direction::R, 0),
        UnitMove(UnitMove::Direction::R, 1),
        UnitMove(UnitMove::Direction::Rp, 0),
        UnitMove(UnitMove::Direction::Rp, 1),
        UnitMove(UnitMove::Direction::F, 0),
        UnitMove(UnitMove::Direction::F, 1),
        UnitMove(UnitMove::Direction::F, 7),
    };
    auto unit_globe = UnitGlobe<8>(8);
    for (const auto unit_move : unit_moves) {
        cout << "unit_move: ";
        unit_move.Print();
        cout << endl;
        unit_globe.Rotate(unit_move);
        unit_globe.Display();
        cout << endl;
    }
}

template <int half_width>
void SearchFormula(const int max_cost, const int max_depth,
                   const int max_conjugate_depth) {
    constexpr auto width = half_width * 2;
    const auto results = GlobeFormulaSearcher<width>().Search(
        max_cost, max_depth, max_conjugate_depth);
    const auto output_filename =
        format("out/globe_formula_{}_{}_{}_{}.txt", half_width, max_cost,
               max_depth, max_conjugate_depth);
    auto ofs = ofstream(output_filename);
    for (const auto& formula : results) {
        formula.Print(ofs);
        ofs << "\n";
    }
}

// max_cost が 56 の場合、変化する箇所が
// 3 箇所なら 18 手以下
// 4 箇所なら 14 手以下
// 5 箇所なら 11 手以下
// 6 箇所なら 9 手以下
// 7 箇所なら 8 手以下
// 8 箇所なら 7 手以下
// 9 箇所なら 6 手以下
// 10 箇所なら 5 手以下
[[maybe_unused]] static void SearchFormula(const int half_width,
                                           const int max_cost = 56,
                                           const int max_depth = 10,
                                           const int max_conjugate_depth = 4) {
    switch (half_width) {
    case 4:
        SearchFormula<4>(max_cost, max_depth, max_conjugate_depth);
        break;
    case 6:
        SearchFormula<6>(max_cost, max_depth, max_conjugate_depth);
        break;
    case 8:
        SearchFormula<8>(max_cost, max_depth, max_conjugate_depth);
        break;
    case 10:
        SearchFormula<10>(max_cost, max_depth, max_conjugate_depth);
        break;
    case 16:
        SearchFormula<16>(max_cost, max_depth, max_conjugate_depth);
        break;
    case 25:
        SearchFormula<25>(max_cost, max_depth, max_conjugate_depth);
        break;
    case 33:
        SearchFormula<33>(max_cost, max_depth, max_conjugate_depth);
        break;
    default:
        cout << "half_width must be 4, 6, 8, 10, 16, 25, 33" << endl;
    }
}

// clang++ -std=c++20 -Wall -Wextra -O3 globe.cpp -DTEST_GLOBE
#ifdef TEST_GLOBE
int main() { TestGlobe(); }
#endif

// clang++ -std=c++20 -Wall -Wextra -O3 globe.cpp -DSEARCH_FORMULA
#ifdef SEARCH_FORMULA
int main(const int argc, const char* const* const argv) {
    switch (argc) {
    case 2:
        SearchFormula(atoi(argv[1]));
        break;
    case 3:
        SearchFormula(atoi(argv[1]), atoi(argv[2]));
        break;
    case 4:
        SearchFormula(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
        break;
    case 5:
        SearchFormula(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]),
                      atoi(argv[4]));
        break;
    default:
        cout << "Usage: " << argv[0]
             << " half_width [max_cost] [max_depth] [max_conjugate_depth]"
             << endl;
    }
}
#endif