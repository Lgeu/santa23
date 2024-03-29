#include "cube.cpp"

template <int order> struct EdgeFormulaSearcher {
    static constexpr auto kMaxNInnerRotations = 3;

    using Cube = ::Cube<order, ColorType24>;
    int max_depth;
    Cube start_cube;

    EdgeFormulaSearcher(const int max_depth)
        : max_depth(max_depth), start_cube(), results(), depth(), cube(),
          move_history(), inner_rotation_counts() {
        start_cube.Reset();
    }

    auto Search() {
        results.clear();
        cube = start_cube;
        Dfs();
        return results;
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

    struct Result {
        bool can_use_for_rainbow_cube;
        Formula formula;
    };

    vector<Result> results;
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
    void Dfs() {
        // 方針: ややこしすぎるので後で回転とか言わず全部列挙する
        const auto valid = CheckValid();
        if (valid >= 1) {
            const auto moves = vector<Move>(move_history.begin(),
                                            move_history.begin() + depth);
            auto facelet_changes_array =
                array<Formula::FaceletChange, (Cube::order - 2) * 24>();
            auto n_facelet_changes = 0;
            for (auto face_id = 0; face_id < 6; face_id++) {
                for (const auto y : {0, Cube::order - 1})
                    for (auto x = 1; x < Cube::order - 1; x++) {
                        const auto pos =
                            FaceletPosition{(i8)face_id, (i8)y, (i8)x};
                        const auto color = cube.Get(pos);
                        const auto original_pos =
                            Cube::ComputeOriginalFaceletPosition(y, x, color);
                        if (pos != original_pos)
                            facelet_changes_array[n_facelet_changes++] = {
                                original_pos, pos};
                    }
                for (const auto x : {0, Cube::order - 1})
                    for (auto y = 1; y < Cube::order - 1; y++) {
                        const auto pos =
                            FaceletPosition{(i8)face_id, (i8)y, (i8)x};
                        const auto color = cube.Get(pos);
                        const auto original_pos =
                            Cube::ComputeOriginalFaceletPosition(y, x, color);
                        if (pos != original_pos)
                            facelet_changes_array[n_facelet_changes++] = {
                                original_pos, pos};
                    }
            }
            if (n_facelet_changes != 0) {
                // cout << n_facelet_changes << endl;
                const auto facelet_changes = vector<Formula::FaceletChange>(
                    facelet_changes_array.begin(),
                    facelet_changes_array.begin() + n_facelet_changes);
                results.push_back({valid == 2, {moves, facelet_changes}});
            }
        }
        if (depth == max_depth) {
            assert(inner_rotation_counts.distance_from_all_zero == 0);
            return;
        }

        // TODO: IDDFS にして、ハッシュを使って手順前後みたいな重複を除去？

        // どの種類の回転に対しても適用される条件
        const auto can_be_next_move = [this](const Move mov) {
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

        // 面の回転 (初手以外)
        if (depth != 0 &&
            inner_rotation_counts.distance_from_all_zero < max_depth - depth) {
            for (const i8 i : {0, Cube::order - 1}) {
                for (auto direction = (i8)0; direction < 6; direction++) {
                    const auto mov = Move{(Move::Direction)direction, i};
                    if (!can_be_next_move(mov))
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
                if (!can_be_next_move(mov))
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
};

template <int order>
static auto SearchEdgeFormulaWithOrder(const int max_depth) {
    auto searcher = EdgeFormulaSearcher<order>(max_depth);
    const auto results = searcher.Search();
    const auto filename =
        format("out/edge_formula_{}_{}.txt", order, max_depth);
    auto os = ofstream(filename);
    if (!os.good()) {
        cerr << format("Cannot open file `{}`.", filename) << endl;
        abort();
    }
    os << "# Number of formulas: " << results.size() << endl;
    for (const auto& [can_use_for_rainbow_cube, formula] : results) {
        os << can_use_for_rainbow_cube << " ";
        formula.Print(os);
        // os << "  " << formula.facelet_changes.size();
        if (0) {
            for (const auto [from, to] : formula.facelet_changes)
                os << format(" {}{}{}->{}{}{}", (int)from.face_id, (int)from.y,
                             (int)from.x, (int)to.face_id, (int)to.y,
                             (int)to.x);
        }
        os << endl;
    }
}

[[maybe_unused]] static auto SearchEdgeFormula(const int order,
                                               const int max_depth) {
    switch (order) {
    case 2:
        return SearchEdgeFormulaWithOrder<2>(max_depth);
    case 3:
        return SearchEdgeFormulaWithOrder<3>(max_depth);
    case 4:
        return SearchEdgeFormulaWithOrder<4>(max_depth);
    case 5:
        return SearchEdgeFormulaWithOrder<5>(max_depth);
    case 6:
        return SearchEdgeFormulaWithOrder<6>(max_depth);
    case 7:
        return SearchEdgeFormulaWithOrder<7>(max_depth);
    case 8:
        return SearchEdgeFormulaWithOrder<8>(max_depth);
    case 9:
        return SearchEdgeFormulaWithOrder<9>(max_depth);
    case 10:
        return SearchEdgeFormulaWithOrder<10>(max_depth);
    case 19:
        return SearchEdgeFormulaWithOrder<19>(max_depth);
    case 33:
        return SearchEdgeFormulaWithOrder<33>(max_depth);
    default:
        assert(false);
    }
}

int main(const int argc, const char* const* const argv) {
    if (argc < 3) {
        cout << "Usage: " << argv[0] << " <order> <max_depth>" << endl;
        return 1;
    }
    const auto order = atoi(argv[1]);
    const auto max_depth = atoi(argv[2]);

    cout << "Order: " << order << endl;
    cout << "Max Depth: " << max_depth << endl;

    SearchEdgeFormula(order, max_depth);
}

// clang++ -std=c++20 -Wall -Wextra -O3 search_edge_formula.cpp