#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstring>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

using std::array;
using std::cout;
using std::endl;
using std::fill;
using std::format;
using std::ifstream;
using std::istringstream;
using std::make_shared;
using std::min;
using std::move;
using std::ofstream;
using std::ostream;
using std::shared_ptr;
using std::string;
using std::swap;
using std::thread;
using std::unordered_set;
using std::vector;
using ios = std::ios;

using i8 = signed char;
using u8 = unsigned char;
using u64 = unsigned long long;

struct Timer {
    std::chrono::system_clock::time_point start;
    inline Timer() { start = std::chrono::system_clock::now(); }
    string operator()() {
        int elapsed =
            (int)(std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now() - start)
                      .count()) /
            1000;
        return "elapsed time: " + std::to_string(elapsed);
    }
    inline void Print(ostream& os = cout) const {
        int elapsed =
            (int)(std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now() - start)
                      .count()) /
            1000;
        os << "elapsed time: " + std::to_string(elapsed) << endl;
    }
};

struct RandomNumberGenerator {
  private:
    u64 seed;

  public:
    inline RandomNumberGenerator(const u64 seed) : seed(seed) {
        assert(seed != 0);
    }
    inline auto Next() {
        seed ^= seed << 9;
        seed ^= seed >> 7;
        return seed;
    }
};

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
        : unit_move(unit_move), unit_id(unit_id) {
        if (unit_move.direction == UnitMove::Direction::F)
            assert(unit_id == -1);
        else
            assert(unit_id >= 0);
    }

    inline Move(const UnitMove::Direction direction, const int depth,
                const int height)
        : unit_move(direction, direction == UnitMove::Direction::F
                                   ? depth
                                   : depth < (height + 1) / 2 ? 0 : 1),
          unit_id(direction == UnitMove::Direction::F
                      ? -1
                      : depth < (height + 1) / 2 ? depth : height - 1 - depth) {
        assert(unit_id < (height + 1) / 2);
    }

    inline Move Inv() const { return Move(unit_move.Inv(), unit_id); }

    template <int height> inline void Print(ostream& os = cout) const {
        assert(unit_id >= -1);
        assert(unit_id < (height + 1) / 2);
        if (unit_move.direction != UnitMove::Direction::F)
            assert(unit_move.depth == 0 || unit_move.depth == 1);
        const auto depth =
            unit_move.direction == UnitMove::Direction::F
                ? unit_move.depth
                : unit_move.depth == 0 ? unit_id : height - 1 - unit_id;
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

    inline UnitFormula(const string& s) {
        assert(s.size() >= 2);
        auto iss = istringstream(s);
        string token;
        while (getline(iss, token, '.')) {
            assert(token.size() >= 2);
            auto inv = false;
            if (token[0] == '-') {
                inv = true;
                token = token.substr(1);
            }
            assert(token.size() >= 2);
            const auto direction = [&]() {
                if (token[0] == 'r')
                    return UnitMove::Direction::R;
                if (token[0] == 'f')
                    return UnitMove::Direction::F;
                assert(false);
                return UnitMove::Direction::R;
            }();
            const auto depth = (i8)stoi(token.substr(1));
            auto unit_move = UnitMove(direction, depth);
            if (inv)
                unit_move = unit_move.Inv();
            unit_moves.push_back(unit_move);
        }
    }

    inline auto operator<=>(const UnitFormula&) const = default;

    inline void Print(ostream& os = cout) const {
        for (auto i = 0; i < (int)unit_moves.size(); i++) {
            unit_moves[i].Print(os);
            if (i != (int)unit_moves.size() - 1)
                os << ".";
        }
    }
};

struct FaceletChanges {
    struct Change {
        u8 from, to;
    };
    vector<Change> changes;
};

struct Formula {
    vector<Move> moves;
    inline Formula(const vector<Move>& moves) : moves(moves) {}
    inline Formula(const string& s, const int height) {
        assert(s.size() >= 2);
        auto iss = istringstream(s);
        string token;
        while (getline(iss, token, '.')) {
            assert(token.size() >= 2);
            auto inv = false;
            if (token[0] == '-') {
                inv = true;
                token = token.substr(1);
            }
            assert(token.size() >= 2);
            const auto direction = [&]() {
                if (token[0] == 'r')
                    return UnitMove::Direction::R;
                if (token[0] == 'f')
                    return UnitMove::Direction::F;
                assert(false);
                return UnitMove::Direction::R;
            }();
            const auto depth = (i8)stoi(token.substr(1));
            auto mov = Move(direction, depth, height);
            if (inv)
                mov = mov.Inv();
            moves.push_back(mov);
        }
    }
    template <int height> inline void Print(ostream& os = cout) const {
        for (auto i = 0; i < (int)moves.size(); i++) {
            moves[i].Print<height>(os);
            if (i != (int)moves.size() - 1)
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
    inline UnitGlobe() = default;
    inline UnitGlobe(const int n_colors) {
        assert(width * 2 % n_colors == 0);
        const auto n = width * 2 / n_colors;
        for (auto i = 0; i < width * 2; i++)
            facelets[i / width][i % width] = Color((u8)(i / n));
        assert(facelets[1][width - 1] == Color(n_colors - 1));
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
    inline void Rotate(const FaceletChanges& facelet_changes) {
        static_assert(sizeof(char) == sizeof(Color));
        array<char, width * 2> tmp;
        for (auto i = 0; i < (int)facelet_changes.changes.size(); i++)
            tmp[i] = ((char*)&facelets)[facelet_changes.changes[i].from];
        for (auto i = 0; i < (int)facelet_changes.changes.size(); i++)
            ((char*)&facelets)[facelet_changes.changes[i].to] = tmp[i];
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
    inline auto ComputeScore(const int n_colors) const {
        // TOOD: 差分計算
        assert(width * 2 % n_colors == 0);
        const auto n = width * 2 / n_colors;
        auto score = 0;
        for (auto i = 0; i < width * 2; i++)
            score += (int)facelets[i / width][i % width].data != i / n;
        return score;
    }
    inline auto ComputeScoreAffectedBy(const FaceletChanges& facelet_changes,
                                       const int n_colors) const {
        assert(width * 2 % n_colors == 0);
        const auto n = width * 2 / n_colors;
        auto score = 0;
        for (auto& change : facelet_changes.changes) {
            score +=
                (int)facelets[change.from / width][change.from % width].data !=
                change.from / n;
        }
        return score;
    }
    // 虹のみ
    inline auto ComputeParity() const {
        auto parity = false;
        auto tmp_globe = UnitGlobe(*this);
        auto data = (u8*)&tmp_globe.facelets;
        for (auto i = 0; i < width * 2; i++) {
            auto p = (int)data[i];
            while (i != p) {
                std::swap(data[i], data[p]);
                parity = !parity;
                p = (int)data[i];
            }
        }
        return parity;
    }
    static inline auto ComputeFaceletChanges(const UnitFormula& formula) {
        auto unit_globe = UnitGlobe(width * 2);
        unit_globe.Rotate(formula);
        auto facelet_changes = FaceletChanges();
        for (auto i = 0; i < width * 2; i++)
            if (unit_globe.facelets[i / width][i % width].data != i)
                facelet_changes.changes.push_back(
                    {unit_globe.facelets[i / width][i % width].data, (u8)i});
        return facelet_changes;
    }
    inline auto operator<=>(const UnitGlobe&) const = default;
    struct Hash {
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

// 赤道は扱わない
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
            if constexpr (height % 2 == 1)
                if (mov.unit_id == height / 2)
                    return;
            units[mov.unit_id].Rotate(mov.unit_move);
        }
    }
    inline void Rotate(const Formula& formula) {
        for (const auto& mov : formula.moves)
            Rotate(mov);
    }
    inline void RotateInv(const Formula& formula) {
        for (auto i = (int)formula.moves.size() - 1; i >= 0; i--)
            Rotate(formula.moves[i].Inv());
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
    static constexpr auto kMaxFlipDepth = 5;

    using UnitGlobe = ::UnitGlobe<width>;
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

        auto n_results = results.size();
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

            if (results.size() == n_results) {
                cout << "No more conjugate augmentation." << endl;
                break;
            }
            n_results = results.size();
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

template <int width> struct Action {
    UnitFormula formula;
    FaceletChanges facelet_changes;

    inline Action(const UnitFormula& formula)
        : formula(formula),
          facelet_changes(UnitGlobe<width>::ComputeFaceletChanges(formula)) {}
};

template <int width> struct ActionCandidateGenerator {
    using UnitGlobe = ::UnitGlobe<width>;
    using Action = ::Action<width>;
    vector<Action> actions;

    // ファイルから手筋を読み取る
    // ファイルには f1.r1.-r0.f1 みたいなのが 1 行に 1 つ書かれている想定
    inline ActionCandidateGenerator(const string& filename,
                                    const bool is_normal)
        : actions() {
        auto ifs = ifstream(filename);
        if (!ifs.good()) {
            cout << "Failed to open " << filename << endl;
            abort();
        }
        string line;
        while (getline(ifs, line)) {
            if (line.empty() || line[0] == '#')
                continue;
            const auto formula = UnitFormula(line);
            // 虹は偶数長の手筋だけ使う
            // 偶数長であれば状態の置換の偶奇が入れ替わらないため
            if (!is_normal && formula.unit_moves.size() % 2 != 0)
                continue;
            actions.emplace_back(formula);
        }

        SanityCheck();
    }

    inline void SanityCheck() const {
        for (auto i = 0; i < (int)actions.size(); i++) {
            const auto action = actions[i];
            auto cube0 = UnitGlobe(width * 2);
            cube0.Rotate(action.formula);
            auto cube1 = UnitGlobe(width * 2);
            cube1.Rotate(action.facelet_changes);
            assert(cube0 == cube1);
        }
    }
};

template <int width> struct State {
    using UnitGlobe = ::UnitGlobe<width>;
    using Action = ::Action<width>;
    UnitGlobe unit_globe;
    int score;   // 目標との距離
    int n_moves; // これまでに回した回数

    inline State(const UnitGlobe& unit_globe, const int n_colors)
        : unit_globe(unit_globe), score(unit_globe.ComputeScore(n_colors)),
          n_moves() {}

    // this function calculate score correction by concatenating actions
    // actions itself will need to be corrected after path restoration
    inline static int CostCorrection(const Action& last_action,
                                     const Action& action) {
        if (last_action.formula.unit_moves.empty())
            return 0;
        int n_last_actions = ssize(last_action.formula.unit_moves);
        int n_min_actions =
            std::max(n_last_actions, (int)action.formula.unit_moves.size());
        int correction = 0;
        // cout << n_min_actions << endl;
        for (int i = 0; i < n_min_actions; i++) {
            if (last_action.formula.unit_moves[n_last_actions - 1 - i]
                    .IsOpposite(action.formula.unit_moves[i])) {
                correction -= 2;
            } else {
                break;
            }
        }
        // cout << "correction end" << endl;
        return correction;
    }

    inline void Apply(const Action& action, const int n_colors,
                      const Action& last_action) {
        // cout << "start apply" << endl;
        auto score_diff =
            unit_globe.ComputeScoreAffectedBy(action.facelet_changes, n_colors);
        unit_globe.Rotate(action.facelet_changes);
        score_diff = unit_globe.ComputeScoreAffectedBy(action.facelet_changes,
                                                       n_colors) -
                     score_diff;
        score += score_diff;
        n_moves += (int)action.formula.unit_moves.size();
        // cout << "start correction" << endl;
        n_moves += CostCorrection(last_action, action);
        // cout << "end correction" << endl;
        // cout << "end apply" << endl;
        if (n_moves < 0) {
            n_moves = 0; // TODO: fix this if possible. modify past actions
                         // dynamically not to remove the same action twice
            // assert(n_moves >= 0);
        }
    }
};

template <int width> struct Node {
    using State = ::State<width>;
    using Action = ::Action<width>;
    State state;
    shared_ptr<Node> parent;
    Action last_action;
    inline Node(const State& state, const shared_ptr<Node>& parent,
                const Action& last_action)
        : state(state), parent(parent), last_action(last_action) {

        if (parent != nullptr) {
            auto globe = parent->state.unit_globe;
            globe.Rotate(last_action.formula);
            assert(globe == state.unit_globe);
        }
    }
};

template <int width> struct BeamSearchSolver {
    using State = ::State<width>;
    using UnitGlobe = ::UnitGlobe<width>;
    using Node = ::Node<width>;
    using ActionCandidateGenerator = ::ActionCandidateGenerator<width>;

    int n_colors;
    int beam_width;
    int n_threads;
    ActionCandidateGenerator action_candidate_generator;
    vector<vector<shared_ptr<Node>>> nodes;

    inline BeamSearchSolver(const int n_colors, const int beam_width,
                            const string& formula_filename,
                            const bool is_normal, const int n_threads = 16)
        : n_colors(n_colors), beam_width(beam_width), n_threads(n_threads),
          action_candidate_generator(formula_filename, is_normal), nodes() {}

    inline shared_ptr<Node> Solve(const UnitGlobe& initial_unit_globe) {
        auto timer = Timer();
        vector<RandomNumberGenerator> rngs;
        for (int i = 0; i < n_threads; ++i)
            rngs.emplace_back(RandomNumberGenerator(42 + i));
        // auto rngs = RandomNumberGenerator(42);

        assert(n_threads >= 1);
        assert(beam_width % n_threads == 0); // for simplicity
        const auto initial_state = State(initial_unit_globe, n_colors);
        const auto initial_node =
            make_shared<Node>(initial_state, nullptr, Action<width>({}));
        nodes.clear();
        [[maybe_unused]] int max_action_cost = 0;
        vector<vector<vector<shared_ptr<Node>>>> nodes_thread;
        if (n_threads == 1) {
            nodes.push_back({initial_node});
        } else {
            // largest case (width=33) can be solved within ~800
            constexpr int estimated_max_cost = 1500;
            nodes.resize(estimated_max_cost,
                         vector<shared_ptr<Node>>(beam_width, initial_node));
            for (auto const& action : action_candidate_generator.actions)
                max_action_cost = std::max(
                    estimated_max_cost, (int)action.formula.unit_moves.size());
            nodes_thread.resize(
                n_threads, vector<vector<shared_ptr<Node>>>(
                               estimated_max_cost, vector<shared_ptr<Node>>(
                                                    beam_width, initial_node)));
        }

        auto minimum_scores = array<int, 32>();
        fill(minimum_scores.begin(), minimum_scores.end(), 9999);
        for (auto current_cost = 0; current_cost < 10000; current_cost++) {
            auto current_minimum_score = 9999;
            if (n_threads == 1) {
                for (const auto& node : nodes[current_cost]) {
                    current_minimum_score =
                        min(current_minimum_score, node->state.score);
                    if (node->state.score == 0) {
                        cout << "Unit solved!" << endl;
                        timer.Print();
                        return node;
                    }
                    for (const auto& action :
                        action_candidate_generator.actions) {
                        auto new_state = node->state;
                        new_state.Apply(action, n_colors, node->last_action);
                        if (new_state.n_moves <= current_cost)
                            continue;
                        if (new_state.n_moves >= (int)nodes.size())
                            nodes.resize(new_state.n_moves + 1);
                        if ((int)nodes[new_state.n_moves].size() < beam_width) {
                            nodes[new_state.n_moves].emplace_back(
                                new Node(new_state, node, action));
                        } else {
                            const auto idx = rngs[0].Next() % beam_width;
                            if (new_state.score <
                                nodes[new_state.n_moves][idx]->state.score)
                                nodes[new_state.n_moves][idx].reset(
                                    new Node(new_state, node, action));
                        }
                    }
                }
            } else {
                vector<thread> threads;
                for (const auto& node: nodes[current_cost]) {
                    current_minimum_score =
                        min(current_minimum_score, node->state.score);
                    if (node->state.score == 0) {
                        cout << "Unit solved!" << endl;
                        timer.Print();
                        return node;
                    }
                }
                for (int ith = 0; ith < n_threads; ++ith) {
                    thread th([&](int ii){
                        for (int k = ii; k < beam_width; k += n_threads) {
                            for (const auto& action: action_candidate_generator.actions) {
                                auto new_state = nodes[current_cost][k]->state;
                                new_state.Apply(action, n_colors, nodes[current_cost][k]->last_action);
                                if (new_state.n_moves <= current_cost)
                                    continue;
                                const auto idx = rngs[ii].Next() % beam_width;
                                if (new_state.score <
                                    nodes_thread[ii][new_state.n_moves][idx]->state.score)
                                    nodes_thread[ii][new_state.n_moves][idx].reset(
                                        new Node(new_state, nodes[current_cost][k], action));
                            }
                        }
                    }, ith);
                    threads.emplace_back(move(th));
                }
                for (auto& th: threads)
                    th.join();
                for (int ith = 0; ith < n_threads; ++ith) {
                    for (int k = 0; k < beam_width; ++k) {
                        // for (int c = current_cost + 1; c <= current_cost + max_action_cost; ++c) {
                        for (int c = current_cost + 1; c <= current_cost + 1; ++c) {
                            if (nodes_thread[ith][c][k]->state.score < nodes[c][k]->state.score)
                                nodes[c][k] = nodes_thread[ith][c][k];
                        }
                    }
                }
            }
            cout << format("current_cost: {}, current_minimum_score: {}",
                           current_cost, current_minimum_score)
                 << endl;
            if (!nodes[current_cost].empty() &&
                current_minimum_score ==
                    minimum_scores[current_cost % minimum_scores.size()]) {
                cout << "Failed." << endl;
                nodes[current_cost][0]->state.unit_globe.Display();
                return nullptr;
            }
            minimum_scores[current_cost % minimum_scores.size()] =
                current_minimum_score;
            nodes[current_cost].clear();
        }
        cout << "Failed." << endl;
        return nullptr;
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

struct Problem {
    int problem_id;
    int n, m;
    bool is_normal;
    Formula sample_formula;
};

[[maybe_unused]] static auto ReadKaggleInput(const string& filename_puzzles,
                                             const string& filename_sample,
                                             const int problem_id) {
    if (problem_id < 338 || problem_id > 397) {
        cout << "problem_id must be in [338, 397]" << endl;
        abort();
    }

    auto ifs_puzzles = ifstream(filename_puzzles);
    if (!ifs_puzzles.good()) {
        cout << "Failed to open " << filename_puzzles << endl;
        abort();
    }
    auto ifs_sample = ifstream(filename_sample);
    if (!ifs_sample.good()) {
        cout << "Failed to open " << filename_sample << endl;
        abort();
    }
    string s_puzzles, s_sample;
    for (auto i = 0; i <= problem_id + 1; i++) {
        getline(ifs_puzzles, s_puzzles);
        getline(ifs_sample, s_sample);
    }

    string puzzle_type, s_sample_formula, solution_state;

    auto iss_puzzles = istringstream(s_puzzles);
    auto iss_sample = istringstream(s_sample);
    string token;
    getline(iss_puzzles, token, ','); // problem_id
    assert(stoi(token) == problem_id);
    getline(iss_puzzles, puzzle_type, ',');
    // "globe_6/4" のような文字列から、n と m を取り出す
    auto iss_puzzle_type = istringstream(puzzle_type);
    getline(iss_puzzle_type, token, '_');
    getline(iss_puzzle_type, token, '/');
    const auto n = stoi(token);
    getline(iss_puzzle_type, token, '/');
    const auto m = stoi(token);
    getline(iss_puzzles, solution_state, ',');
    const auto is_normal = solution_state[0] != 'N'; // TODO: 修正
    getline(iss_sample, token, ',');                 // problem_id
    assert(stoi(token) == problem_id);
    getline(iss_sample, s_sample_formula, ',');
    const auto sample_formula = Formula(s_sample_formula, n + 1);

    cout << format("problem_id: {}, n: {}, m: {}, is_normal: {}", problem_id, n,
                   m, is_normal)
         << endl;

    return Problem{problem_id, n, m, is_normal, sample_formula};
};

[[maybe_unused]] static auto GetNColors(const int n, const int m) {
    if (n == 1 && m == 8)
        return 16;
    else if (n == 1 && m == 16)
        return 16;
    else if (n == 2 && m == 6)
        return 8;
    else if (n == 3 && m == 4)
        return 8;
    else if (n == 6 && m == 4)
        return 8;
    else if (n == 6 && m == 8)
        return 16;
    else if (n == 6 && m == 10)
        return 10;
    else if (n == 3 && m == 33)
        return 22;
    else if (n == 8 && m == 25)
        return 20;
    else {
        cout << format("n = {}, m = {} is not supported", n, m) << endl;
        abort();
    }
}

[[maybe_unused]] static void TestReadKaggleInput() {
    constexpr auto n = 1, m = 8;
    const auto filename_puzzles = "../input/puzzles.csv";
    const auto filename_sample = "../input/sample_submission.csv";
    const auto problem_id = 338;
    const auto problem =
        ReadKaggleInput(filename_puzzles, filename_sample, problem_id);
    cout << format("problem_id: {}, n: {}, m: {}, is_normal: {}",
                   problem.problem_id, problem.n, problem.m, problem.is_normal)
         << endl;
    problem.sample_formula.template Print<7>();
    cout << endl;
    const auto n_colors = GetNColors(problem.n, problem.m);
    cout << "n_colors: " << GetNColors(problem.n, problem.m) << endl;
    auto globe = Globe<n + 1, m * 2>(n_colors);
    globe.RotateInv(problem.sample_formula);
    globe.Display();
}

struct Depth {
    vector<int> depth;
    inline Depth() : depth(1, -1){};
    inline void Push(int d) {
        if (depth.back() == d)
            depth.pop_back();
        else
            depth.push_back(d);
    }
    bool operator==(const Depth& other) const { return depth == other.depth; }
};

vector<Move> MergeResultMoves(vector<Move> const& result_moves1,
                              vector<Move> const& result_moves2) {
    if (result_moves1.empty())
        return result_moves2;
    if (result_moves2.empty())
        return result_moves1;

    // Formula(result_moves1).Print<4>();
    // cout << endl;
    // Formula(result_moves2).Print<4>();
    // cout << endl;

    vector<Move> merged_moves;
    vector<vector<std::pair<Depth, int>>> depth_change(2);
    vector<vector<Move>> result_moves_by_unit = {result_moves1, result_moves2};

    cout << "2 UnitGlobe solutions before merge: "
         << ssize(result_moves_by_unit[0]) << " "
         << ssize(result_moves_by_unit[1]) << endl;
    for (int i = 0; i < 2; ++i) {
        Depth curr_depth; // sentinel
        for (auto& move : result_moves_by_unit[i]) {
            if (move.unit_move.direction == UnitMove::Direction::F) {
                auto prev_depth = curr_depth;
                curr_depth.Push(move.unit_move.depth);
                depth_change[i].push_back({prev_depth, move.unit_move.depth});
                // for(auto d : curr_depth)
                //     cout << d << " ";
                // cout << endl;
            }
        }
        // cout << endl;
        assert(curr_depth.depth.size() == 1 && curr_depth.depth[0] == -1);
    }
    // Find the longest common subsequence
    // can remove Fs * len(LCS)
    vector<vector<int>> dp(depth_change[0].size() + 1,
                           vector<int>(depth_change[1].size() + 1, 0));
    for (int i = 0; i < ssize(depth_change[0]); ++i) {
        for (int j = 0; j < ssize(depth_change[1]); ++j) {
            if (depth_change[0][i] == depth_change[1][j]) {
                dp[i + 1][j + 1] = dp[i][j] + 1;
            } else {
                dp[i + 1][j + 1] = std::max(dp[i + 1][j], dp[i][j + 1]);
            }
        }
    }
    auto lcs = dp[ssize(depth_change[0])][ssize(depth_change[1])];
    cout << "LCS: " << lcs << endl;

    // vector<vector<int>> merged_depth;
    vector<std::pair<Depth, int>> fs;
    // merged_depth.push_back({-1}); // both should be in depth0 state at the
    // end
    int idx0 = ssize(depth_change[0]) - 1, idx1 = ssize(depth_change[1]) - 1;
    while (idx0 >= 0 || idx1 >= 0) {
        if (idx0 < 0) {
            fs.push_back(depth_change[1][idx1]);
            --idx1;
        } else if (idx1 < 0) {
            fs.push_back(depth_change[0][idx0]);
            --idx0;
        } else if (depth_change[0][idx0] == depth_change[1][idx1]) {
            fs.push_back(depth_change[0][idx0]);
            --idx0;
            --idx1;
        } else if (dp[idx0][idx1 + 1] > dp[idx0 + 1][idx1]) {
            fs.push_back(depth_change[0][idx0]);
            --idx0;
        } else {
            fs.push_back(depth_change[1][idx1]);
            --idx1;
        }
    }
    // reverse(merged_depth.begin(), merged_depth.end());
    // cout << "merged_depth: " << ssize(merged_depth) << endl;
    // for(auto& d : merged_depth) {
    //     for(auto dd : d)
    //         cout << dd << " ";
    //     cout << endl;
    // }
    reverse(fs.begin(), fs.end());
    // cout << "fs: " << ssize(fs) << endl;
    // for(auto f : fs)
    //     cout << f.second << " ";
    // cout << endl;

    idx0 = 0, idx1 = 0;
    Depth curr_depth0, curr_depth1;
    for (auto& [depth, f] : fs) {
        // cout << "f: " << f << " idx0 " << idx0 << " idx1 " << idx1 << endl;
        // for (auto& d: depth.depth) {
        //     cout << d << " ";
        // }
        // cout << endl;
        // for (auto& d: curr_depth0.depth) {
        //     cout << d << " ";
        // }
        // cout << endl;
        // for (auto& d: curr_depth1.depth) {
        //     cout << d << " ";
        // }
        // cout << endl;
        while (result_moves_by_unit[0][idx0].unit_move.direction !=
                   UnitMove::Direction::F &&
               idx0 < ssize(result_moves_by_unit[0])) {
            // cout << "idx0++";
            // result_moves_by_unit[0][idx0].unit_move.Print();
            // cout << endl;
            merged_moves.push_back(result_moves_by_unit[0][idx0++]);
        }
        while (result_moves_by_unit[1][idx1].unit_move.direction !=
                   UnitMove::Direction::F &&
               idx1 < ssize(result_moves_by_unit[1])) {
            // cout << "idx1++";
            // result_moves_by_unit[1][idx1].unit_move.Print();
            // cout << endl;
            merged_moves.push_back(result_moves_by_unit[1][idx1++]);
        }
        if (f != -1)
            merged_moves.push_back(Move(UnitMove::Direction::F, f, -1));

        bool ok = f == -1;
        if (idx0 < ssize(result_moves_by_unit[0]) &&
            result_moves_by_unit[0][idx0].unit_move.depth == f &&
            curr_depth0 == depth) {
            // cout << "idx0++ f" <<
            // (int)result_moves_by_unit[0][idx0].unit_move.depth << endl;
            ok = true;
            idx0++;
            curr_depth0.Push(f);
        }
        if (idx1 < ssize(result_moves_by_unit[1]) &&
            result_moves_by_unit[1][idx1].unit_move.depth == f &&
            curr_depth1 == depth) {
            // cout << "idx1++ f" <<
            // (int)result_moves_by_unit[1][idx1].unit_move.depth << endl;
            ok = true;
            idx1++;
            curr_depth1.Push(f);
        }
        assert(ok);
    }
    while (idx0 < ssize(result_moves_by_unit[0])) {
        merged_moves.push_back(result_moves_by_unit[0][idx0++]);
    }
    while (idx1 < ssize(result_moves_by_unit[1])) {
        merged_moves.push_back(result_moves_by_unit[1][idx1++]);
    }
    cout << "merged_moves: " << ssize(merged_moves) << endl;
    assert(ssize(merged_moves) == ssize(result_moves_by_unit[0]) +
                                      ssize(result_moves_by_unit[1]) - lcs);
    // Formula(merged_moves).Print<8>();
    // cout << endl;

    return merged_moves;
}

template <int n, int m>
void Solve(const Problem& problem, const int beam_width = 2,
           const int max_cost = 56, const int max_depth = 10,
           const int max_conjugate_depth = 4, const int n_threads = 1) {
    const auto formula_file = format("out/globe_formula_{}_{}_{}_{}.txt", m,
                                     max_cost, max_depth, max_conjugate_depth);
    constexpr auto height = n + 1;
    constexpr auto width = m * 2;
    const auto n_colors =
        problem.is_normal ? GetNColors(problem.n, problem.m) : width * 2;
    auto initial_globe = Globe<height, width>(n_colors);
    initial_globe.RotateInv(problem.sample_formula);
    initial_globe.Display();

    auto display_globe = [&initial_globe](const Formula& solution) {
        auto globe = initial_globe;
        globe.Rotate(solution);
        globe.Display();
    };

    // 解く
    auto result_moves = vector<Move>();
    auto globe = initial_globe;
    // 虹でパリティを合わせる
    if (!problem.is_normal) {
        cout << "Parity: ";
        for (auto i = 0; i < (int)globe.units.size(); i++) {
            auto parity = globe.units[i].ComputeParity();
            cout << parity;
            if (parity) {
                const auto mov = Move(UnitMove::Direction::R, i, height);
                globe.Rotate(mov);
                result_moves.emplace_back(mov);
            }
        }
        cout << endl;
    }
    // 赤道を揃える
    if constexpr (n % 2 == 0) {
        auto delta = 0;
        for (const auto mov : problem.sample_formula.moves) {
            if (mov.unit_id == height / 2) {
                if (mov.unit_move.direction == UnitMove::Direction::R)
                    delta++;
                else if (mov.unit_move.direction == UnitMove::Direction::Rp)
                    delta--;
                else
                    assert(false);
            }
        }
        delta = (delta % width + width) % width;
        if (delta > width / 2)
            delta -= width;
        cout << format("Equator: {}", delta) << endl;
        for (; delta > 0; delta--) {
            const auto mov = Move(UnitMove::Direction::R, height / 2, height);
            globe.Rotate(mov);
            result_moves.emplace_back(mov);
        }
        for (; delta < 0; delta++) {
            const auto mov = Move(UnitMove::Direction::Rp, height / 2, height);
            globe.Rotate(mov);
            result_moves.emplace_back(mov);
        }
    }
    // 赤道以外をビームサーチで揃える
    // auto n_pre_rotations = (int)result_moves.size();
    auto solver = BeamSearchSolver<width>(n_colors, beam_width, formula_file,
                                          problem.is_normal, n_threads);
    auto result_moves_by_unit = vector<vector<Move>>(globe.units.size());
    for (auto unit_id = 0; unit_id < (int)globe.units.size(); unit_id++) {
        cout << format("Solving unit {}/{}...", unit_id + 1,
                       (int)globe.units.size())
             << endl;
        const auto node = solver.Solve(globe.units[unit_id]);

        // 結果を復元する
        for (auto p = node; p->parent != nullptr; p = p->parent) {
            for (auto i = (int)p->last_action.formula.unit_moves.size() - 1;
                 i >= 0; i--) {
                const auto unit_move = p->last_action.formula.unit_moves[i];
                // Concatenate moves
                Move next_move(unit_move,
                               unit_move.direction == UnitMove::Direction::F
                                   ? -1
                                   : unit_id);
                if (result_moves_by_unit[unit_id].empty() ||
                    not result_moves_by_unit[unit_id]
                            .back()
                            .unit_move.IsOpposite(unit_move)) {
                    result_moves_by_unit[unit_id].emplace_back(
                        unit_move, unit_move.direction == UnitMove::Direction::F
                                       ? -1
                                       : unit_id);
                } else {
                    result_moves_by_unit[unit_id].pop_back();
                }
            }
        }
        reverse(result_moves_by_unit[unit_id].begin(),
                result_moves_by_unit[unit_id].end());
    }

    cout << "merge moves" << endl;
    for (auto& moves : result_moves_by_unit) {
        result_moves = MergeResultMoves(result_moves, moves);
        // result_moves.insert(result_moves.end(), moves.begin(), moves.end());
        // // this replicates the original behavior (no merge)
    }
    // reverse(result_moves.begin() + n_pre_rotations, result_moves.end());
    const auto solution = Formula(result_moves);
    display_globe(solution);
    cout << solution.moves.size() << endl;
    solution.template Print<height>();
    cout << endl;

    // 結果を保存する
    const auto all_solutions_file =
        format("solution_globe/{}_all.txt", problem.problem_id);
    const auto best_solution_file =
        format("solution_globe/{}_best.txt", problem.problem_id);
    auto ofs_all = ofstream(all_solutions_file, ios::app);
    if (ofs_all.good()) {
        solution.template Print<height>(ofs_all);
        ofs_all << endl;
        ofs_all << format("score={} beam_width={} formula_file={}",
                          solution.moves.size(), beam_width, formula_file)
                << endl;
        ofs_all.close();
    } else
        cout << "Failed to open " << all_solutions_file << endl;
    auto ifs_best = ifstream(best_solution_file);
    auto best_score = 9999;
    if (ifs_best.good()) {
        string line;
        getline(ifs_best, line); // 解を読み飛ばす
        getline(ifs_best, line); // スコアを読む
        best_score = stoi(line);
        ifs_best.close();
    }
    if ((int)solution.moves.size() < best_score) {
        auto ofs_best = ofstream(best_solution_file);
        if (ofs_best.good()) {
            solution.template Print<height>(ofs_best);
            ofs_best << endl << solution.moves.size() << endl;
            ofs_best.close();
        } else
            cout << "Failed to open " << best_solution_file << endl;
    }
}

[[maybe_unused]] static void
Solve(const int problem_id, const int beam_width = 2, const int max_cost = 56,
      const int max_depth = 10, const int max_conjugate_depth = 4, const int n_threads = 1) {
    const auto filename_puzzles = "../input/puzzles.csv";
    const auto filename_sample = "../input/sample_submission.csv";
    const auto problem =
        ReadKaggleInput(filename_puzzles, filename_sample, problem_id);
    if (problem.n == 1 && problem.m == 8)
        Solve<1, 8>(problem, beam_width, max_cost, max_depth,
                    max_conjugate_depth, n_threads);
    else if (problem.n == 1 && problem.m == 16)
        Solve<1, 16>(problem, beam_width, max_cost, max_depth,
                     max_conjugate_depth, n_threads);
    else if (problem.n == 2 && problem.m == 6)
        Solve<2, 6>(problem, beam_width, max_cost, max_depth,
                    max_conjugate_depth, n_threads);
    else if (problem.n == 3 && problem.m == 4)
        Solve<3, 4>(problem, beam_width, max_cost, max_depth,
                    max_conjugate_depth, n_threads);
    else if (problem.n == 6 && problem.m == 4)
        Solve<6, 4>(problem, beam_width, max_cost, max_depth,
                    max_conjugate_depth, n_threads);
    else if (problem.n == 6 && problem.m == 8)
        Solve<6, 8>(problem, beam_width, max_cost, max_depth,
                    max_conjugate_depth, n_threads);
    else if (problem.n == 6 && problem.m == 10)
        Solve<6, 10>(problem, beam_width, max_cost, max_depth,
                     max_conjugate_depth, n_threads);
    else if (problem.n == 3 && problem.m == 33)
        Solve<3, 33>(problem, beam_width, max_cost, max_depth,
                     max_conjugate_depth, n_threads);
    else if (problem.n == 8 && problem.m == 25)
        Solve<8, 25>(problem, beam_width, max_cost, max_depth,
                     max_conjugate_depth, n_threads);
    else {
        cout << format("n = {}, m = {} is not supported", problem.n, problem.m)
             << endl;
        abort();
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

// clang++ -std=c++20 -Wall -Wextra -O3 globe.cpp -DTEST_READ_KAGGLE_INPUT
#ifdef TEST_READ_KAGGLE_INPUT
int main() { TestReadKaggleInput(); }
#endif

// clang++ -std=c++20 -Wall -Wextra -O3 globe.cpp -DSOLVE
#ifdef SOLVE
int main(const int argc, const char* const* const argv) {
    if (argc < 2) {
        cout << "Usage: " << argv[0]
             << " problem_id [beam_width] [max_cost] [max_depth] "
                "[max_conjugate_depth] [n_threads]"
             << endl;
        return 1;
    }
    switch (argc) {
    case 2:
        Solve(atoi(argv[1]));
        break;
    case 3:
        Solve(atoi(argv[1]), atoi(argv[2]));
        break;
    case 4:
        Solve(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
        break;
    case 5:
        Solve(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]));
        break;
    case 6:
        Solve(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]),
              atoi(argv[5]));
        break;
    case 7:
        Solve(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]),
              atoi(argv[5]), atoi(argv[6]));
        break;
    default:
        cout << "Usage: " << argv[0]
             << " problem_id [beam_width] [max_cost] [max_depth] "
                "[max_conjugate_depth] [n_threads]"
             << endl;
    }
}
#endif