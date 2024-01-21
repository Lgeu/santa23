#include "cube.cpp"

using std::fill;
using std::min;

using RainbowAction = Formula;

template <int order, typename ColorType = ColorType24>
struct RainbowCube : public Cube<order, ColorType> {
    static_assert(is_same_v<ColorType, ColorType24>);
    RainbowCube() : Cube<order, ColorType>() {}

    inline int ComputeScore(const RainbowCube& target) const {
        int score = 0;
        for (auto i = 0; i < 6; i++) {
            for (int y = 0; y < order; y++) {
                for (int x = 0; x < order; x++) {
                    if (this->Get(i, x, y) != target.Get(i, x, y))
                        score++;
                }
            }
        }
        return score;
    }

    inline void FromCube(const Cube<order, ColorType>& cube) {
        for (auto i = 0; i < 6; i++) {
            for (int y = 0; y < order; y++) {
                for (int x = 0; x < order; x++) {
                    this->Set(i, x, y, cube.Get(i, x, y));
                }
            }
        }
    }
};

template <int order> struct RainbowState {
    using Cube = ::Cube<order, ColorType24>;
    using RainbowCube = ::RainbowCube<order>;
    RainbowCube cube;
    int score;   // target との距離
    int n_moves; // これまでに回した回数

    inline RainbowState(const RainbowCube& cube, const RainbowCube& target_cube)
        : cube(cube), score(cube.ComputeScore(target_cube)), n_moves() {}

    // inplace に変更する
    inline void Apply(const RainbowAction& action,
                      const RainbowCube& target_cube) {
        cube.Rotate(action);
        score = cube.ComputeScore(target_cube);
        n_moves += action.Cost();
    }
};

template <int order> struct RainbowActionCandidateGenerator {
    using RainbowCube = ::RainbowCube<order>;
    using RainbowState = ::RainbowState<order>;
    vector<RainbowAction> actions;

    // ファイルから手筋を読み取る
    // ファイルには f1.d0.-r0.-f1 みたいなのが 1 行に 1 つ書かれている想定
    inline void FromFile(const string& filename, const bool is_normal) {
        const int OrderFormula = 4;
        vector<RainbowAction> actions_original;

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
            if (!is_normal && line[0] == '0')
                continue;
            actions_original.emplace_back(line.substr(0));
        }

        vector<Cube<OrderFormula, ColorType24>> cubes;

        for (auto& action_original : actions_original) {
            for (int x_rot = 0; x_rot < 4; x_rot++) {
                for (int y_rot = 0; y_rot < 4; y_rot++) {
                    for (int x_rot2 = 0; x_rot2 < 4; x_rot2++) {
                        for (int flag_inv = 0; flag_inv < 1; flag_inv++) {
                            RainbowAction action;
                            for (auto mov : action_original.moves) {
                                mov = mov.RotateX(OrderFormula, x_rot);
                                mov = mov.RotateY(OrderFormula, y_rot);
                                mov = mov.RotateX(OrderFormula, x_rot2);
                                action.moves.emplace_back(mov);
                            }

                            if (flag_inv)
                                action = action.Inv();

                            Cube<OrderFormula, ColorType24> cube;
                            cube.Reset();
                            // cube.Display(cerr);
                            // cerr << endl;
                            cube.Rotate(action);

                            bool flag_new = true;
                            for (auto& cube2 : cubes) {
                                if (cube.ComputeFaceDiff(cube2) == 0) {
                                    flag_new = false;
                                    break;
                                }
                            }
                            if (!flag_new)
                                continue;

                            // cube.Display(cerr);
                            // cerr << endl;
                            actions.emplace_back(action);
                            cubes.emplace_back(cube);
                        }
                    }
                }
            }
        }

        for (auto& action : actions) {
            for (auto& mov : action.moves) {
                if (mov.depth == OrderFormula - 1)
                    mov.depth = order - 1;
            }
        }

        // TODO: 重複があるかもしれないので確認した方が良い
    }

    inline const auto& Generate(const RainbowState&) const { return actions; }
};

template <int order> struct RainbowNode {
    using RainbowState = ::RainbowState<order>;
    RainbowState state;
    shared_ptr<RainbowNode> parent;
    RainbowAction last_action;
    inline RainbowNode(const RainbowState& state,
                       const shared_ptr<RainbowNode>& parent,
                       const RainbowAction& last_action)
        : state(state), parent(parent), last_action(last_action) {}
};

template <int order> struct RainbowBeamSearchSolver {
    using RainbowCube = ::RainbowCube<order>;
    using RainbowState = ::RainbowState<order>;
    using RainbowNode = ::RainbowNode<order>;
    using RainbowActionCandidateGenerator =
        ::RainbowActionCandidateGenerator<order>;

    RainbowCube target_cube;
    RainbowActionCandidateGenerator action_candidate_generator;
    int beam_width;
    vector<vector<shared_ptr<RainbowNode>>> nodes;

    inline RainbowBeamSearchSolver(const RainbowCube& target_cube,
                                   const bool is_normal, const int beam_width,
                                   const string& formula_file)
        : target_cube(target_cube), action_candidate_generator(),
          beam_width(beam_width), nodes() {
        action_candidate_generator.FromFile(formula_file, is_normal);
    }

    inline shared_ptr<RainbowNode> Solve(const RainbowCube& start_cube) {
        auto rng = RandomNumberGenerator(42);

        const auto start_state = RainbowState(start_cube, target_cube);
        const auto start_node = make_shared<RainbowNode>(
            start_state, nullptr, RainbowAction{vector<Move>()});
        nodes.clear();
        nodes.resize(1);
        nodes[0].push_back(start_node);

        auto minimum_scores = array<int, 16>();
        fill(minimum_scores.begin(), minimum_scores.end(), 9999);
        for (auto current_cost = 0; current_cost < 100000; current_cost++) {
            auto current_minimum_score = 9999;
            for (const auto& node : nodes[current_cost]) {
                if (!node)
                    continue;
                current_minimum_score =
                    min(current_minimum_score, node->state.score);
                if (node->state.score == 0) {
                    cerr << "Solved!" << endl;
                    return node;
                }
                for (const auto& action :
                     action_candidate_generator.Generate(node->state)) {
                    auto new_state = node->state;
                    new_state.Apply(action, target_cube);
                    if (new_state.n_moves >= (int)nodes.size())
                        nodes.resize(new_state.n_moves + 1);
                    if ((int)nodes[new_state.n_moves].size() < beam_width) {
                        nodes[new_state.n_moves].emplace_back(
                            new RainbowNode(new_state, node, action));
                    } else {
                        const auto idx = rng.Next() % beam_width;
                        if (new_state.score <
                            nodes[new_state.n_moves][idx]->state.score)
                            nodes[new_state.n_moves][idx].reset(
                                new RainbowNode(new_state, node, action));
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

[[maybe_unused]] static void TestRainbowActionCandidateGenerator() {
    constexpr auto kOrder = 4;
    const auto formula_file = "out/rainbow_formula_4.txt";

    using RainbowActionCandidateGenerator =
        RainbowActionCandidateGenerator<kOrder>;
    // using RainbowCube = typename
    // RainbowActionCandidateGenerator::RainbowCube;

    auto action_candidate_generator = RainbowActionCandidateGenerator();
    action_candidate_generator.FromFile(formula_file, true);

    // for (auto i : {0, 1, 2, 12, 13}) {
    //     // for (int i = 0; i <
    //     (int)action_candidate_generator.actions.size();
    //     // i++){
    //     const auto action = action_candidate_generator.actions[i];
    //     cout << format("# {}", i) << endl;
    //     action.Print();
    //     cout << endl;
    //     action.Display<RainbowCube>();
    //     cout << endl << endl;
    // }
    cout << "size: " << action_candidate_generator.actions.size() << endl;
}

template <int order>
static void SolveWithOrder(const int problem_id, const bool is_normal,
                           const Formula& sample_formula) {
    assert(!is_normal);
    constexpr auto beam_width = 128;
    const auto formula_file = "out/rainbow_formula_4.txt";

    // 面ソルバの出力を読み込む
    const auto solution_three_order_file =
        format("solution_three_order/{}_best.txt", problem_id);
    auto solution_three_order_string = string();
    auto ifs = ifstream(solution_three_order_file);
    if (!ifs.good()) {
        cerr << format("Cannot open file `{}`.", solution_three_order_file)
             << endl;
        abort();
    }
    {
        string tmp_string;
        int n_line = 0;
        while (getline(ifs, tmp_string)) {
            if (n_line % 2 == 0 && tmp_string != "") {
                if (solution_three_order_string != "")
                    solution_three_order_string += ".";
                solution_three_order_string += tmp_string;
            }
            n_line++;
        }
    }
    ifs.close();
    const auto solution_three_order = Formula(solution_three_order_string);

    const auto display_cube = [&sample_formula, &solution_three_order](
                                  const Formula& solution = Formula()) {
        auto cube = Cube<order, ColorType24>();
        cube.Reset();
        cube.RotateInv(sample_formula);
        cube.Rotate(solution_three_order);
        cube.Rotate(solution);
        cube.Display();
    };
    display_cube(); // 初期状態を描画する

    // 初期値の設定など
    const auto initial_cube = [&sample_formula, &solution_three_order] {
        auto initial_cube = RainbowCube<order, ColorType24>();
        initial_cube.Reset();
        initial_cube.RotateInv(sample_formula);
        initial_cube.Rotate(solution_three_order);
        return initial_cube;
    }();
    auto target_cube = RainbowCube<order>();
    target_cube.Reset();
    // 解く
    auto solver = RainbowBeamSearchSolver<order>(target_cube, is_normal,
                                                 beam_width, formula_file);
    const auto node = solver.Solve(initial_cube);
    if (node == nullptr) // 失敗
        return;

    // 結果を表示する
    vector<Move> result_moves;
    cout << node->state.n_moves << endl;
    for (auto p = node; p->parent != nullptr; p = p->parent)
        for (auto i = (int)p->last_action.moves.size() - 1; i >= 0; i--)
            result_moves.emplace_back(p->last_action.moves[i]);
    reverse(result_moves.begin(), result_moves.end());
    const auto solution = Formula(result_moves);
    solution.Print();
    cout << endl;
    display_cube(solution);

    Formula solution_all = solution_three_order;
    copy(result_moves.begin(), result_moves.end(),
         back_inserter(solution_all.moves));

    // 結果を保存する
    const auto all_solutions_file =
        format("solution_rainbow/{}_all.txt", problem_id);
    const auto best_solution_file =
        format("solution_rainbow/{}_best.txt", problem_id);
    auto ofs_all = ofstream(all_solutions_file, ios::app);
    if (ofs_all.good()) {
        solution_all.Print(ofs_all);
        ofs_all << endl
                << format("solution_score={} ", solution_all.Cost()) << endl;
        ofs_all.close();
    } else {
        cerr << format("Cannot open file `{}`.", all_solutions_file) << endl;
    }
    auto ifs_best = ifstream(best_solution_file);
    auto best_score = 99999;
    if (ifs_best.good()) {
        string line;
        getline(ifs_best, line); // skip formula
        getline(ifs_best, line); // read score
        best_score = stoi(line);
        ifs_best.close();
    }
    if (solution_all.Cost() < best_score) {
        auto ofs_best = ofstream(best_solution_file);
        if (ofs_best.good()) {
            solution_all.Print(ofs_best);
            ofs_best << endl << solution_all.Cost() << endl;
            ofs_best.close();
        } else
            cerr << format("Cannot open file `{}`.", best_solution_file)
                 << endl;
    }
}

[[maybe_unused]] static void Solve(const int problem_id) {
    const auto filename_puzzles = "../input/puzzles.csv";
    const auto filename_sample = "../input/sample_submission.csv";
    const auto [order, is_normal, sample_formula] =
        ReadKaggleInput(filename_puzzles, filename_sample, problem_id);
    switch (order) {
    case 4:
        SolveWithOrder<4>(problem_id, is_normal, sample_formula);
        break;
    case 5:
        SolveWithOrder<5>(problem_id, is_normal, sample_formula);
        break;
    case 6:
        SolveWithOrder<6>(problem_id, is_normal, sample_formula);
        break;
    case 7:
        SolveWithOrder<7>(problem_id, is_normal, sample_formula);
        break;
    case 8:
        SolveWithOrder<8>(problem_id, is_normal, sample_formula);
        break;
    case 9:
        SolveWithOrder<9>(problem_id, is_normal, sample_formula);
        break;
    case 10:
        SolveWithOrder<10>(problem_id, is_normal, sample_formula);
        break;
    case 19:
        SolveWithOrder<19>(problem_id, is_normal, sample_formula);
        break;
    case 33:
        SolveWithOrder<33>(problem_id, is_normal, sample_formula);
        break;
    default:
        assert(false);
    }
}

// clang-format off
// clang++ -std=c++20 -Wall -Wextra -O3 rainbow_cube.cpp -DTEST_RAINBOW_ACTION_CANDIDATE_GENERATOR
#ifdef TEST_RAINBOW_ACTION_CANDIDATE_GENERATOR
int main() { TestRainbowActionCandidateGenerator(); }
#endif
// clang-format on

// clang++ -std=c++20 -Wall -Wextra -O3 rainbow_cube.cpp -DSOLVE
#ifdef SOLVE
int main(const int argc, const char* const* const argv) {
    if (argc != 2) {
        cerr << format("Usage: {} <problem_id>", argv[0]) << endl;
        return 1;
    }
    const auto problem_id = atoi(argv[1]);
    Solve(problem_id);
}
#endif