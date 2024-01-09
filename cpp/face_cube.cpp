#include <algorithm>
#include <cmath>
#include <functional>
#include <mutex>
#include <thread>
#include <tuple>

#include "cube.cpp"

using std::fill;
using std::lock_guard;
using std::max;
using std::min;
using std::move;
using std::mutex;
using std::pow;
using std::reference_wrapper;
using std::reverse;
using std::thread;
using std::tuple;
using std::unique_lock;

std::mutex mtx;

constexpr int Order = 33;
constexpr int OrderFormula = 7;
const auto formula_file = "out/face_formula_7_8.txt";
constexpr bool flag_parallel = true;
// メモリ削減のため面の情報は落とす
using SliceMap = array<int, Order - 2>;
using SliceMapInv = array<vector<int>, OrderFormula - 2>;

// 面だけのキューブの 1 つの面
// TODO: 実装
template <int order, typename ColorType = ColorType6>
struct FaceCube : public Cube<order, ColorType> {
    static_assert(order == Order);
    FaceCube() : Cube<order, ColorType>() {}

    inline auto ComputeFaceScore(const FaceCube& target) const {
        auto score = 0;
        for (auto face_id = 0; face_id < 6; face_id++) {
            for (int y = 1; y < order - 1; y++) {
                for (int x = 1; x < order - 1; x++) {
                    int coef = 1;
                    if ((order % 2 == 1) && (x == order / 2) &&
                        (y == order / 2))
                        coef = 100;
                    score += coef * (this->Get(face_id, y, x) !=
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

template <int order_formula = OrderFormula, int order = Order>
FaceAction ConvertFaceActionMoveWithSliceMap(
    const FaceAction& face_action, const array<int, order - 2>& /* slice_map */,
    const array<vector<int>, order_formula - 2>& slice_map_inv) {
    auto new_face_action = FaceAction();
    for (auto mv : face_action.moves) {
        if (mv.depth == 0) {
            new_face_action.moves.emplace_back(mv);
            continue;
        } else if (mv.depth == order_formula - 1) {
            mv.depth = order - 1;
            new_face_action.moves.emplace_back(mv);
            continue;
        } else {
            assert(1 <= mv.depth && mv.depth <= order_formula - 2);
            assert(slice_map_inv[mv.depth - 1].size() >= 1);
            int depth_original = mv.depth;
            for (auto& depth : slice_map_inv[depth_original - 1]) {
                mv.depth = depth + 1;
                new_face_action.moves.emplace_back(mv);
            }
        }
    }
    return new_face_action;
}

template <int order_formula = OrderFormula, int order = Order>
FaceAction ConvertFaceActionFaceletChangeWithSliceMap(
    const FaceAction& face_action, const array<int, order - 2>& slice_map,
    const array<vector<int>, order_formula - 2>& slice_map_inv) {
    assert(face_action.use_facelet_changes);
    auto new_face_action =
        ConvertFaceActionMoveWithSliceMap<order_formula, order>(
            face_action, slice_map, slice_map_inv);
    new_face_action.use_facelet_changes = true;
    for (const auto& facelet_change : face_action.facelet_changes) {
        const auto& from_formula = facelet_change.from;
        const auto& to_formula = facelet_change.to;

        if (from_formula.x == 0 || from_formula.x == order_formula - 1 ||
            from_formula.y == 0 || from_formula.y == order_formula - 1) {
            // continue;
            assert(false);
        }

        for (const auto& from_x : slice_map_inv[from_formula.x - 1]) {
            for (const auto& from_y : slice_map_inv[from_formula.y - 1]) {
                for (const auto& to_x : slice_map_inv[to_formula.x - 1]) {
                    for (const auto& to_y : slice_map_inv[to_formula.y - 1]) {
                        const auto from =
                            FaceletPosition{from_formula.face_id,
                                            i8(from_y + 1), i8(from_x + 1)};
                        const auto to = FaceletPosition{
                            to_formula.face_id, i8(to_y + 1), i8(to_x + 1)};
                        new_face_action.facelet_changes.emplace_back(
                            FaceAction::FaceletChange{from, to});
                    }
                }
            }
        }
    }
    return new_face_action;
}

template <int order> struct FaceState {
    using FaceCube = ::FaceCube<order>;
    FaceCube cube;
    int score;   // target との距離
    int n_moves; // これまでに回した回数

    inline FaceState(const FaceCube& cube, const FaceCube& target_cube)
        : cube(cube), score(cube.ComputeFaceScore(target_cube)), n_moves() {}

    inline void Apply(const FaceAction& action, const FaceCube& target_cube) {
        cube.Rotate(action);
        score = cube.ComputeFaceScore(target_cube);
        n_moves += action.Cost();
    }

    // inplace に変更する
    inline void Apply(const FaceAction& action, const FaceCube& target_cube,
                      const SliceMap& slice_map,
                      const SliceMapInv& slice_map_inv) {
        auto action_new =
            ConvertFaceActionMoveWithSliceMap(action, slice_map, slice_map_inv);
        cube.Rotate(action_new);
        score = cube.ComputeFaceScore(target_cube);
        n_moves += action.Cost();
    }

    // 元から
    inline int ScoreWhenApplied(const FaceAction& action,
                                const FaceCube& target_cube) {

        assert(action.use_facelet_changes);
        int score_when_applied = score;
        for (const auto& facelet_change : action.facelet_changes) {
            const auto& from = facelet_change.from;
            const auto& to = facelet_change.to;

            if (from.x == 0 || from.x == order - 1 || from.y == 0 ||
                from.y == order - 1) {
                // continue;
                assert(false);
            }

            const auto color_from_target = target_cube.Get(from);
            const auto color_to_target = target_cube.Get(to);

            if (color_from_target == color_to_target)
                continue;

            const auto color_from = cube.Get(from);

            int coef = 1;
            if (((order & 1) == 1) && (from.x * 2 + 1 == order) &&
                (from.y * 2 + 1 == order))
                coef = 100;

            score_when_applied += (int(color_from == color_from_target) -
                                   int(color_from == color_to_target)) *
                                  coef;
        }
        return score_when_applied;
    }

    // action, target_cube から score を計算する
    inline int ScoreWhenApplied(const FaceAction& action,
                                const FaceCube& target_cube,
                                const SliceMap& slice_map,
                                const SliceMapInv& slice_map_inv) {
        int score_when_applied;

        if (action.use_facelet_changes) {

            // for (int i = 0; i < Order - 2; i++) {
            //     cerr << slice_map[i] << " ";
            // }
            // cerr << endl;
            // action.Print(cerr);

            score_when_applied = score;
            for (const auto& facelet_change : action.facelet_changes) {
                const auto& from_formula = facelet_change.from;
                const auto& to_formula = facelet_change.to;

                if (from_formula.x == 0 || from_formula.x == OrderFormula - 1 ||
                    from_formula.y == 0 || from_formula.y == OrderFormula - 1) {
                    // continue;
                    assert(false);
                }

                // for(const auto& from_x : slice_map_inv[from_formula.x - 1]) {
                //     for(const auto& from_y : slice_map_inv[from_formula.y -
                //     1]) {
                //         for(const auto& to_x : slice_map_inv[to_formula.x -
                //         1]) {
                //             for(const auto& to_y : slice_map_inv[to_formula.y
                //             - 1]) {
                // cerr << endl;
                // cerr << (int)from_formula.x << " " << (int)from_formula.y <<
                // " "
                //      << (int)to_formula.x << " " << (int)to_formula.y <<
                //      endl;
                for (const auto& from_x : slice_map_inv[from_formula.x - 1]) {
                    for (const auto& from_y :
                         slice_map_inv[from_formula.y - 1]) {
                        for (const auto& to_x :
                             slice_map_inv[to_formula.x - 1]) {
                            for (const auto& to_y :
                                 slice_map_inv[to_formula.y - 1]) {
                                // cerr << from_x + 1 << " " << from_y + 1 << "
                                // "
                                //      << to_x + 1 << " " << to_y + 1 << " : "
                                //      << (int)from_formula.x << " "
                                //      << (int)from_formula.y << " "
                                //      << (int)to_formula.x << " "
                                //      << (int)to_formula.y << endl;
                                const auto from = FaceletPosition{
                                    from_formula.face_id, i8(from_y + 1),
                                    i8(from_x + 1)};
                                const auto to =
                                    FaceletPosition{to_formula.face_id,
                                                    i8(to_y + 1), i8(to_x + 1)};
                                const auto color_from_target =
                                    target_cube.Get(from);
                                const auto color_to_target =
                                    target_cube.Get(to);
                                if (color_from_target == color_to_target)
                                    continue;
                                const auto color_from = cube.Get(from);

                                int coef = 1;
                                if (((Order & 1) == 1) &&
                                    (from.x * 2 + 1 == Order) &&
                                    (from.y * 2 + 1 == Order))
                                    coef = 100;

                                score_when_applied +=
                                    (int(color_from == color_from_target) -
                                     int(color_from == color_to_target)) *
                                    coef;
                            }
                        }
                    }
                }

                // const auto color_from_target = target_cube.Get(from);
                // const auto color_to_target = target_cube.Get(to);

                // if (color_from_target == color_to_target)
                //     continue;

                // const auto color_from = cube.Get(from);

                // int coef = 1;
                // if (((order & 1) == 1) && (from.x * 2 + 1 == order) &&
                //     (from.y * 2 + 1 == order))
                //     coef = 100;

                // score_when_applied += (int(color_from == color_from_target) -
                //                        int(color_from == color_to_target)) *
                //                       coef;
            }
        } else {
            auto action_new = ConvertFaceActionMoveWithSliceMap(
                action, slice_map, slice_map_inv);
            cube.Rotate(action_new);
            score_when_applied = cube.ComputeFaceScore(target_cube);
            cube.RotateInv(action_new);
        }
        return score_when_applied;
    }
};

// yield を使って EdgeAction を生成する？
template <int order> struct FaceActionCandidateGenerator {
    static_assert(order == Order);
    using FaceCube = ::FaceCube<order>;
    using FaceState = ::FaceState<order>;
    vector<SliceMap> slice_maps;        // array?
    vector<SliceMapInv> slice_maps_inv; // array?
    // vector<tuple<FaceAction, SliceMap&, SliceMapInv&>> actions;
    // vector<tuple<FaceAction, reference_wrapper<SliceMap>,
    //              reference_wrapper<SliceMapInv>>>
    //     actions;
    vector<tuple<FaceAction, FaceAction, SliceMap, SliceMapInv, bool>>
        actions; // TODO 参照
                 // bool
                 // はcubeサイズを大きくした時に変更箇所数が変わらなければtrue

    void DfsSliceMaps(vector<int> used, SliceMap& slice_map,
                      SliceMapInv& slice_map_inv, int depth, int max_depth) {
        if (depth >= 0) {
            // 追加
            slice_maps.push_back(slice_map);
            slice_maps_inv.push_back(slice_map_inv);
        }
        if (depth == max_depth) {
            return;
        }
        for (int i = 0; i < order - 2; i++) {
            // for (int i = 0; i < order / 2 - 1; i++) {
            if (used[i])
                continue;
            if ((Order % 2 == 1) && (i == order / 2 - 1))
                continue;
            // ペアで更新
            slice_map[i] = depth;
            slice_map_inv[depth].emplace_back(i);
            slice_map[order - 3 - i] = OrderFormula - 3 - depth;
            slice_map_inv[OrderFormula - 3 - depth].emplace_back(order - 3 - i);
            used[i] = 1;
            used[order - 3 - i] = 1;
            DfsSliceMaps(used, slice_map, slice_map_inv, depth + 1, max_depth);
            slice_map[i] = -1;
            slice_map_inv[depth].pop_back();
            used[i] = 0;
            slice_map[order - 3 - i] = -1;
            slice_map_inv[OrderFormula - 3 - depth].pop_back();
            used[order - 3 - i] = 0;
        }
    }

    FaceActionCandidateGenerator() : slice_maps(), slice_maps_inv(), actions() {
        // slice_maps と slice_maps_inv を生成する
        vector<int> used(order - 2, 0);
        SliceMap slice_map;
        SliceMapInv slice_map_inv;
        fill(slice_map.begin(), slice_map.end(), -1);
        DfsSliceMaps(used, slice_map, slice_map_inv, 0, OrderFormula / 2 - 1);
        if constexpr (order % 2 == 1) {
            used[order / 2 - 1] = 1;
            slice_map[order / 2 - 1] = OrderFormula / 2 - 1;
            slice_map_inv[OrderFormula / 2 - 1].emplace_back(order / 2 - 1);
            DfsSliceMaps(used, slice_map, slice_map_inv, 0,
                         OrderFormula / 2 - 1);
        }
    }

    // ファイルから手筋を読み取る
    // ファイルには f1.d0.-r0.-f1 みたいなのが 1 行に 1 つ書かれている想定
    inline void FromFile(const string& filename) {
        actions.clear();

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
            FaceAction faceaction_formula(line);
            faceaction_formula.EnableFaceletChangesWithNoSameRaw<
                Cube<OrderFormula, ColorType24>>();
            faceaction_formula.DisableFaceletChangeEdgeCorner<
                Cube<OrderFormula, ColorType24>>();
            faceaction_formula
                .DisableFaceletChangeSameFace<Cube<OrderFormula, ColorType6>>();
            // if (faceaction_formula.facelet_changes.size() > 6) {
            //     continue;
            //     // if (faceaction_formula.moves.size() == 1 &&
            //     //     (faceaction_formula.moves[0].depth == 0 ||
            //     //      faceaction_formula.moves[0].depth == OrderFormula -
            //     1)) {
            //     //     // 回転
            //     // } else {
            //     //     // 緊急回避
            //     //     // TODO FIX
            //     //     continue;
            //     // }
            // }
            vector<int> vec_use_slices(OrderFormula - 2, 0);
            for (Move& mv : faceaction_formula.moves) {
                if (1 <= mv.depth && mv.depth <= OrderFormula - 2) {
                    vec_use_slices[mv.depth - 1] = 1;
                    vec_use_slices[OrderFormula - 2 - mv.depth] = 1;
                }
            }

            bool flag_scale = true;
            {
                array<int, OrderFormula> slice_map;
                array<vector<int>, OrderFormula - 2> slice_map_inv;
                for (int i = 0; i < OrderFormula / 2 - 1; i++) {
                    slice_map[i] = i;
                    slice_map_inv[i].emplace_back(i);
                    slice_map[OrderFormula - 1 - i] = OrderFormula - 3 - i;
                    slice_map_inv[OrderFormula - 3 - i].emplace_back(
                        OrderFormula - 1 - i);
                }
                if (OrderFormula % 2 == 1) {
                    slice_map[OrderFormula / 2] = OrderFormula / 2 - 1;
                    slice_map_inv[OrderFormula / 2 - 1].emplace_back(
                        OrderFormula / 2);
                }
                FaceAction action_new =
                    ConvertFaceActionMoveWithSliceMap<OrderFormula,
                                                      OrderFormula + 2>(
                        faceaction_formula, slice_map, slice_map_inv);
                action_new.EnableFaceletChangesWithNoSameRaw<
                    Cube<OrderFormula + 2, ColorType24>>();
                action_new.DisableFaceletChangeEdgeCorner<
                    Cube<OrderFormula + 2, ColorType24>>();
                action_new.DisableFaceletChangeSameFace<
                    Cube<OrderFormula + 2, ColorType6>>();
                if (action_new.facelet_changes.size() !=
                    faceaction_formula.facelet_changes.size()) {
                    flag_scale = false;
                }
            }

            // int cnt = 0;
            for (int i = 0; i < (int)slice_maps.size(); i++) {
                bool flag_use = true;
                auto& slice_map = slice_maps[i];
                auto& slice_map_inv = slice_maps_inv[i];
                for (int j = 0; j < OrderFormula - 2; j++) {
                    if (vec_use_slices[j] != (slice_map_inv[j].size() > 0)) {
                        flag_use = false;
                        break;
                    }
                }
                if (!flag_use)
                    continue;
                {
                    auto slice_map = slice_maps[i];
                    auto slice_map_inv = slice_maps_inv[i];
                    if (Order % 2 == 1) {
                        if (slice_map[Order / 2 - 1] == -1) {
                            slice_map[Order / 2 - 1] = OrderFormula / 2 - 1;
                            slice_map_inv[OrderFormula / 2 - 1].emplace_back(
                                Order / 2 - 1);
                        }
                    }

                    if (flag_scale) {
                        FaceAction faceaction =
                            ConvertFaceActionFaceletChangeWithSliceMap<
                                OrderFormula, Order>(faceaction_formula,
                                                     slice_map, slice_map_inv);
                        actions.emplace_back(faceaction, faceaction_formula,
                                             slice_map, slice_map_inv, true);
                        // actions.emplace_back(faceaction_formula,
                        // std::ref(slice_map),
                        //                      std::ref(slice_map_inv));
                        // actions.push_back({faceaction_formula,
                        // std::ref(slice_map),
                        //                    std::ref(slice_map_inv)});

                        // cnt++;
                        // {
                        //     auto [action, slice_map, slice_map_inv] =
                        //         actions[actions.size() - 1];
                        //     cerr << line << endl;
                        //     for (int i = 0; i < Order - 2; i++) {
                        //         cerr << slice_map[i] << " ";
                        //     }
                        //     cerr << endl;
                        // }
                    } else {
                        // faceaction_formula action_new =
                        //     Convertfaceaction_formulaMoveWithSliceMap<OrderFormula,
                        //                                       Order>(
                        //         faceaction_formula, slice_map,
                        //         slice_map_inv);
                        // action_new.EnableFaceletChangesWithNoSameRaw<
                        //     Cube<Order, ColorType24>>();
                        // action_new.DisableFaceletChangeEdgeCorner<
                        //     Cube<Order, ColorType24>>();
                        // action_new.DisableFaceletChangeSameFace<
                        //     Cube<Order, ColorType6>>();

                        FaceAction faceaction =
                            ConvertFaceActionMoveWithSliceMap<OrderFormula,
                                                              Order>(
                                faceaction_formula, slice_map, slice_map_inv);
                        faceaction.EnableFaceletChangesWithNoSameRaw<
                            Cube<Order, ColorType24>>();
                        faceaction.DisableFaceletChangeEdgeCorner<
                            Cube<Order, ColorType24>>();
                        faceaction.DisableFaceletChangeSameFace<
                            Cube<Order, ColorType6>>();
                        actions.emplace_back(faceaction, faceaction_formula,
                                             slice_map, slice_map_inv, false);
                    }
                }
            }
            // cerr << cnt << endl;
            // cerr << endl;
        }

        // TODO: 重複があるかもしれないので確認した方が良い
    }

    inline const auto& Generate(const FaceState&) const { return actions; }
};

template <int order> struct FaceNode {
    using FaceState = ::FaceState<order>;
    FaceState state;
    shared_ptr<FaceNode> parent;
    FaceAction last_action, last_action_formula;
    SliceMap slice_map;
    SliceMapInv slice_map_inv;
    bool flag_last_action_scale;
    inline FaceNode(const FaceState& state, const shared_ptr<FaceNode>& parent,
                    const FaceAction& last_action,
                    const FaceAction& last_action_formula)
        : state(state), parent(parent), last_action(last_action),
          last_action_formula(last_action_formula), slice_map(),
          slice_map_inv(), flag_last_action_scale(false) {}
    inline FaceNode(const FaceState& state, const shared_ptr<FaceNode>& parent,
                    const FaceAction& last_action,
                    const FaceAction& last_action_formula,
                    const SliceMap& slice_map, const SliceMapInv& slice_map_inv,
                    bool flag_last_action_scale)
        : state(state), parent(parent), last_action(last_action),
          last_action_formula(last_action_formula), slice_map(slice_map),
          slice_map_inv(slice_map_inv),
          flag_last_action_scale(flag_last_action_scale) {}
};

template <int order> struct FaceBeamSearchSolver {
    static_assert(order == Order);
    using FaceCube = ::FaceCube<order>;
    using FaceState = ::FaceState<order>;
    using FaceNode = ::FaceNode<order>;
    using FaceActionCandidateGenerator = ::FaceActionCandidateGenerator<order>;

    FaceCube target_cube;
    FaceActionCandidateGenerator action_candidate_generator;
    int beam_width;
    int n_threads;
    vector<vector<shared_ptr<FaceNode>>> nodes;

    inline FaceBeamSearchSolver(const FaceCube& target_cube,
                                const int beam_width,
                                const string& formula_file,
                                const int n_threads = 1)
        : target_cube(target_cube), action_candidate_generator(),
          beam_width(beam_width), n_threads(n_threads), nodes() {
        action_candidate_generator.FromFile(formula_file);
    }

    inline shared_ptr<FaceNode> Solve(const FaceCube& start_cube) {
        auto rng = RandomNumberGenerator(42);

        const auto start_state = FaceState(start_cube, target_cube);
        const auto start_node = make_shared<FaceNode>(
            start_state, nullptr, FaceAction{vector<Move>()},
            FaceAction{vector<Move>()});

        if (n_threads == 1) {
            nodes.resize(1);
            nodes[0].push_back(start_node);
        } else {
            nodes.resize(100000,
                         vector<shared_ptr<FaceNode>>(beam_width, start_node));
            nodes[0][0] = start_node;
        }

        cout << format("total actions={}",
                       action_candidate_generator.actions.size())
             << endl;

        assert(n_threads == 1);

        assert(n_threads >= 1);
        vector<thread> threads;

        // int max_action_cost = 0;
        // if (n_threads >= 2) {
        //     for (const auto& action : action_candidate_generator.actions) {
        //         max_action_cost = max(max_action_cost, action.Cost());
        //     }
        // }

        for (auto current_cost = 0; current_cost < 100000; current_cost++) {
            auto current_minimum_score = 9999;
            cout << format("current_cost={} nodes={}", current_cost,
                           nodes[current_cost].size())
                 << endl;
            if (nodes[current_cost].empty()) {
                continue;
            }
            // nodes[current_cost][0]->state.cube.Display(cerr);
            for (const auto& node : nodes[current_cost]) {
                if (!node)
                    continue;
                current_minimum_score =
                    min(current_minimum_score, node->state.score);
                if (node->state.score == 0) {
                    cerr << "Solved!" << endl;
                    return node;
                }

                if (n_threads == 1) {
                    for (const auto& [action, action_formula, slice_map,
                                      slice_map_inv, flag_last_action_scale] :
                         action_candidate_generator.Generate(node->state)) {
                        int new_n_moves = node->state.n_moves + action.Cost();
                        if (new_n_moves >= (int)nodes.size())
                            nodes.resize(new_n_moves + 1);
                        if ((int)nodes[new_n_moves].size() < beam_width) {
                            auto new_state = node->state;
                            // new_state.Apply(action, target_cube, slice_map,
                            //                 slice_map_inv);
                            new_state.Apply(action, target_cube);
                            nodes[new_state.n_moves].emplace_back(new FaceNode(
                                new_state, node, action, action_formula,
                                slice_map, slice_map_inv,
                                flag_last_action_scale));
                        } else {
                            if (false) {
                                auto new_state = node->state;
                                new_state.Apply(action, target_cube);
                                int new_score_slow = new_state.score;
                                // const auto idx = rng.Next() % beam_width;
                                // if (new_score_slow <
                                //     nodes[new_n_moves][idx]->state.score) {
                                //     nodes[new_n_moves][idx].reset(
                                //         new FaceNode(new_state, node, action,
                                //                      slice_map,
                                //                      slice_map_inv));
                                // }

                                FaceAction action_new =
                                    ConvertFaceActionMoveWithSliceMap<
                                        OrderFormula, Order>(action, slice_map,
                                                             slice_map_inv);

                                action_new.Print(cerr);
                                cerr << endl;
                                action_new.EnableFaceletChangesWithNoSameRaw<
                                    Cube<Order, ColorType24>>();
                                action_new.DisableFaceletChangeEdgeCorner<
                                    Cube<Order, ColorType24>>();
                                action_new.DisableFaceletChangeSameFace<
                                    Cube<Order, ColorType6>>();
                                for (auto& facelet_change :
                                     action_new.facelet_changes) {
                                    cerr << (int)facelet_change.from.face_id
                                         << " " << (int)facelet_change.from.y
                                         << " " << (int)facelet_change.from.x
                                         << " : "
                                         << (int)facelet_change.to.face_id
                                         << " " << (int)facelet_change.to.y
                                         << " " << (int)facelet_change.to.x
                                         << endl;
                                }
                                action.Print(cerr);
                                cerr << endl;

                                FaceAction action_tmp = action;
                                action_tmp.EnableFaceletChangesWithNoSameRaw<
                                    Cube<OrderFormula, ColorType24>>();
                                action_tmp.DisableFaceletChangeEdgeCorner<
                                    Cube<OrderFormula, ColorType24>>();
                                action_tmp.DisableFaceletChangeSameFace<
                                    Cube<OrderFormula, ColorType6>>();
                                for (auto& facelet_change :
                                     action_tmp.facelet_changes) {
                                    cerr << (int)facelet_change.from.face_id
                                         << " " << (int)facelet_change.from.y
                                         << " " << (int)facelet_change.from.x
                                         << " : "
                                         << (int)facelet_change.to.face_id
                                         << " " << (int)facelet_change.to.y
                                         << " " << (int)facelet_change.to.x
                                         << endl;
                                }
                                cerr << endl;
                                int new_score = node->state.ScoreWhenApplied(
                                    action, target_cube, slice_map,
                                    slice_map_inv);
                                cerr << new_score << " " << new_score_slow
                                     << endl;
                                cerr << endl;

                                assert(new_score == new_score_slow);
                            }

                            // int new_score = node->state.ScoreWhenApplied(
                            //     action, target_cube, slice_map,
                            //     slice_map_inv);
                            int new_score = node->state.ScoreWhenApplied(
                                action, target_cube);

                            const auto idx = rng.Next() % beam_width;
                            if (new_score <
                                nodes[new_n_moves][idx]->state.score) {
                                auto new_state = node->state;
                                // new_state.Apply(action, target_cube,
                                // slice_map,
                                //                 slice_map_inv);
                                new_state.Apply(action, target_cube);
                                nodes[new_n_moves][idx].reset(new FaceNode(
                                    new_state, node, action, action_formula,
                                    slice_map, slice_map_inv,
                                    flag_last_action_scale));
                            }
                        }
                    }

                    // TODO 並列化
                    if constexpr (flag_parallel)
                        if (node->parent) {
                            // action を全探索
                            // if (node->flag_last_action_scale) {
                            // any action is ok when actually rotating
                            if (true) {
                                // if last action can be modified with
                                // ConvertFaceActionFaceletChangeWithSliceMap
                                SliceMap slice_map_new = node->slice_map;
                                SliceMapInv slice_map_inv_new =
                                    node->slice_map_inv;

                                // list up used slices in formula
                                vector<int> vec_use_slices(OrderFormula - 2, 0);
                                for (Move& mv :
                                     node->last_action_formula.moves) {
                                    if (1 <= mv.depth &&
                                        mv.depth <= OrderFormula - 2) {
                                        vec_use_slices[mv.depth - 1] = 1;
                                        vec_use_slices[OrderFormula - 2 -
                                                       mv.depth] = 1;
                                    }
                                }

                                // cerr << "original slice map" << endl;
                                // for (int i = 0; i < Order - 2; i++) {
                                //     cerr << slice_map_new[i] << " ";
                                // }
                                // cerr << endl;
                                // for (int i = 0; i < OrderFormula - 2; i++) {
                                //     cerr << slice_map_inv_new[i].size() << "
                                //     ";
                                // }
                                // cerr << endl;

                                for (int slice_idx = 0; slice_idx < Order - 2;
                                     slice_idx++) {
                                    if (slice_map_new[slice_idx] != -1) {
                                        continue;
                                    }
                                    if constexpr (Order % 2 == 1) {
                                        if (slice_idx == Order / 2 - 1) {
                                            continue;
                                        }
                                    }
                                    for (int slice_idx_formula = 0;
                                         slice_idx_formula < OrderFormula - 2;
                                         slice_idx_formula++) {
                                        if constexpr (OrderFormula % 2 == 1) {
                                            if (slice_idx_formula ==
                                                OrderFormula / 2 - 1) {
                                                continue;
                                            }
                                        }
                                        if (!vec_use_slices
                                                [slice_idx_formula]) {
                                            continue;
                                        }

                                        // try new slice
                                        slice_map_new[slice_idx] =
                                            slice_idx_formula;
                                        slice_map_inv_new[slice_idx_formula]
                                            .emplace_back(slice_idx);
                                        slice_map_new[Order - 3 - slice_idx] =
                                            OrderFormula - 3 -
                                            slice_idx_formula;
                                        slice_map_inv_new[OrderFormula - 3 -
                                                          slice_idx_formula]
                                            .emplace_back(Order - 3 -
                                                          slice_idx);
                                        // FaceAction action_new =
                                        //     ConvertFaceActionFaceletChangeWithSliceMap<
                                        //         OrderFormula, Order>(
                                        //         node->last_action_formula,
                                        //         slice_map_new,
                                        //         slice_map_inv_new);
                                        FaceAction action_new =
                                            ConvertFaceActionMoveWithSliceMap<
                                                OrderFormula, Order>(
                                                node->last_action_formula,
                                                slice_map_new,
                                                slice_map_inv_new);

                                        auto new_state = node->parent->state;
                                        new_state.Apply(action_new,
                                                        target_cube);
                                        const auto idx =
                                            rng.Next() % beam_width;
                                        if (new_state.score <
                                            nodes[new_state.n_moves][idx]
                                                ->state.score) {
                                            nodes[new_state.n_moves][idx].reset(
                                                new FaceNode(
                                                    new_state, node->parent,
                                                    action_new,
                                                    node->last_action_formula,
                                                    slice_map_new,
                                                    slice_map_inv_new,
                                                    node->flag_last_action_scale));
                                        }

                                        // int new_score =
                                        //     node->parent->state.ScoreWhenApplied(
                                        //         action_new, target_cube);
                                        // int new_n_moves =
                                        //     node->parent->state.n_moves +
                                        //     action_new.Cost();

                                        // cerr << current_cost << " " <<
                                        // new_n_moves
                                        //      << endl;

                                        // const auto idx = rng.Next() %
                                        // beam_width;
                                        // // if (new_score <
                                        // //
                                        // nodes[new_n_moves][idx]->state.score)
                                        // //     {
                                        // if (true) {
                                        //     auto new_state =
                                        //     node->parent->state;
                                        //     new_state.Apply(action_new,
                                        //                     target_cube);
                                        //     cerr << slice_idx << " "
                                        //          << slice_idx_formula <<
                                        //          endl;
                                        //     for (int i = 0; i < Order - 2;
                                        //     i++) {
                                        //         cerr << slice_map_new[i] << "
                                        //         ";
                                        //     }
                                        //     cerr << endl;
                                        //     for (int i = 0; i < OrderFormula
                                        //     - 2;
                                        //          i++) {
                                        //         cerr <<
                                        //         slice_map_inv_new[i].size()
                                        //              << " ";
                                        //     }
                                        //     cerr << endl;
                                        //     cerr << new_state.score << " "
                                        //          << new_score << endl;
                                        //     assert(new_state.score ==
                                        //     new_score);
                                        //     // nodes[new_n_moves][idx].reset(
                                        //     //     new FaceNode(
                                        //     //         new_state, node,
                                        //     action_new,
                                        //     // node->last_action_formula,
                                        //     //         slice_map_new,
                                        //     //         slice_map_inv_new,
                                        //     true));
                                        // }

                                        slice_map_new[slice_idx] = -1;
                                        slice_map_inv_new[slice_idx_formula]
                                            .pop_back();
                                        slice_map_new[Order - 3 - slice_idx] =
                                            -1;
                                        slice_map_inv_new[OrderFormula - 3 -
                                                          slice_idx_formula]
                                            .pop_back();
                                    }
                                }
                            }
                        }

                } else {
                    assert(false);
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

#ifdef TEST_FACE_CUBE
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
#endif

#ifdef TEST_FROM_CUBE
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
#endif

#ifdef TEST_READ_NNN
[[maybe_unused]] static void TestReadNNN() {
    const auto filename = "in/input_example_1.nnn";
    auto cube = Cube<7, ColorType6>();
    cube.ReadNNN(filename);
    cube.Display();
    auto face_cube = FaceCube<7>();
    face_cube.FromCube(cube);
    face_cube.Display();
}
#endif

#ifdef GENERATE_TEST_NNN
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
#endif

#ifdef TEST_FACE_ACTION_CANDIDATE_GENERATOR
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
#endif

[[maybe_unused]] static void TestFaceBeamSearch() {
    // constexpr auto kOrder = 9;
    // const auto formula_file = "out/face_formula_9_7.txt";
    // constexpr auto kOrder = 9;
    constexpr int kOrder = Order;
    static_assert(kOrder == Order);
    const auto beam_width = 1;

    constexpr int n_threads = 1;

    cout << format("kOrder={} formula_file={} beam_width={} n_threads={}",
                   kOrder, formula_file, beam_width, n_threads)
         << endl;

    using Solver = FaceBeamSearchSolver<kOrder>;
    using FaceCube = typename Solver::FaceCube;

    auto initial_cube = FaceCube();
    if (true) {
        // ランダムな initial_cube を用意する
        // 解けたり解けなかったりする
        initial_cube.Reset();
        auto rng = RandomNumberGenerator(42);
        for (auto i = 0; i < 10000; i++) {
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

    initial_cube.Display(cout);

    auto target_cube = FaceCube();
    target_cube.Reset();

    auto solver = Solver(target_cube, beam_width, formula_file, n_threads);

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
        initial_cube.Display(cout);
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