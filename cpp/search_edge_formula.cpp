#include <unordered_map>
#include <unordered_set>

#include "edge_cube.cpp"

using std::max;
using std::unordered_map;
using std::unordered_set;

template <int order> struct EdgeFormulaSearcher {
    static constexpr auto kMaxNInnerRotations = 3;

    using Cube = ::Cube<order, ColorType24>;
    using EdgeCube = ::EdgeCube<order, ColorType48>;
    int max_depth;
    int max_depth_2;
    int max_cost;

    Cube start_cube;

    EdgeFormulaSearcher()
        : max_depth(), max_depth_2(), max_cost(), start_cube(),
          normal_results(), rainbow_results(), depth(), cube(), move_history(),
          inner_rotation_counts(), face_phases() {
        start_cube.Reset();
        assert(max_depth < (int)move_history.size());
        assert(max_depth_2 < (int)move_history.size());
    }

    struct Result {
        Formula formula;
    };

    tuple<vector<Result>, vector<Result>>
    Search(const int max_cost_, const int max_depth_, const int max_depth_2_,
           const int max_conjugate_depth) {
        max_cost = max_cost_;
        max_depth = max_depth_;
        max_depth_2 = max_depth_2_;

        normal_results.clear();
        rainbow_results.clear();
        cube = start_cube;

        // order > 9 の場合、9 で探索した後に augment する
        if constexpr (order > 9) {
            auto child_searcher = EdgeFormulaSearcher<9>();
            const auto [child_normal_results, child_rainbow_results] =
                child_searcher.Search(max_cost, max_depth, max_depth_2,
                                      max_conjugate_depth);
            // TODO
            assert(false);

            // 31C3

            RemoveDuplicates(normal_results, true);
            RemoveDuplicates(rainbow_results, true);
            return make_tuple(normal_results, rainbow_results);
        }

        // DFS で探索する
        cout << "Searching (1/2)..." << endl;
        Dfs();
        cout << format("Done. {} + {} formulas found.", normal_results.size(),
                       rainbow_results.size())
             << endl;

        // DFS (2) で探索する
        cout << "Searching (2/2)..." << endl;
        Dfs2();
        cout << format("Done. {} + {} formulas found.", normal_results.size(),
                       rainbow_results.size())
             << endl;

        // 重複を除去する
        cout << "Removing duplicates..." << endl;
        RemoveDuplicates(normal_results, false);
        RemoveDuplicates(rainbow_results, false);
        cout << format("Done. {} + {} formulas left.", normal_results.size(),
                       rainbow_results.size())
             << endl;

        for (auto i = 0; i < max_conjugate_depth; i++) {
            cout << format("Augmenting (conjugate {}/{})...", i + 1,
                           max_conjugate_depth)
                 << endl;
            AugmentConjugate(normal_results, rainbow_results);
            cout << format("Done. {} + {} formulas found.",
                           normal_results.size(), rainbow_results.size())
                 << endl;

            // 重複を除去する
            cout << "Removing duplicates..." << endl;
            RemoveDuplicates(normal_results, true);
            RemoveDuplicates(rainbow_results, true);
            cout << format("Done. {} + {} formulas left.",
                           normal_results.size(), rainbow_results.size())
                 << endl;
        }

        return make_tuple(normal_results, rainbow_results);
    }

    struct InnerRotationCounts {
        array<array<i8, order>, 3> counts; // (axis, depth) -> count
        int distance_from_all_zero;

        inline void Add(const Move mov) {
            auto& count = counts[(int)mov.GetAxis()][mov.depth];
            distance_from_all_zero -= count == 3 ? 1 : count;
            count = (count + (mov.IsClockWise() ? 1 : -1)) & 3;
            distance_from_all_zero += count == 3 ? 1 : count;
        }

        inline auto ComputeDeltaDistance(const Move mov) const {
            const auto count = counts[(int)mov.GetAxis()][mov.depth];
            const auto old_distance = count == 3 ? 1 : count;
            const auto new_count = (count + (mov.IsClockWise() ? 1 : -1)) & 3;
            const auto new_distance = new_count == 3 ? 1 : new_count;
            const auto delta_distance = new_distance - old_distance;
            assert(delta_distance == -1 || delta_distance == 1);
            return delta_distance;
        }
    };

    vector<Result> normal_results;
    vector<Result> rainbow_results;
    int depth;
    Cube cube;
    array<Move, 12> move_history;
    InnerRotationCounts inner_rotation_counts;

    // 有効な手筋かチェックする
    // 虹でも使えるなら 2
    // 通常だけなら 1
    // 使えないなら 0
    auto CheckValid() const {
        // 手筋の長さは 4 以上
        if (depth < 4)
            return 0;
        // 面以外の回転の戻し残しが無い
        if (inner_rotation_counts.distance_from_all_zero != 0)
            return 0;
        // 最後が面の回転でない (次の項目の下位互換なので無くても良い)
        if (move_history[depth - 1].IsFaceRotation<Cube::order>())
            return 0;
        // 面の回転の後には、別の軸の回転がある
        {
            auto last_face_roration_axis = (i8)-1;
            for (auto d = 1; d < depth; d++) {
                const auto mov = move_history[d];
                const auto axis = (i8)mov.GetAxis();
                if (mov.IsFaceRotation<Cube::order>())
                    last_face_roration_axis = axis;
                else if (axis != last_face_roration_axis)
                    last_face_roration_axis = (i8)-1;
            }
            if (last_face_roration_axis != (i8)-1)
                return 0;
        }
        // 面が揃っている
        static const auto reference_cube = [] {
            auto cube = Cube();
            cube.Reset();
            return cube;
        }();
        bool can_use_for_rainbow_cube = true;
        for (auto face_id = 0; face_id < 6; face_id++) {
            for (auto y = 1; y < Cube::order - 1; y++) {
                for (auto x = 1; x < Cube::order - 1; x++) {
                    const auto reference_color =
                        reference_cube.faces[face_id]
                            .GetIgnoringOrientation(y, x)
                            .data;
                    const auto color =
                        cube.faces[face_id].GetIgnoringOrientation(y, x).data;
                    if (reference_color != color) {
                        can_use_for_rainbow_cube = false;
                        if (reference_color / 4 != color / 4)
                            return 0;
                    }
                }
            }
        }
        return can_use_for_rainbow_cube ? 2 : 1;
    }
    // どの種類の回転に対しても適用される条件
    inline bool CanBeNextMove(const Move mov) const {
        if (depth == 0)
            return true;
        const auto last_mov = move_history[depth - 1];
        if (mov.GetAxis() != last_mov.GetAxis())
            return true;
        // 同じ軸の回転では、回転方向と回転箇所が昇順である
        if (mov.depth < last_mov.depth)
            return false; // 回転位置を昇順にする
        else if (mov.depth > last_mov.depth)
            return true; // 回転位置を昇順にする
        else if ((int)mov.direction == ((int)last_mov.direction ^ 1)) {
            return false; // 回転位置が同じかつ回転方向が逆ならだめ
        }
        assert(mov == last_mov);
        // 正の向きの回転が 3 回連続してはいけない
        if (last_mov.IsClockWise()) {
            if (depth >= 2) {
                const auto second_last_mov = move_history[depth - 2];
                if (second_last_mov == last_mov)
                    return false;
            }
            return true;
        } else
            return false; // 負の向きの回転が 2 回連続してはいけない
    };
    void Dfs() {
        // 方針: ややこしすぎるので後で回転とか言わず全部列挙する
        const auto valid = CheckValid();
        if (valid >= 1) {
            const auto moves = vector<Move>(move_history.begin(),
                                            move_history.begin() + depth);
            normal_results.push_back({Formula(moves)});
            if (valid == 2)
                rainbow_results.push_back({Formula(moves)});
        }
        if (depth == max_depth) {
            assert(inner_rotation_counts.distance_from_all_zero == 0);
            return;
        }

        // 面の回転 (初手以外)
        if (depth != 0 &&
            inner_rotation_counts.distance_from_all_zero < max_depth - depth) {
            for (const i8 i : {0, Cube::order - 1}) {
                for (auto direction = (i8)0; direction < 6; direction++) {
                    const auto mov = Move{(Move::Direction)direction, i};
                    if (!CanBeNextMove(mov))
                        continue;
                    const auto inv_mov = mov.Inv();
                    for (auto d = depth - 1;; d--) {
                        // 初手まで遡っても他の軸の回転が無いものはだめ
                        // 他の軸の回転を挟まずに戻すのはだめ
                        if (d < 0 || inv_mov == move_history[d])
                            goto next_face_move;
                        // 他の軸の回転が見つかれば良し
                        else if (mov.GetAxis() != move_history[d].GetAxis())
                            break;
                    }
                    move_history[depth] = mov;
                    cube.Rotate(mov);
                    depth++;
                    Dfs();
                    depth--;
                    cube.Rotate(inv_mov);
                next_face_move:;
                }
            }
        }

        // 面以外の回転
        const auto can_increase =
            inner_rotation_counts.distance_from_all_zero <
                max_depth - depth - 1 &&
            inner_rotation_counts.distance_from_all_zero < kMaxNInnerRotations;
        const auto can_decrease =
            depth >= 2 && inner_rotation_counts.distance_from_all_zero >= 1 &&
            inner_rotation_counts.distance_from_all_zero <= max_depth - depth;
        if (!can_increase && !can_decrease)
            return;
        for (auto i = (i8)1; i < Cube::order - 1; i++) {
            for (auto direction = (i8)0; direction < 6; direction++) {
                const auto mov = Move{(Move::Direction)direction, i};
                if (!CanBeNextMove(mov))
                    continue;
                const auto delta =
                    inner_rotation_counts.ComputeDeltaDistance(mov);
                if ((delta == 1 && !can_increase) ||
                    (delta == -1 && !can_decrease))
                    continue;
                inner_rotation_counts.Add(mov);
                const auto inv_mov = mov.Inv();
                move_history[depth] = mov;
                cube.Rotate(mov);
                depth++;
                Dfs();
                depth--;
                cube.Rotate(inv_mov);
                inner_rotation_counts.Add(inv_mov);
            }
        }
    }
    struct FacePhases {
        array<array<i8, 2>, 3> counts; // (axis, depth) -> count
        array<i8, 3> axis_distances;
        i8 distance_from_all_zero_except_one_axis;

        inline void Add(const Move mov) {
            assert(mov.IsFaceRotation<order>());
            const auto axis = (int)mov.GetAxis();
            auto& count = counts[axis][mov.depth != 0];
            axis_distances[axis] -= count == 3 ? 1 : count;
            count = (count + (mov.IsClockWise() ? 1 : -1)) & 3;
            axis_distances[axis] += count == 3 ? 1 : count;
            distance_from_all_zero_except_one_axis =
                axis_distances[0] + axis_distances[1] + axis_distances[2] -
                max(axis_distances[0],
                    max(axis_distances[1], axis_distances[2]));
        }
    };
    FacePhases face_phases;
    // 後で面以外の回転を追加することにし、ここでは面の回転だけ探索する
    inline void Dfs2() {
        if (depth != 0) {
            const auto moves = vector<Move>(move_history.begin(),
                                            move_history.begin() + depth);
            normal_results.push_back({Formula(moves)});
            rainbow_results.push_back({Formula(moves)});
        }
        if (depth == max_depth_2) {
            assert(face_phases.distance_from_all_zero_except_one_axis == 0);
            return;
        }

        // 面の回転
        for (const i8 i : {0, order - 1}) {
            for (auto direction = (i8)0; direction < 6; direction++) {
                const auto mov = Move{(Move::Direction)direction, i};
                if (!CanBeNextMove(mov))
                    continue;
                auto tmp_face_phases = face_phases;
                face_phases.Add(mov);
                if (face_phases.distance_from_all_zero_except_one_axis >=
                    max_depth_2 - depth) {
                    face_phases = tmp_face_phases;
                    continue;
                }
                move_history[depth++] = mov;
                Dfs2();
                depth--;
                face_phases = tmp_face_phases;
            }
        }
    }
    inline auto
    RemoveDuplicates(vector<Result>& results,
                     const bool remove_if_no_relative_changes) const {
        sort(results.begin(), results.end(),
             [](const Result& a, const Result& b) {
                 if (a.formula.Cost() != b.formula.Cost())
                     return a.formula.Cost() < b.formula.Cost();
                 return a.formula.moves < b.formula.moves;
             });
        auto top = 0;
        auto found_permutations =
            unordered_set<EdgeCube, typename EdgeCube::Hash>();
        // 本質的に同じものは、手数が最短なら重複を許容する
        auto found_relative_permutations =
            unordered_map<EdgeCube, int, typename EdgeCube::Hash>();

        auto ref_cube = EdgeCube();
        ref_cube.Reset();
        for (auto idx_results = 0; idx_results < (int)results.size();
             idx_results++) {
            const auto& result = results[idx_results];
            auto tmp_cube = ref_cube;
            tmp_cube.Rotate(result.formula);
            auto n_changes = 0;
            for (auto face_id = 0; face_id < 6; face_id++)
                for (auto edge_id = 0; edge_id < 4; edge_id++)
                    for (auto x = 0; x < order - 2; x++)
                        if (tmp_cube.faces[face_id].facelets[edge_id][x] !=
                            ref_cube.faces[face_id].facelets[edge_id][x])
                            n_changes++;
            if (n_changes == 0)
                continue;
            const auto [it, inserted] = found_permutations.insert(tmp_cube);
            if (!inserted)
                continue;
            assert(!tmp_cube.corner_parity); // 使わない
            // 面の回転だけ戻す
            for (auto i = (int)result.formula.moves.size() - 1; i >= 0; i--) {
                const auto mov = result.formula.moves[i];
                if (mov.template IsFaceRotation<order>())
                    tmp_cube.Rotate(mov.Inv());
            }
            auto n_relative_changes = 0;
            for (auto face_id = 0; face_id < 6; face_id++)
                for (auto edge_id = 0; edge_id < 4; edge_id++)
                    for (auto x = 0; x < order - 2; x++)
                        if (tmp_cube.faces[face_id].facelets[edge_id][x] !=
                            ref_cube.faces[face_id].facelets[edge_id][x])
                            n_relative_changes++;
            if (n_relative_changes * result.formula.Cost() > max_cost)
                continue;
            if (remove_if_no_relative_changes) {
                if (n_relative_changes == 0)
                    continue;
                const auto [it, inserted] = found_relative_permutations.insert(
                    {tmp_cube, result.formula.Cost()});
                if (it->second != result.formula.Cost())
                    continue;
            }
            results[top++] = result;
        }
        results.resize(top);
    }
    // rainbow にだけ適用して良く、
    // rainbow で生成されたものは normal でも使える
    inline void AugmentConjugate(vector<Result>& normal_results,
                                 vector<Result>& rainbow_results) const {
        const auto original_results_size = (int)rainbow_results.size();
        // rainbow_results.reserve(original_results_size * (1 + order * 6));
        // normal_results.reserve(normal_results.size() +
        //                        original_results_size * (order * 6));
        for (auto direction = (i8)0; direction < 6; direction++) {
            for (auto i = 0; i < order; i++) {
                const auto mov = Move{(Move::Direction)direction, (i8)i};
                const auto axis = (int)mov.GetAxis();
                const auto inv_mov = mov.Inv();
                for (auto idx_results = 0; idx_results < original_results_size;
                     idx_results++) {
                    auto result = rainbow_results[idx_results];
                    if (mov.IsFaceRotation<order>()) {
                        // 端の回転の場合は後ろに置かなくて良い
                        result.formula.moves.insert(
                            result.formula.moves.begin(), mov);
                    } else {
                        // 位相のずれが一切無いか、同じ軸のものしかないなら良い
                        auto face_phases = FacePhases();
                        for (const auto m : result.formula.moves)
                            if (m.template IsFaceRotation<order>())
                                face_phases.Add(m);
                        if (face_phases.axis_distances[(axis + 1) % 3] != 0 ||
                            face_phases.axis_distances[(axis + 2) % 3] != 0)
                            continue;
                        result.formula.moves.insert(
                            result.formula.moves.begin(), mov);
                        result.formula.moves.push_back(inv_mov);
                    }
                    rainbow_results.push_back(result);
                    normal_results.push_back(result);
                }
            }
        }
    }
};

template <int order>
static auto SearchEdgeFormulaWithOrder(const int max_cost, const int max_depth,
                                       const int max_depth_2,
                                       const int max_conjugate_depth) {
    auto searcher = EdgeFormulaSearcher<order>();
    const auto [normal_results, rainbow_results] =
        searcher.Search(max_cost, max_depth, max_depth_2, max_conjugate_depth);
    for (auto i = 0; i < 2; i++) {
        const auto name = i == 0 ? "normal" : "rainbow";
        const auto& results = i == 0 ? normal_results : rainbow_results;
        const auto filename =
            format("out/edge_formula_{}_{}_{}_{}_{}_{}.txt", name, order,
                   max_cost, max_depth, max_depth_2, max_conjugate_depth);
        auto ofs = ofstream(filename);
        if (!ofs.good()) {
            cerr << format("Cannot open file `{}`.", filename) << endl;
            abort();
        }
        ofs << "# Number of formulas: " << results.size() << endl;
        for (const auto& [formula] : results) {
            formula.Print(ofs);
            ofs << endl;
        }
    }
}

[[maybe_unused]] static auto
SearchEdgeFormula(const int order, const int max_cost = 90,
                  const int max_depth = 7, const int max_depth_2 = 7,
                  const int max_conjugate_depth = 6) {
    switch (order) {
    case 2:
        return SearchEdgeFormulaWithOrder<2>(max_cost, max_depth, max_depth_2,
                                             max_conjugate_depth);
    case 3:
        return SearchEdgeFormulaWithOrder<3>(max_cost, max_depth, max_depth_2,
                                             max_conjugate_depth);
    case 4:
        return SearchEdgeFormulaWithOrder<4>(max_cost, max_depth, max_depth_2,
                                             max_conjugate_depth);
    case 5:
        return SearchEdgeFormulaWithOrder<5>(max_cost, max_depth, max_depth_2,
                                             max_conjugate_depth);
    case 6:
        return SearchEdgeFormulaWithOrder<6>(max_cost, max_depth, max_depth_2,
                                             max_conjugate_depth);
    case 7:
        return SearchEdgeFormulaWithOrder<7>(max_cost, max_depth, max_depth_2,
                                             max_conjugate_depth);
    case 8:
        return SearchEdgeFormulaWithOrder<8>(max_cost, max_depth, max_depth_2,
                                             max_conjugate_depth);
    case 9:
        return SearchEdgeFormulaWithOrder<9>(max_cost, max_depth, max_depth_2,
                                             max_conjugate_depth);
    case 10:
        return SearchEdgeFormulaWithOrder<10>(max_cost, max_depth, max_depth_2,
                                              max_conjugate_depth);
    case 19:
        return SearchEdgeFormulaWithOrder<19>(max_cost, max_depth, max_depth_2,
                                              max_conjugate_depth);
    case 33:
        return SearchEdgeFormulaWithOrder<33>(max_cost, max_depth, max_depth_2,
                                              max_conjugate_depth);
    default:
        assert(false);
    }
}

int main(const int argc, const char* const* const argv) {
    switch (argc) {
    case 2:
        SearchEdgeFormula(atoi(argv[1]));
        break;
    case 3:
        SearchEdgeFormula(atoi(argv[1]), atoi(argv[2]));
        break;
    case 4:
        SearchEdgeFormula(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
        break;
    case 5:
        SearchEdgeFormula(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]),
                          atoi(argv[4]));
        break;
    case 6:
        SearchEdgeFormula(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]),
                          atoi(argv[4]), atoi(argv[5]));
        break;
    default:
        cout << "Usage: " << argv[0]
             << " <order> [<max_cost> [<max_depth> [<max_depth_2> "
                "[<max_conjugate_depth>]]]]"
             << endl;
    }
}

// clang++ -std=c++20 -Wall -Wextra -O3 search_edge_formula.cpp