#include <algorithm>
#include <cmath>
#include <functional>
#include <mutex>
#include <numeric>
#include <thread>
#include <tuple>

#include "cube.cpp"

using std::cin;
using std::fill;
using std::flush;
using std::iota;
using std::lock_guard;
using std::max;
using std::min;
using std::move;
using std::mutex;
using std::pow;
using std::reference_wrapper;
using std::reverse;
using std::swap;
using std::thread;
using std::tuple;

std::mutex mtx;

#ifndef ORDER
#define ORDER 7
#endif
#ifndef DEPTH
#define DEPTH 7
#endif
#ifndef N_THREADS
#define N_THREADS 16
#endif

constexpr int Order = ORDER;

#if ORDER == 4
constexpr int OrderFormula = 4;
#elif ORDER == 5
constexpr int OrderFormula = 5;
#elif ORDER % 2 == 0
constexpr int OrderFormula = 6;
#else
constexpr int OrderFormula = 7;
#endif

// const auto formula_file = "out/face_formula_7_7.txt";
const auto formula_file =
    format("out/face_formula_{}_{}.txt", OrderFormula, DEPTH);
constexpr bool flag_parallel = true;
// メモリ削減のため面の情報は落とす
using SliceMap = array<int, Order - 2>;
using SliceMapInv = array<vector<int>, OrderFormula - 2>;

#ifdef RAINBOW

constexpr int COEF_PARITY = 10;
constexpr int COEF_PARITY_CROSS = 10;

using ColorTypeChameleon = ColorType24;

inline int RainbowDist(const int c1, const int c2) {
    if (c1 == c2)
        return 0;
    // if (c1 / 4 == c2 / 4 && c1 % 4 == c2 % 4)
    // return 1;
    if ((c1 & 29) == (c2 & 29))
        return 2;
    // if ((c1 / 4 == c2 / 4) && (c1 % 2 == c2 % 2))
    //     return 2;
    else if (Cube<Order, ColorType6>::GetOppositeFaceId(c1 / 4) == c2 / 4)
        return 2;
    else
        return 1;
}

template <typename T> bool InversionParityInplace(vector<T>& V) {
    int tmp = 0;
    for (int i = 0; i < (int)V.size(); i++) {
        while (V[i] != i) {
            swap(V[i], V[V[i]]);
            tmp++;
        }
    }
    return tmp % 2 == 1;
}

template <typename T> bool InversionParity(vector<T> V) {
    int tmp = 0;
    for (int i = 0; i < (int)V.size(); i++) {
        while (V[i] != i) {
            swap(V[i], V[V[i]]);
            tmp++;
        }
    }
    return tmp % 2 == 1;
}

#else
using ColorTypeChameleon = ColorType6;
#endif

// 面だけのキューブの 1 つの面
// TODO: 実装
template <int order, typename ColorType = ColorType6>
struct FaceCube : public Cube<order, ColorType> {
    static_assert(order == Order);
    FaceCube() : Cube<order, ColorType>() {}

#ifdef RAINBOW
    // calculate parity
    static vector<int> GetParityVector(const FaceCube& cube,
                                       const FaceCube& target) {
        // vector<int> ret((order / 2 - 1) * (order - order / 2 - 1));
        vector<int> ret;
        vector<int> V(24);
        for (int x = 1; x < order / 2; x++) {
            for (int y = 1; y < order - order / 2; y++) {
                for (int face_id = 0; face_id < 6; face_id++) {
                    for (int orientation = 0; orientation < 4; orientation++) {
                        int idx_true =
                            target.faces[face_id].Get(y, x, orientation).data;
                        int idx =
                            cube.faces[face_id].Get(y, x, orientation).data;
                        // cerr << y << " " << x << " " << face_id << " "
                        //      << orientation << " " << idx_true << " " << idx
                        //      << endl;
                        V[idx_true] = idx;
                    }
                }
                int idx = (x - 1) + (order / 2 - 1) * (y - 1);
                // for (auto& v : V) {
                //     cerr << v << " ";
                // }
                // cerr << endl;
                // ret[idx] = InversionParityInplace(V) % 2;
                if (InversionParityInplace(V) % 2 == 1)
                    ret.emplace_back(idx);
            }
        }
        return ret;
    }

    static vector<int> GetParityVectorFlag(const FaceCube& cube,
                                           const FaceCube& target) {
        vector<int> ret((order / 2 - 1) * (order - order / 2 - 1));
        for (auto& idx : GetParityVector(cube, target)) {
            ret[idx] = 1;
        }
        return ret;
    }
#endif

    inline auto ComputeFaceScore(const FaceCube& target) const {
        auto score = 0;
#ifdef RAINBOW
        // FaceCube cube_copy = *this;
        for (auto face_id = 0; face_id < 6; face_id++) {
            assert(target.faces[face_id].GetOrientation() == 0);
            int initial_orientation = this->faces[face_id].GetOrientation();
            // while (cube_copy.faces[face_id].GetOrientation() != 0) {
            //     cube_copy.faces[face_id].RotateCW(1);
            // }

            for (int y = 1; y < order - 1; y++) {
                for (int x = 1; x < order - 1; x++) {
                    int coef = 1;
                    if ((order % 2 == 1) && (x == order / 2) &&
                        (y == order / 2))
                        coef = 100;
                    // distance of face
                    i8 c1 = this->Get(face_id, y, x).data;
                    // i8 c2 = target.Get(face_id, y, x).data;
                    i8 c2 = target.faces[face_id]
                                .Get(y, x, initial_orientation)
                                .data;

                    score += coef * RainbowDist(c1, c2);
                }
            }

            // while (cube_copy.faces[face_id].GetOrientation() !=
            //        initial_orientation) {
            //     cube_copy.faces[face_id].RotateCW(1);
            // }
        }

        // parity check
        if (1) {
            int sum_parity = 0;
            vector<i8> V(24);
            for (int x = 1; x < order / 2; x++) {
                for (int y = 1; y < order - order / 2; y++) {
                    // if (x == y)
                    //     continue;
                    int cnt_wrong =
                        0; // TODO  位置が違うマス数 <= 4 ならパリティ考慮?
                    for (int face_id = 0; face_id < 6; face_id++) {
                        for (int orientation = 0; orientation < 4;
                             orientation++) {
                            // int idx_true = face_id * 4 + orientation;
                            int idx_true = target.faces[face_id]
                                               .Get(y, x, orientation)
                                               .data;
                            int idx = this->faces[face_id]
                                          .Get(y, x, orientation)
                                          .data;
                            V[idx_true] = idx;
                            if (idx_true != idx)
                                cnt_wrong++;
                        }
                        // if (cnt_wrong > 5) {
                        //     break;
                        // }
                    }
                    // if (cnt_wrong <= 5)
                    //     sum_parity += InversionParityInplace(V);
                    // sum_parity += InversionParityInplace(V) * (24 -
                    // cnt_wrong);
                    if ((order % 2 == 1) && y == order / 2)
                        sum_parity +=
                            InversionParityInplace(V) * COEF_PARITY_CROSS;
                    else
                        sum_parity += InversionParityInplace(V);
                }
            }
            // score += sum_parity * 10;
            // cerr << "socre = " << score << endl;
            // cerr << "sum_parity naive = " << sum_parity << endl;
            score += sum_parity * COEF_PARITY;
        }
#else
        for (auto face_id = 0; face_id < 6; face_id++) {
            for (int y = 1; y < order - 1; y++) {
                for (int x = 1; x < order - 1; x++) {
                    int coef = 1;
                    if ((order % 2 == 1) && (x == order / 2) &&
                        (y == order / 2))
                        coef = 100;
                    // distance of face
                    i8 c1 = this->Get(face_id, y, x).data;
                    i8 c2 = target.Get(face_id, y, x).data;
                    // if (Cube<order, ColorType>::GetOppositeFaceId(c1) == c2)
                    // {
                    //     coef *= 2;
                    // }

                    score += coef * FaceCube::GetFaceDistance(c1, c2);
                }
            }
        }
#endif
        return score;
    }

    inline void FromCube(const Cube<order, ColorType>& rhs) {
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
    using FaceCube = ::FaceCube<order, ColorTypeChameleon>;
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
#ifdef RAINBOW
    inline int ScoreWhenApplied(const FaceAction& action, FaceCube& target_cube,
                                const vector<int>& parity_cube,
                                const vector<int>& parity_action) const {
#else
    inline int ScoreWhenApplied(const FaceAction& action,
                                FaceCube& target_cube) const {
#endif
        assert(action.use_facelet_changes);
#ifdef RAINBOW

    // static Cube<Order, ColorType6> cube_for_orientation;
    // cube_for_orientation.RotateOrientation(action);

    Cube<2, ColorType6> cube_for_orientation;
    for (auto& mov : action.moves) {
        if (mov.depth == 0)
            cube_for_orientation.Rotate(mov);
        else if (mov.depth == Order - 1) {
            cube_for_orientation.RotateOrientation(Move{mov.direction, 1});
        }
    }
#endif
        int score_when_applied = score;
        for (const auto& facelet_change : action.facelet_changes) {
            const auto& from = facelet_change.from;
            const auto& to = facelet_change.to;

            if (from.x == 0 || from.x == order - 1 || from.y == 0 ||
                from.y == order - 1) {
                // continue;
                assert(false);
            }

            int coef = 1;
            if (((order & 1) == 1) && (from.x * 2 + 1 == order) &&
                (from.y * 2 + 1 == order))
                coef = 100;

            // score_when_applied += (int(color_from == color_from_target) -
            //                        int(color_from == color_to_target)) *
            //                       coef;

#ifdef RAINBOW
            const int orientation_from =
                cube.faces[from.face_id].GetOrientation();
            int orientation_to =
                (cube.faces[to.face_id].GetOrientation() +
                 cube_for_orientation.faces[to.face_id].GetOrientation() + 4) &
                3;
            const auto color_from_target = target_cube.faces[from.face_id].Get(
                from.y, from.x, orientation_from);
            const auto color_to_target =
                target_cube.faces[to.face_id].Get(to.y, to.x, orientation_to);
            const auto color_from = cube.Get(from);

            score_when_applied +=
                (RainbowDist(color_from.data, color_to_target.data) -
                 RainbowDist(color_from.data, color_from_target.data)) *
                coef;
#else

            const auto color_from_target = target_cube.Get(from);
            const auto color_to_target = target_cube.Get(to);

            if (color_from_target == color_to_target)
                continue;

            const auto color_from = cube.Get(from);
            score_when_applied +=
                (FaceCube::GetFaceDistance(color_from.data,
                                           color_to_target.data) -
                 FaceCube::GetFaceDistance(color_from.data,
                                           color_from_target.data)) *
                coef;
#endif
        }

#ifdef RAINBOW
        if (1) {
            // parity
            int cnt_parity = 0;
            // for (auto& idx : parity_cube) {
            //     cerr << idx << " ";
            // }
            // cerr << endl;
            for (const auto& idx : parity_action) {
                // cerr << idx << endl;
                assert(idx < parity_cube.size());
                if ((order % 2 == 1) &&
                    idx >= (order / 2 - 1) * (order / 2 - 1)) {
                    if (parity_cube[idx])
                        cnt_parity -= COEF_PARITY_CROSS;
                    else
                        cnt_parity += COEF_PARITY_CROSS;
                } else {
                    if (parity_cube[idx])
                        cnt_parity--;
                    else
                        cnt_parity++;
                }
            }
            // cerr << "socre = " << score_when_applied << endl;
            // cerr << "sum_parity fast = " << cnt_parity << endl;
            score_when_applied += cnt_parity * COEF_PARITY;
        }

#endif

#ifdef TESTSCORE
        {
            cube.Rotate(action);
            int score_when_applied_true = cube.ComputeFaceScore(target_cube);
            cube.RotateInv(action);
            if (score_when_applied != score_when_applied_true) {
                cerr << score_when_applied << " " << score_when_applied_true
                     << " " << action.facelet_changes.size() << endl;
                // for (int i = 0; i < 6; i++) {
                //     cerr <<
                //     (int)cube_for_orientation.faces[i].GetOrientation()
                //          << " ";
                // }
                cerr << endl;
                action.Print(cerr);
                cerr << endl;
                cube.Display(cerr);
                cerr << endl;
                cube.Rotate(action);
                cube.Display(cerr);
                cerr << endl;
                target_cube.Display(cerr);
                assert(false);
            } else {
                // cerr << score_when_applied << " " << score_when_applied_true
                //      << endl;
            }
            assert(score_when_applied == score_when_applied_true);
            // return score_when_applied;
        }
#endif

#ifdef RAINBOW
        cube_for_orientation.RotateOrientationInv(action);
#endif

        return score_when_applied;
    }

    // action, target_cube から score を計算する
    inline int ScoreWhenApplied(const FaceAction& action,
                                const FaceCube& target_cube,
                                const SliceMap& slice_map,
                                const SliceMapInv& slice_map_inv) {
        int score_when_applied;

        // if (action.use_facelet_changes) {
        if (false) {

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

                                int coef = 1;
                                if (((Order & 1) == 1) &&
                                    (from.x * 2 + 1 == Order) &&
                                    (from.y * 2 + 1 == Order))
                                    coef = 100;

                                // score_when_applied +=
                                //     (int(color_from == color_from_target) -
                                //      int(color_from == color_to_target)) *
                                //     coef;
#ifdef RAINBOW
                                const int orientation_from =
                                    cube.faces[from.face_id].GetOrientation();
                                const int orientation_to =
                                    cube.faces[to.face_id].GetOrientation();
                                const auto color_from_target =
                                    target_cube.faces[from.face_id].Get(
                                        from.y, from.x, orientation_from);
                                const auto color_to_target =
                                    target_cube.faces[to.face_id].Get(
                                        to.y, to.x, orientation_to);
                                const auto color_from = cube.Get(from);

                                score_when_applied +=
                                    (RainbowDist(color_from.data,
                                                 color_to_target.data) -
                                     RainbowDist(color_from.data,
                                                 color_from_target.data)) *
                                    coef;
#else
                                const auto color_from_target =
                                    target_cube.Get(from);
                                const auto color_to_target =
                                    target_cube.Get(to);
                                if (color_from_target == color_to_target)
                                    continue;
                                const auto color_from = cube.Get(from);
                                score_when_applied +=
                                    (FaceCube::GetFaceDistance(
                                         color_from.data,
                                         color_to_target.data) -
                                     FaceCube::GetFaceDistance(
                                         color_from.data,
                                         color_from_target.data)) *
                                    coef;
#endif
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
    using FaceCube = ::FaceCube<order, ColorTypeChameleon>;
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

#ifdef RAINBOW
    vector<vector<int>> actions_parity;
#endif

    // split actions into N parts evenly
    vector<FaceActionCandidateGenerator> Split(int N) {
        vector<FaceActionCandidateGenerator> ret(N);

        long long changes_sum = 0;
        for (int i = 0; i < (int)actions.size(); i++) {
            changes_sum += get<0>(actions[i]).facelet_changes.size();
        }
        long long changes_sum_per_part = changes_sum / N;
        long long changes = 0;
        int idx = 0;
        for (int i = 0; i < (int)actions.size(); i++) {
            changes += get<0>(actions[i]).facelet_changes.size();
            ret[idx].actions.emplace_back(move(actions[i]));
#ifdef RAINBOW
            ret[idx].actions_parity.emplace_back(move(actions_parity[i]));
#endif
            if (idx != N - 1 && changes > changes_sum_per_part) {
                cerr << "changes = " << changes << endl;
                idx++;
                changes = 0;
            }
        }
        cerr << "changes = " << changes << endl;

        actions.clear();
#ifdef RAINBOW
        actions_parity.clear();
#endif
        return ret;

        // // original
        //         vector<int> ret_sum(N);
        //         vector<int> V(actions.size());
        //         iota(V.begin(), V.end(), 0);
        //         sort(V.begin(), V.end(), [&](int i, int j) {
        //             return get<0>(actions[i]).Cost() >
        //             get<0>(actions[j]).Cost();
        //         });
        //         for (auto& v : V) {
        //             int min_idx =
        //                 min_element(ret_sum.begin(), ret_sum.end()) -
        //                 ret_sum.begin();
        //             ret_sum[min_idx] += get<0>(actions[v]).Cost();
        //             ret[min_idx].actions.emplace_back(move(actions[v]));
        //             // ret[min_idx].actions.emplace_back(actions[v]);
        // #ifdef RAINBOW
        //             ret[min_idx].actions_parity.emplace_back(move(actions_parity[v]));
        //             //
        //             ret[min_idx].actions_parity.emplace_back(actions_parity[v]);
        // #endif
        //         }
        //         // print
        //         for (auto& r : ret_sum) {
        //             cerr << "split size = " << r << endl;
        //         }
        //         return ret;
    }

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

        vector<FaceAction> actions_tmp1;

        // ファイルから読み取る
        auto ifs = ifstream(filename);
        if (!ifs.good()) {
            cerr << format("Cannot open file `{}`.", filename) << endl;
            abort();
        }
        string line;
        int cnt = 0;
        while (getline(ifs, line)) {
            if (line.empty() || line[0] == '#')
                continue;
            cnt++;
            cerr << "read lines = " << cnt << "\r" << flush;
            FaceAction faceaction_formula(line);
            // faceaction_formula
            //     .EnableFaceletChangesAll<Cube<OrderFormula, ColorType24>>();
            faceaction_formula.EnableFaceletChangesWithNoSameRaw<
                Cube<OrderFormula, ColorType24>>();
            faceaction_formula.DisableFaceletChangeEdgeCorner<
                Cube<OrderFormula, ColorType24>>();

            // if (faceaction_formula.moves.size() == 1) {
            //     cerr << line << endl;
            //     for (auto& facelet_change :
            //          faceaction_formula.facelet_changes) {
            //         cerr << (int)facelet_change.from.face_id << " "
            //              << (int)facelet_change.from.y << " "
            //              << (int)facelet_change.from.x << " : "
            //              << (int)facelet_change.to.face_id << " "
            //              << (int)facelet_change.to.y << " "
            //              << (int)facelet_change.to.x << endl;
            //     }
            // }

#ifndef RAINBOW
            faceaction_formula
                .DisableFaceletChangeSameFace<Cube<OrderFormula, ColorType6>>();
#endif

            // if (faceaction_formula.facelet_changes.size() == 3) {
            if (false) {
                bool flag = true;
                for (auto& mov : faceaction_formula.moves) {
                    if (mov.depth * 2 + 1 == OrderFormula) {
                        flag = false;
                        break;
                    }
                }
                for (auto& ch : faceaction_formula.facelet_changes) {
                    if (ch.to.x == ch.to.y ||
                        ch.to.x == OrderFormula - 1 - ch.to.y) {
                        flag = false;
                        break;
                    } else {
                        // flag = false;
                        // break;
                    }
                }
                vector<int> face_id_cnt(6);
                for (auto& ch : faceaction_formula.facelet_changes) {
                    face_id_cnt[ch.from.face_id]++;
                }
                int cnt = 0;
                for (auto& cnt_face_id : face_id_cnt) {
                    if (cnt_face_id)
                        cnt++;
                }
                if (cnt != 3) {
                    flag = false;
                }
                static int cnt2 = 0;
                if (flag) {
                    cnt2++;
                    cerr << "Count : " << cnt2 << endl << endl;
                    Cube<OrderFormula, ColorType24> cube;
                    cube.Reset();
                    cube.Rotate(faceaction_formula.moves);
                    cube.Display(cerr);
                    cerr << endl;
                    faceaction_formula.Print(cerr);
                    cerr << endl;
                    for (auto& ch : faceaction_formula.facelet_changes) {
                        cerr << (int)ch.from.face_id << " " << (int)ch.from.y
                             << " " << (int)ch.from.x << " : "
                             << (int)ch.to.face_id << " " << (int)ch.to.y << " "
                             << (int)ch.to.x << endl;
                    }
                }
            }

            vector<int> face_id_cnt(6);
            for (auto& ch : faceaction_formula.facelet_changes) {
                face_id_cnt[ch.from.face_id]++;
            }
            int face_id_cnt_sum = 0;
            for (auto& cnt_face_id : face_id_cnt) {
                if (cnt_face_id)
                    face_id_cnt_sum++;
            }

            // if (faceaction_formula.facelet_changes.size() <= 3) {
            //     actions_tmp1.emplace_back(faceaction_formula);
            // }

            if (1 <= face_id_cnt_sum && face_id_cnt_sum <= 3) {
                if (0) {
                    bool flag = true;
                    if (face_id_cnt_sum != 3)
                        flag = false;
                    if (faceaction_formula.facelet_changes.size() != 3)
                        flag = false;
                    for (int i = 0;
                         i < (int)faceaction_formula.facelet_changes.size();
                         i++) {
                        int x = faceaction_formula.facelet_changes[i].from.x;
                        int y = faceaction_formula.facelet_changes[i].from.y;
                        if (!(x == OrderFormula / 2 || y == OrderFormula / 2))
                            flag = false;
                    }
                    if (flag) {
                        Cube<OrderFormula, ColorType24> cube;
                        cube.Reset();
                        cube.Rotate(faceaction_formula.moves);
                        cube.Display(cerr);
                        cerr << endl;
                    }
                }

                // if (false) {
                // static int cnt = 0;
                // cnt++;
                // cout << cnt << endl;
                // cout << cnt << endl;

                // 全ての面に回転を加える
                vector<int> V_face_id;
                for (int face_id = 0; face_id < 6; face_id++) {
                    if (face_id_cnt[face_id]) {
                        V_face_id.emplace_back(face_id);
                    }
                }
                // for (int i = 0; i < (1 << (2 * V_face_id.size())); i++) {
                for (int i = 0; i < (1 << (1 * V_face_id.size())); i++) {
                    // for (int i = 0; i < (1 << max(4, int(1 *
                    // V_face_id.size())));
                    //      i++) {
                    FaceAction faceaction_formula_tmp;
                    for (int j = 1; j < (int)V_face_id.size(); j++) {
                        // int cnt = (i >> (2 * j)) & 3;
                        int cnt = 0;
                        if (j == 0)
                            cnt = 0;
                        if (j == 1)
                            cnt = i & 3;
                        else if (j == 2)
                            cnt = (i >> 2) & 1;
                        else
                            assert(false);
                        if (cnt == 0) {
                        } else if (cnt == 3) {
                            faceaction_formula_tmp.moves.emplace_back(
                                Cube<OrderFormula, ColorType24>::
                                    GetFaceRotateMoveInv(V_face_id[j]));
                        } else {
                            for (int k = 0; k < cnt; k++) {
                                faceaction_formula_tmp.moves.emplace_back(
                                    Cube<OrderFormula, ColorType24>::
                                        GetFaceRotateMove(V_face_id[j]));
                            }
                        }
                    }
                    // 結合
                    copy(faceaction_formula.moves.begin(),
                         faceaction_formula.moves.end(),
                         back_inserter(faceaction_formula_tmp.moves));
                    faceaction_formula_tmp.EnableFaceletChangesWithNoSameRaw<
                        Cube<OrderFormula, ColorType24>>();
                    faceaction_formula_tmp.DisableFaceletChangeEdgeCorner<
                        Cube<OrderFormula, ColorType24>>();
#ifndef RAINBOW
                    faceaction_formula_tmp.DisableFaceletChangeSameFace<
                        Cube<OrderFormula, ColorType6>>();
#endif
                    actions_tmp1.emplace_back(faceaction_formula_tmp);
                }
            } else {
                actions_tmp1.emplace_back(faceaction_formula);
            }
        }
        cerr << endl;

        int idx_faceaction = 0;
        for (auto& faceaction_formula : actions_tmp1) {
            idx_faceaction++;
            cerr << "idx_faceaction = " << idx_faceaction << "/"
                 << actions_tmp1.size() << "\r" << flush;

            // if (faceaction_formula.facelet_changes.size() > 6) {
            //     continue;
            //     // if (faceaction_formula.moves.size() == 1 &&
            //     //     (faceaction_formula.moves[0].depth == 0 ||
            //     //      faceaction_formula.moves[0].depth == OrderFormula
            //     - 1)) {
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
                // action_new.EnableFaceletChangesAll<
                //     Cube<OrderFormula + 2, ColorType24>>();
                action_new.EnableFaceletChangesWithNoSameRaw<
                    Cube<OrderFormula + 2, ColorType24>>();
                action_new.DisableFaceletChangeEdgeCorner<
                    Cube<OrderFormula + 2, ColorType24>>();
#ifndef RAINBOW
                action_new.DisableFaceletChangeSameFace<
                    Cube<OrderFormula + 2, ColorType6>>();
#endif
                if (action_new.facelet_changes.size() !=
                    faceaction_formula.facelet_changes.size()) {
                    flag_scale = false;
                }
            }

            // int cnt = 0;
            for (int i = 0; i < (int)slice_maps.size(); i++) {
                bool flag_use = true;
                // auto& slice_map = slice_maps[i];
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
                        // faceaction.EnableFaceletChangesAll<
                        //     Cube<Order, ColorType24>>();
                        faceaction.EnableFaceletChangesWithNoSameRaw<
                            Cube<Order, ColorType24>>();
                        faceaction.DisableFaceletChangeEdgeCorner<
                            Cube<Order, ColorType24>>();
#ifndef RAINBOW
                        faceaction.DisableFaceletChangeSameFace<
                            Cube<Order, ColorType6>>();
#endif
                        actions.emplace_back(faceaction, faceaction_formula,
                                             slice_map, slice_map_inv, false);
                    }
                }
            }
            // cerr << cnt << endl;
            // cerr << endl;
        }
        cerr << endl;

#ifdef RAINBOW
        {
            cerr << "update parity vectors" << endl;
            actions_parity.clear();
            actions_parity.reserve(actions.size());
            // FaceCube<Order, ColorType24> cube();
            auto cube = ::FaceCube<Order, ColorType24>();
            auto target = ::FaceCube<Order, ColorType24>();
            target.Reset();
            int cnt = 0;
            for (const auto& action : actions) {
                cerr << "update parity vectors : " << cnt++ << "/"
                     << actions.size() << "\r" << flush;
                //  << endl;
                const auto& [faceaction, faceaction_formula, slice_map,
                             slice_map_inv, flag_last_action_scale] = action;
                cube.Reset();
                // cube.Display();
                cube.Rotate(faceaction);
                // cube.Display();
                actions_parity.emplace_back(
                    ::FaceCube<Order, ColorType24>::GetParityVector(cube,
                                                                    target));
            }
            cerr << endl;
        }
#endif

        // // print
        // for (auto& action : actions) {
        //     auto [action_new, action_formula, slice_map, slice_map_inv,
        //           flag_last_action_scale] = action;
        //     cerr << endl;
        //     action_formula.Print(cerr);
        //     cerr << endl;
        //     for (auto& facelet_change : action_formula.facelet_changes) {
        //         cerr << (int)facelet_change.from.face_id << " "
        //              << (int)facelet_change.from.y << " "
        //              << (int)facelet_change.from.x << " : "
        //              << (int)facelet_change.to.face_id << " "
        //              << (int)facelet_change.to.y << " "
        //              << (int)facelet_change.to.x << endl;
        //     }
        // }

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
    FaceState CopyState() const { return state; }
};

template <int order> struct FaceBeamSearchSolver {
    static_assert(order == Order);
    using FaceCube = ::FaceCube<order, ColorTypeChameleon>;
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

    inline shared_ptr<FaceNode> Solve(const FaceCube& start_cube,
                                      const int id = -1) {
        auto rng = RandomNumberGenerator(42);
        vector<RandomNumberGenerator> rngs;
        if (n_threads >= 2) {
            for (int i = 0; i < n_threads; i++) {
                rngs.emplace_back(RandomNumberGenerator(i));
            }
        }

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

        assert(n_threads >= 1);
        vector<thread> threads;

        int max_action_cost = 0;
        if (n_threads >= 2) {
            for (const auto& action : action_candidate_generator.actions) {
                max_action_cost = max(max_action_cost, get<0>(action).Cost());
            }
        }

        vector<FaceActionCandidateGenerator> multi_action_candidate_generator;
        if (n_threads >= 2) {
            multi_action_candidate_generator =
                action_candidate_generator.Split(n_threads - 1);
        }

        shared_ptr<FaceNode> node_solved;

        while (true) {
            for (auto current_cost = 0; current_cost < 100000; current_cost++) {
                auto current_minimum_score = 9999;
                if (nodes[current_cost].empty()) {
                    continue;
                }
                nodes[current_cost][0]->state.cube.Display(cerr);
                cout << format("current_cost={} nodes={}", current_cost,
                               nodes[current_cost].size())
                     << endl;
                // for (const auto& node : nodes[current_cost]) {
                for (int idx_node = (int)nodes[current_cost].size() - 1;
                     idx_node >= 0; idx_node--) {
                    const auto& node = nodes[current_cost][idx_node];
                    if (!node)
                        continue;
                    if (current_cost == 0 &&
                        idx_node != (int)nodes[0].size() - 1)
                        break;
                    current_minimum_score =
                        min(current_minimum_score, node->state.score);
                    if (node->state.score == 0) {
                        cerr << "Solved!" << endl;
                        // return node;
                        node_solved = node;
                        goto BEAM_END;
                    }

                    cout << format("score={}, last_action_cost={} "
                                   "facelet_changes_len={} "
                                   "facelet_changes_len_formula={}",
                                   node->state.score, node->last_action.Cost(),
                                   node->last_action.facelet_changes.size(),
                                   node->last_action_formula.facelet_changes
                                       .size())
                         << endl;

#ifdef RAINBOW
                    auto parity_cube = FaceCube::GetParityVectorFlag(
                        node->state.cube, target_cube);
#endif

                    if (n_threads == 1) {
                        // for (const auto& [action, action_formula, slice_map,
                        //                   slice_map_inv,
                        //                   flag_last_action_scale]
                        //                   :
                        //      action_candidate_generator.Generate(node->state))
                        //      {
                        for (int idx_action = 0;
                             idx_action <
                             (int)action_candidate_generator.actions.size();
                             idx_action++) {
                            const auto& [action, action_formula, slice_map,
                                         slice_map_inv,
                                         flag_last_action_scale] =
                                action_candidate_generator.actions[idx_action];

#ifdef RAINBOW
                            const auto& parity_action =
                                action_candidate_generator
                                    .actions_parity[idx_action];
#endif

                            int new_n_moves =
                                node->state.n_moves + action.Cost();
                            if (new_n_moves >= (int)nodes.size())
                                nodes.resize(new_n_moves + 1);
                            if ((int)nodes[new_n_moves].size() < beam_width) {
                                auto new_state = node->state;
                                // new_state.Apply(action, target_cube,
                                // slice_map,
                                //                 slice_map_inv);
                                new_state.Apply(action, target_cube);
                                nodes[new_state.n_moves].emplace_back(
                                    new FaceNode(new_state, node, action,
                                                 action_formula, slice_map,
                                                 slice_map_inv,
                                                 flag_last_action_scale));
                            } else {
                                if (false) {
                                    auto new_state = node->state;
                                    new_state.Apply(action, target_cube);
                                    int new_score_slow = new_state.score;
                                    // const auto idx = rng.Next() % beam_width;
                                    // if (new_score_slow <
                                    //     nodes[new_n_moves][idx]->state.score)
                                    //     { nodes[new_n_moves][idx].reset(
                                    //         new FaceNode(new_state, node,
                                    //         action,
                                    //                      slice_map,
                                    //                      slice_map_inv));
                                    // }

                                    FaceAction action_new =
                                        ConvertFaceActionMoveWithSliceMap<
                                            OrderFormula, Order>(
                                            action, slice_map, slice_map_inv);

                                    action_new.Print(cerr);
                                    cerr << endl;
                                    action_new
                                        .EnableFaceletChangesWithNoSameRaw<
                                            Cube<Order, ColorType24>>();
                                    action_new.DisableFaceletChangeEdgeCorner<
                                        Cube<Order, ColorType24>>();
                                    action_new.DisableFaceletChangeSameFace<
                                        Cube<Order, ColorType6>>();
                                    for (auto& facelet_change :
                                         action_new.facelet_changes) {
                                        cerr
                                            << (int)facelet_change.from.face_id
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
                                    action_tmp
                                        .EnableFaceletChangesWithNoSameRaw<
                                            Cube<OrderFormula, ColorType24>>();
                                    action_tmp.DisableFaceletChangeEdgeCorner<
                                        Cube<OrderFormula, ColorType24>>();
                                    action_tmp.DisableFaceletChangeSameFace<
                                        Cube<OrderFormula, ColorType6>>();
                                    for (auto& facelet_change :
                                         action_tmp.facelet_changes) {
                                        cerr
                                            << (int)facelet_change.from.face_id
                                            << " " << (int)facelet_change.from.y
                                            << " " << (int)facelet_change.from.x
                                            << " : "
                                            << (int)facelet_change.to.face_id
                                            << " " << (int)facelet_change.to.y
                                            << " " << (int)facelet_change.to.x
                                            << endl;
                                    }
                                    cerr << endl;
                                    int new_score =
                                        node->state.ScoreWhenApplied(
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
                                //

#ifdef NAIVE
                                auto new_state = node->state;
                                new_state.Apply(action, target_cube);
                                int new_score = new_state.score;
                                const auto idx = rng.Next() % beam_width;
                                if (new_score <
                                    nodes[new_n_moves][idx]->state.score) {
                                    nodes[new_n_moves][idx].reset(new FaceNode(
                                        new_state, node, action, action_formula,
                                        slice_map, slice_map_inv,
                                        flag_last_action_scale));
                                }
#else

#ifdef RAINBOW
                                int new_score = node->state.ScoreWhenApplied(
                                    action, target_cube, parity_cube,
                                    parity_action);
#else
                                int new_score = node->state.ScoreWhenApplied(
                                    action, target_cube);
#endif

                                const auto idx = rng.Next() % beam_width;
                                if (new_score <
                                    nodes[new_n_moves][idx]->state.score) {
                                    // if (true) {
                                    auto new_state = node->state;
                                    // new_state.Apply(action, target_cube,
                                    // slice_map,
                                    //                 slice_map_inv);
                                    new_state.Apply(action, target_cube);
                                    // cerr << new_score << " " <<
                                    // new_state.score
                                    //      << endl;
                                    assert(new_state.score == new_score);
                                    if (new_state.score != new_score) {
                                        cerr << "score did not match" << endl;
                                        cerr << new_state.score << " "
                                             << new_score << endl;
                                        exit(1);
                                    }
                                    nodes[new_n_moves][idx].reset(new FaceNode(
                                        new_state, node, action, action_formula,
                                        slice_map, slice_map_inv,
                                        flag_last_action_scale));
                                }
#endif
                            }
                        }

                        // TODO 並列化
                        // #ifndef NAIVE
                        // cerr << "parallel" << endl;
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
                                    vector<int> vec_use_slices(OrderFormula - 2,
                                                               0);
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
                                    // for (int i = 0; i < OrderFormula - 2;
                                    // i++) {
                                    //     cerr << slice_map_inv_new[i].size()
                                    //     << "
                                    //     ";
                                    // }
                                    // cerr << endl;

                                    for (int slice_idx = 0;
                                         slice_idx < Order - 2; slice_idx++) {
                                        if (slice_map_new[slice_idx] != -1) {
                                            continue;
                                        }
                                        if constexpr (Order % 2 == 1) {
                                            if (slice_idx == Order / 2 - 1) {
                                                continue;
                                            }
                                        }
                                        for (int slice_idx_formula = 0;
                                             slice_idx_formula <
                                             OrderFormula - 2;
                                             slice_idx_formula++) {
                                            if constexpr (OrderFormula % 2 ==
                                                          1) {
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
                                            slice_map_new[Order - 3 -
                                                          slice_idx] =
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

                                            auto new_state =
                                                node->parent->state;
                                            new_state.Apply(action_new,
                                                            target_cube);
                                            const auto idx =
                                                rng.Next() % beam_width;
                                            if (new_state.score <
                                                nodes[new_state.n_moves][idx]
                                                    ->state.score) {
                                                nodes[new_state.n_moves][idx]
                                                    .reset(new FaceNode(
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
                                            //     for (int i = 0; i < Order -
                                            //     2; i++) {
                                            //         cerr << slice_map_new[i]
                                            //         << "
                                            //         ";
                                            //     }
                                            //     cerr << endl;
                                            //     for (int i = 0; i <
                                            //     OrderFormula
                                            //     - 2;
                                            //          i++) {
                                            //         cerr <<
                                            //         slice_map_inv_new[i].size()
                                            //              << " ";
                                            //     }
                                            //     cerr << endl;
                                            //     cerr << new_state.score << "
                                            //     "
                                            //          << new_score << endl;
                                            //     assert(new_state.score ==
                                            //     new_score);
                                            //     //
                                            //     nodes[new_n_moves][idx].reset(
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
                                            slice_map_new[Order - 3 -
                                                          slice_idx] = -1;
                                            slice_map_inv_new[OrderFormula - 3 -
                                                              slice_idx_formula]
                                                .pop_back();
                                        }
                                    }
                                }
                            }
                        // #endif

                    } else {
                        vector<thread> threads;
                        vector<vector<vector<shared_ptr<FaceNode>>>> nodes_memo(
                            n_threads,
                            vector(beam_width,
                                   vector<shared_ptr<FaceNode>>(
                                       max_action_cost + 10, start_node)));

                        for (int i = 0; i < n_threads - 1; i++) {
                            thread th(
                                [&](int ii) {
                                    auto& action_candidate_generator =
                                        multi_action_candidate_generator[ii];
                                    auto& rng = rngs[ii];
                                    for (int idx_action = 0;
                                         idx_action <
                                         (int)action_candidate_generator.actions
                                             .size();
                                         idx_action++) {
                                        const auto& [action, action_formula,
                                                     slice_map, slice_map_inv,
                                                     flag_last_action_scale] =
                                            action_candidate_generator
                                                .actions[idx_action];
#ifdef RAINBOW
                                        const auto& parity_action =
                                            action_candidate_generator
                                                .actions_parity[idx_action];
#endif
                                    // int new_n_moves =
                                    //     node->state.n_moves + action.Cost();

#ifdef RAINBOW
                                        int new_score =
                                            node->state.ScoreWhenApplied(
                                                action, target_cube,
                                                parity_cube, parity_action);
#else
                                        int new_score =
                                            node->state.ScoreWhenApplied(
                                                action, target_cube);
#endif

                                        const auto idx =
                                            rng.Next() % beam_width;
                                        auto& node_nxt =
                                            nodes_memo[ii][idx][action.Cost()];
                                        if ((!node_nxt) ||
                                            new_score < node_nxt->state.score) {
                                            // auto new_state = node->state;
                                            auto new_state = node->CopyState();
                                            new_state.Apply(action,
                                                            target_cube);
                                            if (new_state.score != new_score) {
                                                cerr << "score did not match"
                                                     << endl;
                                                cerr << new_state.score << " "
                                                     << new_score << endl;
                                                exit(1);
                                            }
                                            node_nxt.reset(new FaceNode(
                                                new_state, node, action,
                                                action_formula, slice_map,
                                                slice_map_inv,
                                                flag_last_action_scale));
                                        }
                                    }
                                },
                                i);
                            threads.emplace_back(move(th));
                        }

                        // parallel
                        if constexpr (flag_parallel) {
                            if (node->parent) {
                                thread th([&]() {
                                    auto& rng = rngs[n_threads - 1];
                                    SliceMap slice_map_new = node->slice_map;
                                    SliceMapInv slice_map_inv_new =
                                        node->slice_map_inv;

                                    // list up used slices in formula
                                    vector<int> vec_use_slices(OrderFormula - 2,
                                                               0);
                                    for (Move& mv :
                                         node->last_action_formula.moves) {
                                        if (1 <= mv.depth &&
                                            mv.depth <= OrderFormula - 2) {
                                            vec_use_slices[mv.depth - 1] = 1;
                                            vec_use_slices[OrderFormula - 2 -
                                                           mv.depth] = 1;
                                        }
                                    }
                                    for (int slice_idx = 0;
                                         slice_idx < Order - 2; slice_idx++) {
                                        if (slice_map_new[slice_idx] != -1) {
                                            continue;
                                        }
                                        if constexpr (Order % 2 == 1) {
                                            if (slice_idx == Order / 2 - 1) {
                                                continue;
                                            }
                                        }
                                        for (int slice_idx_formula = 0;
                                             slice_idx_formula <
                                             OrderFormula - 2;
                                             slice_idx_formula++) {
                                            if constexpr (OrderFormula % 2 ==
                                                          1) {
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
                                            slice_map_new[Order - 3 -
                                                          slice_idx] =
                                                OrderFormula - 3 -
                                                slice_idx_formula;
                                            slice_map_inv_new[OrderFormula - 3 -
                                                              slice_idx_formula]
                                                .emplace_back(Order - 3 -
                                                              slice_idx);
                                            FaceAction action_new =
                                                ConvertFaceActionMoveWithSliceMap<
                                                    OrderFormula, Order>(
                                                    node->last_action_formula,
                                                    slice_map_new,
                                                    slice_map_inv_new);

                                            auto new_state =
                                                node->parent->state;
                                            new_state.Apply(action_new,
                                                            target_cube);
                                            const auto idx =
                                                rng.Next() % beam_width;
                                            auto& node_nxt =
                                                nodes_memo[n_threads - 1][idx]
                                                          [action_new.Cost() -
                                                           node->last_action
                                                               .Cost()];
                                            if (!node_nxt ||
                                                new_state.score <
                                                    node_nxt->state.score) {
                                                node_nxt.reset(new FaceNode(
                                                    new_state, node->parent,
                                                    action_new,
                                                    node->last_action_formula,
                                                    slice_map_new,
                                                    slice_map_inv_new,
                                                    node->flag_last_action_scale));
                                            }
                                            slice_map_new[slice_idx] = -1;
                                            slice_map_inv_new[slice_idx_formula]
                                                .pop_back();
                                            slice_map_new[Order - 3 -
                                                          slice_idx] = -1;
                                            slice_map_inv_new[OrderFormula - 3 -
                                                              slice_idx_formula]
                                                .pop_back();
                                        }
                                    }
                                });
                                threads.emplace_back(move(th));
                            }
                        }

                        for (auto& th : threads) {
                            th.join();
                        }
                        threads.clear();

                        // update nodes
                        for (int i = 0; i < n_threads; i++) {
                            for (int j = 0; j < beam_width; j++) {
                                for (int action_cost = 1;
                                     action_cost <= max_action_cost;
                                     action_cost++) {
                                    // cerr << nodes_memo[i][j][action_cost]
                                    //             ->state.score
                                    //      << endl;

                                    // if (!nodes_memo[i][j][action_cost])
                                    //     continue;

                                    // auto& node_sub =
                                    // nodes_memo[i][j][action_cost];

                                    // auto& node_new =
                                    //     nodes[current_cost + action_cost][j];

                                    // node_sub->state.cube.Display(cerr);
                                    // cerr << endl;
                                    // cerr << node_sub->state.score << " "
                                    //      << node_sub->state.n_moves << endl;

                                    // cerr << current_cost + action_cost <<
                                    // endl; swap(node_new, node_sub);
                                    // nodes[current_cost +
                                    // action_cost][j].reset(
                                    //     new FaceNode(
                                    //         // node->state,
                                    //         node_sub->state, node,
                                    //         node_sub->last_action,
                                    //         node_sub->last_action_formula,
                                    //         node_sub->slice_map,
                                    //         node_sub->slice_map_inv,
                                    //         node_sub->flag_last_action_scale));

                                    // nodes[current_cost + action_cost][j] =
                                    //     move(nodes_memo[i][j][action_cost]);

                                    auto& node_nxt =
                                        nodes_memo[i][j][action_cost];
                                    if (!node_nxt)
                                        continue;
                                    auto& node =
                                        nodes[current_cost + action_cost][j];
                                    if (!node) {
                                        node = move(node_nxt);
                                        // cerr << node->state.score << " "
                                        //      << node_nxt->state.score <<
                                        //      endl;
                                    } else if (node_nxt->state.score <
                                               node->state.score) {
                                        node = node_nxt;
                                    }
                                }
                            }
                        }
                    }
                }

                // cout << format("current_cost={} current_minimum_score={}",
                //                current_cost, current_minimum_score)
                // << endl;
                // nodes[current_cost].clear();
            }

        BEAM_END:
            if (node_solved) {
                ostream& out = cout;
                out << "solved" << endl;
                out << node_solved->state.n_moves << endl;
                vector<Move> moves;
                for (auto p = node_solved; p->parent != nullptr;
                     p = p->parent) {
                    for (auto i = (int)p->last_action.moves.size() - 1; i >= 0;
                         i--) {
                        moves.emplace_back(p->last_action.moves[i]);
                    }
                }
                reverse(moves.begin(), moves.end());
                const auto solution = Formula(moves);
                solution.Print(out);
                out << endl;
                auto start_cube_copy = start_cube;
                start_cube_copy.Rotate(solution);
                start_cube_copy.Display(out);

                beam_width *= 2;
                if (n_threads >= 2) {
                    for (auto& nodess : nodes) {
                        // nodess.clear();
                        nodess.resize(beam_width, start_node);
                    }
                }

                // save to file
                if (id >= 0) {
                    // 追加
                    string filename_all =
                        format("solution_face/{}_all.txt", id);
                    // ベストのみ
                    string filename_best =
                        format("solution_face/{}_best.txt", id);

                    ofstream fout_all(filename_all, ios::app);
                    if (!fout_all) {
                        cerr << "cannot open " << filename_all << endl;
                        exit(1);
                    }
                    solution.Print(fout_all);
                    fout_all << endl;
                    fout_all << moves.size() << endl;
                    fout_all.close();

                    // read best score
                    int best_score = -1;
                    ifstream fin_best(filename_best);
                    if (fin_best) {
                        string line;
                        getline(fin_best, line);
                        getline(fin_best, line);
                        best_score = stoi(line);
                        fin_best.close();
                    }
                    if (best_score == -1 || best_score > (int)moves.size()) {
                        ofstream fout_best(filename_best, ios::out);
                        if (!fout_best) {
                            cerr << "cannot open " << filename_best << endl;
                            exit(1);
                        }
                        solution.Print(fout_best);
                        fout_best << endl;
                        fout_best << moves.size() << endl;
                        fout_best.close();
                    }
                }
            } else {
                break;
            }
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

[[maybe_unused]] static void TestFaceBeamSearch(int argc, char** argv) {
    // constexpr auto kOrder = 9;
    // const auto formula_file = "out/face_formula_9_7.txt";
    // constexpr auto kOrder = 9;
    constexpr int kOrder = Order;
    static_assert(kOrder == Order);
    auto beam_width = 1;

    int n_threads = N_THREADS;

    using Solver = FaceBeamSearchSolver<kOrder>;
    using FaceCube = typename Solver::FaceCube;

    int id = -1;

    auto initial_cube = FaceCube();
    if (0) {
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
        if (argc >= 2) {
            cerr << "argv[1] = " << argv[1] << endl;
            id = atoi(argv[1]);
        } else
            cin >> id;
        if (argc >= 3) {
            cerr << "argv[2] = " << argv[2] << endl;
            beam_width = atoi(argv[2]);
        }
        if (argc >= 4) {
            cerr << "argv[3] = " << argv[3] << endl;
            n_threads = atoi(argv[3]);
        }
        string filename_puzzles = "../../../input/santa-2023/puzzles.csv";
        string filename_sample =
            "../../../input/santa-2023/sample_submission.csv";
        auto [puzzle_size, is_normal, sample_formula] =
            ReadKaggleInput(filename_puzzles, filename_sample, id);

        assert(puzzle_size == kOrder);
        if (puzzle_size != kOrder) {
            cerr << "puzzle_size != kOrder" << endl;
            exit(1);
        }
#ifdef RAINBOW
        assert(!is_normal);
        if (is_normal) {
            cerr << "is_normal" << endl;
            exit(1);
        }
#else
        assert(is_normal);
        if (!is_normal) {
            cerr << "!is_normal" << endl;
            exit(1);
        }
#endif

        cout << "problem id = " << id << endl;

        initial_cube.Reset();
        initial_cube.RotateInv(sample_formula);

        // string s;
        // cin >> s;
        // initial_cube.Reset();
        // FaceAction faceaction(s);
        // initial_cube.RotateInv(faceaction);
    }

    cout << format("kOrder={} formula_file={} beam_width={} n_threads={}",
                   kOrder, formula_file, beam_width, n_threads)
         << endl;

    initial_cube.Display(cout);

    auto target_cube = FaceCube();
    target_cube.Reset();

    auto solver = Solver(target_cube, beam_width, formula_file, n_threads);

    const auto node = solver.Solve(initial_cube, id);
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
int main(int argc, char** argv) { TestFaceBeamSearch(argc, argv); }
#endif

/*
Rainbow では面の回転を加えない方が良い？
*/