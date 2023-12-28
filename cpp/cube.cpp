#include <array>
#include <cassert>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

using std::array;
using std::cerr;
using std::cout;
using std::endl;
using std::make_shared;
using std::ostream;
using std::shared_ptr;
using std::string;
using std::vector;

using i8 = signed char;
using u64 = unsigned long long;

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

template <int siz> struct RandomNumberTable {
  private:
    array<u64, siz> data;

  public:
    inline RandomNumberTable(const u64 seed) {
        auto rng = RandomNumberGenerator(seed);
        for (auto&& d : data)
            d = rng.Next();
    }
    inline auto Get(const int i) const {
        assert(i <= 0 && i < siz);
        return data[i];
    }
};

struct ColorType6 {
    static constexpr auto kNColors = 6;
    i8 data;

    inline auto operator<=>(const ColorType6&) const = default;

    inline void Print(ostream& os) const { os << (char)('A' + data); }

    friend auto& operator<<(ostream& os, const ColorType6 t) {
        t.Print(os);
        return os;
    }
};

template <int order, typename ColorType_ = ColorType6, bool use_hash = false>
struct Face {
    using ColorType = ColorType_;
    static_assert(sizeof(ColorType) == 1); // 実質最大 24 色なので 1byte

    // 回転した時にいい感じに計算できるような乱数表
    struct RNT {
      public:
        inline RNT(const u64 /*seed*/) {
            // TODO
        }
        inline auto Get(const int /*y*/, const int /*x*/,
                        const ColorType /*color*/) const {
            // TODO
        }
    };

  private:
    array<array<ColorType, order>, order> facelets; // マス
    int orientation;                                // 軸
    u64 hash_value;                                 // ハッシュ値
    shared_ptr<RNT> rnt; // ハッシュ計算の乱数表

  public:
    static inline auto MakeRNT(const u64 seed)
        requires use_hash
    {
        return make_shared<RNT>(seed);
    }

    inline Face()
        requires(!use_hash)
        : facelets(), orientation(), hash_value(), rnt() {}

    inline Face(const RNT& rnt)
        requires use_hash
        : facelets(), orientation(), hash_value(), rnt(rnt) {
        Fill(0);
    }

    // 1 色で埋める
    void Fill(const ColorType color) {
        if constexpr (use_hash)
            hash_value = 0ull;
        for (auto y = 0; y < order; y++)
            for (auto x = 0; x < order; x++) {
                if constexpr (use_hash)
                    hash_value ^= rnt->Get(
                        ((y * order) + x) * ColorType::kNColors + color);
                facelets[y][x] = color;
            }
    }

    // 時計回りに step * 90 度回転
    inline void RotateCW(const int step) {
        if constexpr (use_hash) {
            assert(false); // TODO
        }
        orientation = (orientation - step) & 3;
    }

    inline ColorType Get(const int y, const int x) const {
        assert(0 <= y && y < order);
        assert(0 <= x && x < order);
        switch (orientation) {
        case 0:
            return facelets[y][x];
        case 1:
            return facelets[x][order - 1 - y];
        case 2:
            return facelets[order - 1 - y][order - 1 - x];
        case 3:
            return facelets[order - 1 - x][y];
        default:
            assert(false);
        }
    }

    inline void Set(const int y, const int x, const ColorType color) {
        assert(0 <= y && y < order);
        assert(0 <= x && x < order);

        // 回転操作に対してハッシュの計算が重すぎるような気がする
        if constexpr (use_hash) {
            assert(rnt.get() != nullptr);
            hash_value ^=
                rnt->Get(((y * order) + x) * ColorType::kNColors + Get(y, x));
            hash_value ^=
                rnt->Get(((y * order) + x) * ColorType::kNColors + color);
        }
        switch (orientation) {
        case 0:
            facelets[y][x] = color;
            return;
        case 1:
            facelets[x][order - 1 - y] = color;
            return;
        case 2:
            facelets[order - 1 - y][order - 1 - x] = color;
            return;
        case 3:
            facelets[order - 1 - x][y] = color;
            return;
        default:
            assert(false);
        }
    }

    inline ColorType GetIgnoringOrientation(const int y, const int x) const {
        assert(0 <= y && y < order);
        assert(0 <= x && x < order);
        return facelets[y][x];
    }

    inline auto Hash() const
        requires use_hash
    {
        return hash_value;
    }
};

// 手 (90 度回転)
struct Move {
    enum struct Direction : i8 { F, Fp, D, Dp, R, Rp }; // p は反時計回り
    Direction direction;
    i8 depth;

    inline bool IsClockWise() const { return ((int)direction & 1) == 0; }

    inline void Print(ostream& os = cout) const {
        static constexpr auto direction_strings =
            array<const char*, 6>{"f", "-f", "d", "-d", "r", "-r"};
        os << direction_strings[(int)direction] << (int)depth;
    }

    inline Move Inv() const {
        return {(Direction)((i8)direction ^ (i8)1), depth};
    }

    inline auto operator<=>(const Move&) const = default;
};

// マスの座標
struct FaceletPosition {
    i8 face_id, y, x;
};

// 手筋
struct Formula {
    struct FaceletChange {
        FaceletPosition from, to;
    };

    // 手の列
    vector<Move> moves;
    // facelet_changes を使うかどうか
    bool use_facelet_changes;
    // 手筋を適用することでどのマスがどこに移動するか (TODO)
    vector<FaceletChange> facelet_changes;

    inline Formula(const vector<Move>& moves)
        : moves(moves), use_facelet_changes(false), facelet_changes() {}

    inline Formula(const vector<Move>& moves,
                   const vector<FaceletChange>& facelet_changes)
        : moves(moves), use_facelet_changes(true),
          facelet_changes(facelet_changes) {}

    inline int Cost() const { return (int)moves.size(); }

    // f1.d0.-r0.-f1 のような形式で出力する
    inline void Print(ostream& os = cout) const {
        for (auto i = 0; i < (int)moves.size(); i++) {
            if (i != 0)
                os << '.';
            moves[i].Print(os);
        }
    }
};

// キューブ
template <int order_, typename ColorType_ = ColorType6> struct Cube {
    static constexpr auto order = order_;
    using ColorType = ColorType_;
    enum { D1, F0, R0, F1, R1, D0 };

    array<Face<order, ColorType>, 6> faces;

    inline void Rotate(const Move& mov) {
#define FROM_BOTTOM order - 1 - mov.depth, i
#define FROM_LEFT i, mov.depth
#define FROM_TOP mov.depth, order - 1 - i
#define FROM_RIGHT order - 1 - i, order - 1 - mov.depth
        switch (mov.direction) {
        case Move::Direction::F:
            if (mov.depth == 0)
                faces[F0].RotateCW(1);
            else if (mov.depth == order - 1)
                faces[F1].RotateCW(-1);
            for (auto i = 0; i < order; i++) {
                // clang-format off
                const auto tmp =           faces[R1].Get(FROM_RIGHT);
                faces[R1].Set(FROM_RIGHT,  faces[D0].Get(FROM_TOP));
                faces[D0].Set(FROM_TOP,    faces[R0].Get(FROM_LEFT));
                faces[R0].Set(FROM_LEFT,   faces[D1].Get(FROM_BOTTOM));
                faces[D1].Set(FROM_BOTTOM, tmp);
                // clang-format on
            }
            break;
        case Move::Direction::Fp:
            if (mov.depth == 0)
                faces[F0].RotateCW(-1);
            else if (mov.depth == order - 1)
                faces[F1].RotateCW(1);
            for (auto i = 0; i < order; i++) {
                // clang-format off
                const auto tmp =           faces[D1].Get(FROM_BOTTOM);
                faces[D1].Set(FROM_BOTTOM, faces[R0].Get(FROM_LEFT));
                faces[R0].Set(FROM_LEFT,   faces[D0].Get(FROM_TOP));
                faces[D0].Set(FROM_TOP,    faces[R1].Get(FROM_RIGHT));
                faces[R1].Set(FROM_RIGHT,  tmp);
                // clang-format on
            }
            break;
        case Move::Direction::D:
            if (mov.depth == 0)
                faces[D0].RotateCW(1);
            else if (mov.depth == order - 1)
                faces[D1].RotateCW(-1);
            for (auto i = 0; i < order; i++) {
                // clang-format off
                const auto tmp =           faces[R1].Get(FROM_BOTTOM);
                faces[R1].Set(FROM_BOTTOM, faces[F1].Get(FROM_BOTTOM));
                faces[F1].Set(FROM_BOTTOM, faces[R0].Get(FROM_BOTTOM));
                faces[R0].Set(FROM_BOTTOM, faces[F0].Get(FROM_BOTTOM));
                faces[F0].Set(FROM_BOTTOM, tmp);
                // clang-format on
            }
            break;
        case Move::Direction::Dp:
            if (mov.depth == 0)
                faces[D0].RotateCW(-1);
            else if (mov.depth == order - 1)
                faces[D1].RotateCW(1);
            for (auto i = 0; i < order; i++) {
                // clang-format off
                const auto tmp =           faces[F0].Get(FROM_BOTTOM);
                faces[F0].Set(FROM_BOTTOM, faces[R0].Get(FROM_BOTTOM));
                faces[R0].Set(FROM_BOTTOM, faces[F1].Get(FROM_BOTTOM));
                faces[F1].Set(FROM_BOTTOM, faces[R1].Get(FROM_BOTTOM));
                faces[R1].Set(FROM_BOTTOM, tmp);
                // clang-format on
            }
            break;
        case Move::Direction::R:
            if (mov.depth == 0)
                faces[R0].RotateCW(1);
            else if (mov.depth == order - 1)
                faces[R1].RotateCW(-1);
            for (auto i = 0; i < order; i++) {
                // clang-format off
                const auto tmp =           faces[F0].Get(FROM_RIGHT);
                faces[F0].Set(FROM_RIGHT,  faces[D0].Get(FROM_RIGHT));
                faces[D0].Set(FROM_RIGHT,  faces[F1].Get(FROM_LEFT));
                faces[F1].Set(FROM_LEFT,   faces[D1].Get(FROM_RIGHT));
                faces[D1].Set(FROM_RIGHT,  tmp);
                // clang-format on
            }
            break;
        case Move::Direction::Rp:
            if (mov.depth == 0)
                faces[R0].RotateCW(-1);
            else if (mov.depth == order - 1)
                faces[R1].RotateCW(1);
            for (auto i = 0; i < order; i++) {
                // clang-format off
                const auto tmp =           faces[D1].Get(FROM_RIGHT);
                faces[D1].Set(FROM_RIGHT,  faces[F1].Get(FROM_LEFT));
                faces[F1].Set(FROM_LEFT,   faces[D0].Get(FROM_RIGHT));
                faces[D0].Set(FROM_RIGHT,  faces[F0].Get(FROM_RIGHT));
                faces[F0].Set(FROM_RIGHT,  tmp);
                // clang-format on
            }
            break;
        }
#undef FROM_BOTTOM
#undef FROM_LEFT
#undef FROM_TOP
#undef FROM_RIGHT
    }

    inline void Rotate(const Formula& formula) {
        if (formula.use_facelet_changes)
            assert(false); // TODO
        else
            for (const auto& m : formula.moves)
                Rotate(m);
    }

    inline auto Get(const int face_id, const int y, const int x) const {
        return faces[face_id].Get(y, x);
    }

    inline auto Get(const FaceletPosition& facelet_position) const {
        return Get(facelet_position.face_id, facelet_position.y,
                   facelet_position.x);
    }

    inline auto Set(const int face_id, const int y, const int x,
                    ColorType color) {
        faces[face_id].Set(y, x, color);
    }

    inline void Set(const FaceletPosition& facelet_position, ColorType color) {
        Set(facelet_position.face_id, facelet_position.y, facelet_position.x,
            color);
    }

    inline auto ComputeFaceDiff(const Cube& rhs) const {
        // TODO: 差分計算
        auto diff = 0;
        for (auto face_id = 0; face_id < 6; face_id++)
            for (auto y = 1; y < order - 1; y++)
                for (auto x = 1; x < order - 1; x++)
                    diff += Get(face_id, y, x) != rhs.Get(face_id, y, x);
        return diff;
    }

    // Kaggle フォーマットを読み取る
    inline void Read(string& s) {
        assert(s.size() >= 6 * order * order * 2 - 1);
        auto i = 0;
        for (auto face_id = (i8)0; face_id < 6; face_id++)
            for (auto y = (i8)1; y < order - 1; y++)
                for (auto x = (i8)1; x < order - 1; x++)
                    Set(face_id, y, x, {s[i++] - 'A'});
    }

    inline void Print() const {
        // TODO
    }

    inline void Display(ostream& os = cout) const {
        for (auto y = 0; y < order; y++) {
            for (auto x = 0; x < order; x++)
                os << ' ';
            for (auto x = 0; x < order; x++)
                os << faces[D1].Get(y, x);
            os << '\n';
        }
        for (auto y = 0; y < order; y++) {
            for (const auto face_id : {R1, F0, R0, F1}) {
                for (auto x = 0; x < order; x++)
                    os << faces[face_id].Get(y, x);
            }
            os << '\n';
        }
        for (auto y = 0; y < order; y++) {
            for (auto x = 0; x < order; x++)
                os << ' ';
            for (auto x = 0; x < order; x++)
                os << faces[D0].Get(y, x);
            os << '\n';
        }
    }
};

using Action = Formula;

template <int order, typename ColorType = ColorType6> struct State {
    using Cube = ::Cube<order, ColorType>;
    Cube cube;
    int score;   // target との距離
    int n_moves; // これまでに回した回数

    inline State(const Cube& cube, const Cube& target_cube)
        : cube(cube), score(cube.ComputeFaceDiff(target_cube)), n_moves() {}

    inline auto ComputeNextActionCandidates(const Cube& /*target_cube*/) const {
        auto action_candidates = vector<Action>();
        for (auto d = (i8)0; d < 6; d++) {
            const auto direction = (Move::Direction)d;
            for (auto depth = (i8)0; depth < order; depth++)
                action_candidates.push_back(vector<Move>{{direction, depth}});
        }

        // TODO: L'(n-j) U L'(i) U' L(n-j) U L(i)
        // 回転で頭壊れた

        return action_candidates;
    }

    inline void Apply(const Action& action, const Cube& target_cube) {
        cube.Rotate(action);
        score = cube.ComputeFaceDiff(target_cube);
        n_moves += action.Cost();
    }
};

template <int order, typename ColorType = ColorType6> struct Node {
    using State = ::State<order, ColorType>;
    State state;
    shared_ptr<Node> parent;
    Action last_action;
    inline Node(const State& state, const shared_ptr<Node>& parent,
                const Action& last_action)
        : state(state), parent(parent), last_action(last_action) {}
};

template <int order, typename ColorType = ColorType6> struct BeamSearchSolver {
    using Node = ::Node<order, ColorType>;
    using State = ::State<order, ColorType>;
    using Cube = ::Cube<order, ColorType>;
    vector<vector<shared_ptr<Node>>> nodes;

    inline shared_ptr<Node> Solve(const State& initial_state,
                                  const Cube& target_cube) {
        const auto beam_width = 256;
        auto rng = RandomNumberGenerator(42);

        nodes.resize(1);
        nodes[0].push_back(
            make_shared<Node>(initial_state, nullptr, Action({})));
        for (auto current_cost = 0; current_cost < 100000; current_cost++) {
            for (const auto& node : nodes[current_cost]) {
                if (node->state.score == 0) {
                    cerr << "Solved!" << endl;
                    return node;
                }
                for (const auto& action :
                     node->state.ComputeNextActionCandidates(target_cube)) {
                    auto new_state = node->state;
                    new_state.Apply(action, target_cube);
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
            nodes[current_cost].clear();
        }
        cerr << "Failed." << endl;
        return nullptr;
    }
};

void TestBeamSearch() {
    constexpr auto kOrder = 7;
    using Solver = BeamSearchSolver<kOrder, ColorType6>;
    using Cube = typename Solver::Cube;
    using State = typename Solver::State;

    auto solver = Solver();
    auto initial_cube = Cube();
    auto target_cube = Cube();
    const auto initial_state = State(initial_cube, target_cube);

    // ここで target_cube 渡すの、設計失敗してそう
    solver.Solve(initial_state, target_cube);

    // TODO: 結果を表示する
}

void TestCube() {
    constexpr auto kOrder = 7;

    auto cube = Cube<kOrder, ColorType6>();

    // 初期化
    for (auto face_id = (i8)0; face_id < 6; face_id++)
        for (auto y = 0; y < kOrder; y++)
            for (auto x = 0; x < kOrder; x++) {
                const auto color = face_id;
                // const auto color =
                //     face_id * 4 + (y >= kOrder / 2) * 2 + (x >= kOrder / 2);
                cube.Set(face_id, y, x, {color});
            }

    // 適当に動かす
    // const auto moves = {
    //     Move{Move::Direction::D, (i8)1},
    //     Move{Move::Direction::Fp, (i8)0},
    //     Move{Move::Direction::R, (i8)3},
    // };

    const auto moves = {
        Move{Move::Direction::F, (i8)2},  //
        Move{Move::Direction::R, (i8)0},  //
        Move{Move::Direction::Fp, (i8)2}, //
        Move{Move::Direction::D, (i8)6},  //
        Move{Move::Direction::R, (i8)0},  //
        Move{Move::Direction::Fp, (i8)4}, //
        Move{Move::Direction::D, (i8)6},  //
        Move{Move::Direction::F, (i8)4},  //
    };
    const auto formula = Formula(moves);
    formula.Print();
    cout << endl;

    cube.Display();
    for (const auto& mov : moves) {
        mov.Print();
        cout << endl;
        cube.Rotate(mov);
        cube.Display();
    }
}

int main() {
    // TestCube();
    TestBeamSearch();
}

// clang++ -std=c++20 -Wall -Wextra -O3 cube.cpp
// ./a.out

/*
メモ
* ハッシュ関数実装しかけたけど、重そうだし、
  実は重複除去しなくても割と大丈夫なんじゃないかという気がしたので一旦やめた
* RCube を真似して面の回転を実装したけど、
  order=8 くらいだと愚直の方が良かったりしそう
* ビームサーチではない
*/