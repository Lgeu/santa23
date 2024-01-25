#include <algorithm>
#include <array>
#include <cassert>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

using std::array;
using std::cout;
using std::endl;
using std::format;
using std::ifstream;
using std::istringstream;
using std::make_shared;
using std::min;
using std::ofstream;
using std::ostream;
using std::shared_ptr;
using std::string;
using std::swap;
using std::unordered_set;
using std::vector;

using i8 = signed char;
using u8 = unsigned char;
using u64 = unsigned long long;

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
        : unit_move(direction, direction == UnitMove::Direction::F ? depth
                               : depth < (height + 1) / 2          ? 0
                                                                   : 1),
          unit_id(direction == UnitMove::Direction::F ? -1
                  : depth < (height + 1) / 2          ? depth
                                                      : height - 1 - depth) {
        assert(unit_id < (height + 1) / 2);
    }

    inline Move Inv() const { return Move(unit_move.Inv(), unit_id); }

    template <int height> inline void Print(ostream& os = cout) const {
        assert(unit_id >= -1);
        assert(unit_id < (height + 1) / 2);
        if (unit_move.direction != UnitMove::Direction::F)
            assert(unit_move.depth == 0 || unit_move.depth == 1);
        const auto depth = unit_move.direction == UnitMove::Direction::F
                               ? unit_move.depth
                           : unit_move.depth == 0 ? unit_id
                                                  : height - 1 - unit_id;
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
    static constexpr auto kMaxFlipDepth = 4;

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
                                    const bool is_normal) {
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

        Test();
    }

    inline void Test() const {
        for (auto i = 0; i < 3; i++) {
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

    inline void Apply(const Action& action, const int n_colors) {
        unit_globe.Rotate(action.facelet_changes);
        score = unit_globe.ComputeScore(n_colors);
        n_moves += (int)action.formula.unit_moves.size();
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
    ActionCandidateGenerator action_candidate_generator;
    vector<vector<shared_ptr<Node>>> nodes;

    inline BeamSearchSolver(const int n_colors, const int beam_width,
                            const string& formula_filename,
                            const bool is_normal)
        : n_colors(n_colors), beam_width(beam_width),
          action_candidate_generator(formula_filename, is_normal), nodes() {}

    inline shared_ptr<Node> Solve(const UnitGlobe& initial_unit_globe) {
        auto rng = RandomNumberGenerator(42);
        const auto initial_state = State(initial_unit_globe, n_colors);
        const auto initial_node =
            make_shared<Node>(initial_state, nullptr, Action<width>({}));
        nodes.clear();
        nodes.push_back({initial_node});

        auto minimum_scores = array<int, 32>();
        fill(minimum_scores.begin(), minimum_scores.end(), 9999);
        for (auto current_cost = 0; current_cost < 10000; current_cost++) {
            auto current_minimum_score = 9999;
            for (const auto& node : nodes[current_cost]) {
                current_minimum_score =
                    min(current_minimum_score, node->state.score);
                if (node->state.score == 0) {
                    cout << "Unit solved!" << endl;
                    return node;
                }
                for (const auto& action : action_candidate_generator.actions) {
                    auto new_state = node->state;
                    new_state.Apply(action, n_colors);
                    if (new_state.n_moves >= (int)nodes.size())
                        nodes.resize(new_state.n_moves + 1);
                    if ((int)nodes[new_state.n_moves].size() < beam_width) {
                        nodes[new_state.n_moves].emplace_back(
                            new Node(new_state, node, action));
                    } else {
                        const auto idx = rng.Next() % beam_width;
                        if (new_state.score <
                            nodes[new_state.n_moves][idx]->state.score)
                            nodes[new_state.n_moves][idx].reset(
                                new Node(new_state, node, action));
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
    const auto is_normal = solution_state[0] != 'N';
    getline(iss_sample, token, ','); // problem_id
    assert(stoi(token) == problem_id);
    getline(iss_sample, s_sample_formula, ',');
    const auto sample_formula = Formula(s_sample_formula, n + 1);

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

template <int n, int m> void Solve(const Problem& problem) {
    const auto formula_filename = format("out/globe_formula_{}_56_10_4.txt", m);
    const auto beam_width = 2;

    constexpr auto height = n + 1;
    constexpr auto width = m * 2;
    const auto n_colors =
        problem.is_normal ? GetNColors(problem.n, problem.m) : width * 2;
    auto initial_globe = Globe<height, width>(n_colors);
    initial_globe.RotateInv(problem.sample_formula);
    initial_globe.Display();
    auto solver = BeamSearchSolver<width>(n_colors, beam_width,
                                          formula_filename, problem.is_normal);

    auto display_globe = [&initial_globe](const Formula& solution) {
        auto globe = initial_globe;
        globe.Rotate(solution);
        globe.Display();
    };

    // 解く
    auto result_moves = vector<Move>();
    // TODO: 虹でパリティを合わせる
    assert(problem.is_normal);
    // TODO: 奇数の高さのやつの中心を合わせる
    assert(n % 2 == 1);
    for (auto unit_id = 0; unit_id < (int)initial_globe.units.size();
         unit_id++) {
        cout << format("Solving unit {}/{}...", unit_id + 1,
                       (int)initial_globe.units.size())
             << endl;
        const auto node = solver.Solve(initial_globe.units[unit_id]);

        // 結果を復元する
        for (auto p = node; p->parent != nullptr; p = p->parent) {
            for (auto i = (int)p->last_action.formula.unit_moves.size() - 1;
                 i >= 0; i--) {
                const auto unit_move = p->last_action.formula.unit_moves[i];
                result_moves.emplace_back(
                    unit_move, unit_move.direction == UnitMove::Direction::F
                                   ? -1
                                   : unit_id);
            }
        }
    }

    reverse(result_moves.begin(), result_moves.end());
    const auto solution = Formula(result_moves);
    display_globe(solution);
    cout << solution.moves.size() << endl;
    solution.template Print<height>();
    cout << endl;

    // TODO: 結果を保存する
}

[[maybe_unused]] static void Solve(const int problem_id) {
    const auto filename_puzzles = "../input/puzzles.csv";
    const auto filename_sample = "../input/sample_submission.csv";
    const auto problem =
        ReadKaggleInput(filename_puzzles, filename_sample, problem_id);
    if (problem.n == 1 && problem.m == 8)
        Solve<1, 8>(problem);
    else if (problem.n == 1 && problem.m == 16)
        Solve<1, 16>(problem);
    else if (problem.n == 2 && problem.m == 6)
        Solve<2, 6>(problem);
    else if (problem.n == 3 && problem.m == 4)
        Solve<3, 4>(problem);
    else if (problem.n == 6 && problem.m == 4)
        Solve<6, 4>(problem);
    else if (problem.n == 6 && problem.m == 8)
        Solve<6, 8>(problem);
    else if (problem.n == 6 && problem.m == 10)
        Solve<6, 10>(problem);
    else if (problem.n == 3 && problem.m == 33)
        Solve<3, 33>(problem);
    else if (problem.n == 8 && problem.m == 25)
        Solve<8, 25>(problem);
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
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " problem_id" << endl;
        return 1;
    }
    Solve(atoi(argv[1]));
}
#endif