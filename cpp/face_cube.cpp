#include <algorithm>

#include "cube.cpp"

using std::min;
using std::reverse;

// 面だけのキューブの 1 つの面
// TODO: 実装
template <int order, typename ColorType = ColorType6>
struct FaceCube : public Cube<order, ColorType> {
    FaceCube() : Cube<order, ColorType>() {}

    inline auto ComputeFaceScore(const FaceCube& target) const {
        auto score = 0;
        for (auto face_id = 0; face_id < 6; face_id++) {
            for (int y = 1; y < order - 1; y++) {
                for (int x = 1; x < order - 1; x++) {
                    score +=
                        this->Get(face_id, y, x) != target.Get(face_id, y, x);
                    if ((order % 2 == 1) && (x == order / 2) &&
                        (y == order / 2))
                        score += 100 * (this->Get(face_id, y, x) !=
                                        target.Get(face_id, y, x));
                }
            }
        }
        return score;
    }

    inline void FromCube(const Cube<order, ColorType6>& rhs) {
        for (auto face_id = 0; face_id < 6; face_id++) {
            // for (int y = 1; y < order - 1; y++) {
            //     for (int x = 1; x < order - 1; x++) {
            for (int y = 0; y < order; y++) {
                for (int x = 0; x < order; x++) {
                    this->Set(face_id, y, x, rhs.Get(face_id, y, x));
                }
            }
        }
    }
};

using FaceAction = Formula;

template <int order> struct FaceState {
    using FaceCube = ::FaceCube<order>;
    FaceCube cube;
    int score;   // target との距離
    int n_moves; // これまでに回した回数

    inline FaceState(const FaceCube& cube, const FaceCube& target_cube)
        : cube(cube), score(cube.ComputeFaceScore(target_cube)), n_moves() {}

    // inplace に変更する
    inline void Apply(const FaceAction& action, const FaceCube& target_cube) {
        cube.Rotate(action);
        score = cube.ComputeFaceScore(target_cube);
        n_moves += action.Cost();
    }

    // action, target_cube から score を計算する
    inline int ScoreWhenApplied(const FaceAction& action,
                                const FaceCube& target_cube) {
        int score_when_applied;

        if (action.use_facelet_changes) {
            score_when_applied = score;
            int dscore = 0;
            for (const auto& facelet_change : action.facelet_changes) {
                auto from = facelet_change.from;
                auto to = facelet_change.to;

                if (from.x == 0 || from.x == order - 1 || from.y == 0 ||
                    from.y == order - 1) {
                    // continue;
                    assert(false);
                }

                auto color_from_target = target_cube.Get(from);
                auto color_to_target = target_cube.Get(to);
                auto color_from = cube.Get(from);

                int coef = 1;
                if ((order % 2 == 1) && (from.x == order / 2) &&
                    (from.y == order / 2))
                    coef = 100;

                dscore += (color_from == color_from_target) * coef;
                dscore -= (color_from == color_to_target) * coef;
            }
            score_when_applied += dscore;
        } else {
            cube.Rotate(action);
            score_when_applied = cube.ComputeFaceScore(target_cube);
            cube.RotateInv(action);
        }
        return score_when_applied;
    }
};

// yield を使って EdgeAction を生成する？
template <int order> struct FaceActionCandidateGenerator {
    using FaceCube = ::FaceCube<order>;
    using FaceState = ::FaceState<order>;
    vector<FaceAction> actions;

    // ファイルから手筋を読み取る
    // ファイルには f1.d0.-r0.-f1 みたいなのが 1 行に 1 つ書かれている想定
    inline void FromFile(const string& filename) {
        actions.clear();

        // // 面の回転 1 つだけからなる手筋を別途加える
        // 全回転を追加
        for (auto i = 0; i < 6; i++) {
            for (int j = 0; j < order; j++) {
                actions.emplace_back(
                    vector<Move>{Move{(Move::Direction)i, (i8)j}});
                actions.back()
                    .EnableFaceletChangesWithNoSameRaw<
                        Cube<order, ColorType24>>();
                actions.back()
                    .DisableFaceletChangeEdgeCorner<Cube<order, ColorType24>>();
                actions.back()
                    .DisableFaceletChangeSameFace<Cube<order, ColorType6>>();
            }
        }

        // ファイルから読み取る
        auto ifs = ifstream(filename);
        if (!ifs.good()) {
            cerr << format("Cannot open file `{}`.", filename) << endl;
            abort();
        }
        string line;
        while (getline(ifs, line)) {
            if (line.empty() || line[0] == '#')
                continue;
            actions.emplace_back(line);
            actions.back()
                .EnableFaceletChangesWithNoSameRaw<Cube<order, ColorType24>>();
            actions.back()
                .DisableFaceletChangeEdgeCorner<Cube<order, ColorType24>>();
            actions.back()
                .DisableFaceletChangeSameFace<Cube<order, ColorType6>>();
        }

        // TODO: 重複があるかもしれないので確認した方が良い
    }

    inline const auto& Generate(const FaceState&) const { return actions; }
};

template <int order> struct FaceNode {
    using FaceState = ::FaceState<order>;
    FaceState state;
    shared_ptr<FaceNode> parent;
    FaceAction last_action;
    inline FaceNode(const FaceState& state, const shared_ptr<FaceNode>& parent,
                    const FaceAction& last_action)
        : state(state), parent(parent), last_action(last_action) {}
};

template <int order> struct FaceBeamSearchSolver {
    using FaceCube = ::FaceCube<order>;
    using FaceState = ::FaceState<order>;
    using FaceNode = ::FaceNode<order>;
    using FaceActionCandidateGenerator = ::FaceActionCandidateGenerator<order>;

    FaceCube target_cube;
    FaceActionCandidateGenerator action_candidate_generator;
    int beam_width;
    vector<vector<shared_ptr<FaceNode>>> nodes;

    inline FaceBeamSearchSolver(const FaceCube& target_cube,
                                const int beam_width,
                                const string& formula_file)
        : target_cube(target_cube), action_candidate_generator(),
          beam_width(beam_width), nodes() {
        action_candidate_generator.FromFile(formula_file);
    }

    inline shared_ptr<FaceNode> Solve(const FaceCube& start_cube) {
        auto rng = RandomNumberGenerator(42);

        const auto start_state = FaceState(start_cube, target_cube);
        const auto start_node = make_shared<FaceNode>(
            start_state, nullptr, FaceAction{vector<Move>()});
        nodes.resize(1);
        nodes[0].push_back(start_node);

        for (auto current_cost = 0; current_cost < 100000; current_cost++) {
            auto current_minimum_score = 9999;
            nodes[current_cost][0]->state.cube.Display();
            for (const auto& node : nodes[current_cost]) {
                current_minimum_score =
                    min(current_minimum_score, node->state.score);
                if (node->state.score == 0) {
                    cerr << "Solved!" << endl;
                    return node;
                }
                for (const auto& action :
                     action_candidate_generator.Generate(node->state)) {

                    int new_n_moves = node->state.n_moves + action.Cost();
                    if (new_n_moves >= (int)nodes.size())
                        nodes.resize(new_n_moves + 1);
                    if ((int)nodes[new_n_moves].size() < beam_width) {
                        auto new_state = node->state;
                        new_state.Apply(action, target_cube);
                        nodes[new_state.n_moves].emplace_back(
                            new FaceNode(new_state, node, action));
                    } else {
                        int new_score =
                            node->state.ScoreWhenApplied(action, target_cube);
                        const auto idx = rng.Next() % beam_width;
                        if (new_score < nodes[new_n_moves][idx]->state.score) {
                            auto new_state = node->state;
                            new_state.Apply(action, target_cube);
                            nodes[new_state.n_moves][idx].reset(
                                new FaceNode(new_state, node, action));
                        }
                    }
                }
            }

            cout << format("current_cost={} current_minimum_score={}",
                           current_cost, current_minimum_score)
                 << endl;
            nodes[current_cost].clear();
        }
        cerr << "Failed." << endl;
        return nullptr;
    }
};

[[maybe_unused]] static void TestFaceCube() {
    constexpr auto kOrder = 7;
    const auto moves = {
        Move{Move::Direction::F, (i8)2},  //
        Move{Move::Direction::D, (i8)1},  //
        Move{Move::Direction::F, (i8)1},  //
        Move{Move::Direction::D, (i8)5},  //
        Move{Move::Direction::Fp, (i8)0}, //
        Move{Move::Direction::D, (i8)3},  //
    };

    const auto formula = Formula(moves);
    formula.Print();
    cout << endl;
    formula.Display<FaceCube<kOrder>>();
}

[[maybe_unused]] static void TestFromCube() {
    auto cube = Cube<5, ColorType6>();
    cube.Reset();
    auto reference_face_cube = FaceCube<5>();
    reference_face_cube.Reset();
    auto converted_face_cube = FaceCube<5>();

    // ランダムに回す
    auto rng = RandomNumberGenerator(42);
    for (auto i = 0; i < 100; i++) {
        const auto mov =
            Move{(Move::Direction)(rng.Next() % 6), (i8)(rng.Next() % 5)};
        cube.Rotate(mov);
        reference_face_cube.Rotate(mov);
    }

    cube.Display();
    reference_face_cube.Display();

    converted_face_cube.FromCube(cube);
    converted_face_cube.Display();
}

[[maybe_unused]] static void TestReadNNN() {
    const auto filename = "in/input_example_1.nnn";
    auto cube = Cube<7, ColorType6>();
    cube.ReadNNN(filename);
    cube.Display();
    auto face_cube = FaceCube<7>();
    face_cube.FromCube(cube);
    face_cube.Display();
}

[[maybe_unused]] static void GenerateTestNNN() {
    const auto filename = "facecube_test.nnn";
    auto cube = Cube<7, ColorType6>();
    cube.Reset();
    // ランダムに回す
    auto rng = RandomNumberGenerator(42);
    for (auto i = 0; i < 1000; i++) {
        const auto mov =
            Move{(Move::Direction)(rng.Next() % 6), (i8)(rng.Next() % 5)};
        cube.Rotate(mov);
    }
    cube.WriteNNN(filename);
    cube.Display();
    auto face_cube = FaceCube<7>();
    face_cube.ReadNNN(filename);
    face_cube.Display();
}

[[maybe_unused]] static void TestFaceActionCandidateGenerator() {
    constexpr auto kOrder = 5;
    const auto formula_file = "out/face_formula_5_4.txt";

    using FaceActionCandidateGenerator = FaceActionCandidateGenerator<kOrder>;
    using FaceCube = typename FaceActionCandidateGenerator::FaceCube;

    auto action_candidate_generator = FaceActionCandidateGenerator();
    action_candidate_generator.FromFile(formula_file);

    for (auto i : {0, 1, 2, 12, 13}) {
        const auto action = action_candidate_generator.actions[i];
        cout << format("# {}", i) << endl;
        action.Print();
        cout << endl;
        action.Display<FaceCube>();
        cout << endl << endl;
    }
}

[[maybe_unused]] static void TestFaceBeamSearch() {
    constexpr auto kOrder = 19;
    const auto formula_file = "out/face_formula_19_7.txt";
    const auto beam_width = 1;

    cerr << format("kOrder={} formula_file={} beam_width={}", kOrder,
                   formula_file, beam_width)
         << endl;

    using Solver = FaceBeamSearchSolver<kOrder>;
    using FaceCube = typename Solver::FaceCube;

    auto initial_cube = FaceCube();
    if (true) {
        // ランダムな initial_cube を用意する
        // 解けたり解けなかったりする
        initial_cube.Reset();
        auto rng = RandomNumberGenerator(42);
        for (auto i = 0; i < 1000; i++) {
            const auto mov = Move{(Move::Direction)(rng.Next() % 6),
                                  (i8)(rng.Next() % kOrder)};
            initial_cube.Rotate(mov);
        }
    } else {
        // const auto input_cube_file = "in/input_example_face_1.nnn";
        // auto cube = Cube<7, ColorType6>();
        // cube.ReadNNN(input_cube_file);
        // initial_cube.FromCube(cube);
    }

    initial_cube.Display();

    auto target_cube = FaceCube();
    target_cube.Reset();

    auto solver = Solver(target_cube, beam_width, formula_file);

    const auto node = solver.Solve(initial_cube);
    if (node != nullptr) {
        cout << node->state.n_moves << endl;
        auto moves = vector<Move>();
        for (auto p = node; p->parent != nullptr; p = p->parent)
            for (auto i = (int)p->last_action.moves.size() - 1; i >= 0; i--)
                moves.emplace_back(p->last_action.moves[i]);
        reverse(moves.begin(), moves.end());
        const auto solution = Formula(moves);
        solution.Print();
        cout << endl;
        initial_cube.Rotate(solution);
        initial_cube.Display();
    }
}

// clang++ -std=c++20 -Wall -Wextra -O3 face_cube.cpp -DTEST_FACE_CUBE
#ifdef TEST_FACE_CUBE
int main() { TestFaceCube(); }
#endif

// clang++ -std=c++20 -Wall -Wextra -O3 face_cube.cpp -DTEST_FROM_CUBE
#ifdef TEST_FROM_CUBE
int main() { TestFromCube(); }
#endif

// clang++ -std=c++20 -Wall -Wextra -O3 face_cube.cpp -DTEST_READ_NNN
#ifdef TEST_READ_NNN
int main() { TestReadNNN(); }
#endif

// clang++ -std=c++20 -Wall -Wextra -O3 face_cube.cpp -DGENERATE_TEST_NNN
#ifdef GENERATE_TEST_NNN
int main() { GenerateTestNNN(); }
#endif

// clang-format off
// clang++ -std=c++20 -Wall -Wextra -O3 face_cube.cpp
// -DTEST_face_ACTION_CANDIDATE_GENERATOR
#ifdef TEST_FACE_ACTION_CANDIDATE_GENERATOR
int main() { TestFaceActionCandidateGenerator(); }
#endif
// clang-format on

// clang++ -std=c++20 -Wall -Wextra -O3 face_cube.cpp -DTEST_FACE_BEAM_SEARCH
#ifdef TEST_FACE_BEAM_SEARCH
int main() { TestFaceBeamSearch(); }
#endif