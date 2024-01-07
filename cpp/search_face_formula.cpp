#include "cube.cpp"

#include <algorithm>
#include <numeric>

using std::clamp;
using std::iota;
using std::max_element;
using std::sort;

template <int order> struct FaceFormulaSearcher {
    static constexpr auto kMaxNInnerRotations = 3;

    using ColorType = ColorType24;
    using Cube = ::Cube<order, ColorType>;
    int max_depth;
    Cube start_cube;

    FaceFormulaSearcher(const int max_depth)
        : max_depth(max_depth), start_cube(), results(), depth(), cube(),
          move_history(), inner_rotations(), n_inner_rotations() {
        start_cube.Reset();
    }

    auto Search() {
        results.clear();
        cube = start_cube;
        Dfs();
        return results;
    }

    vector<Formula> results;
    int depth;                                        // Dfs(), CheckValid()
    Cube cube;                                        // Dfs(), CheckValid()
    array<Move, 8> move_history;                      // Dfs(), CheckValid()
    array<Move, kMaxNInnerRotations> inner_rotations; // Dfs()
    int n_inner_rotations;                            // Dfs(), CheckValid()

    // 有効な手筋かチェックする
    bool CheckValid() const {
        // 手筋の長さは 4 以上
        if (depth < 4)
            return false;
        // 面以外の回転の戻し残しが無い
        if (n_inner_rotations != 0)
            return false;
        // 最後が面の回転でない (次の項目の下位互換なので無くても良い)
        if (move_history[depth - 1].IsFaceRotation<Cube::order>())
            return false;
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
                return false;
        }
        // 面の揃っていないマスが2以上6以下
        // 最大数の調整
        // TODO 各面に異なる色を割り当てる
        {
            constexpr int n_facecub_diff_min = 1;
            constexpr int n_facecub_diff_max = 6;
            int n_facecube_diff = 0;
            for (auto face_id = 0; face_id <= 5; face_id++) {
                array<int, ColorType::kNColors> color_counts = {};
                for (auto y = 1; y < Cube::order - 1; y++) {
                    for (auto x = 1; x < Cube::order - 1; x++) {
                        const auto color = cube.faces[face_id]
                                               .GetIgnoringOrientation(y, x)
                                               .data /
                                           4;
                        color_counts[color]++;
                    }
                }
                // 最大色
                const auto color =
                    i8(max_element(color_counts.begin(), color_counts.end()) -
                       color_counts.begin());
                for (auto y = 1; y < Cube::order - 1; y++) {
                    for (auto x = 1; x < Cube::order - 1; x++)
                        if (cube.faces[face_id]
                                    .GetIgnoringOrientation(y, x)
                                    .data /
                                4 !=
                            color)
                            n_facecube_diff++;
                }
            }
            assert(n_facecube_diff != 1);
            if (clamp(n_facecube_diff, n_facecub_diff_min,
                      n_facecub_diff_max) != n_facecube_diff)
                return false;
        }
        return true;
    }
    void Dfs() {
        // 方針: ややこしすぎるので後で回転とか言わず全部列挙する
        if (CheckValid()) {
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
                results.emplace_back(moves, facelet_changes);
            }
            // valid ならば、それより深いところまで探索する必要は無い
            return;
        }
        if (depth == max_depth) {
            assert(n_inner_rotations == 0);
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
        if (depth != 0 && n_inner_rotations < max_depth - depth) {
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

        // 面以外の回転であって、面の変化を増やすもの
        if (n_inner_rotations < max_depth - depth - 1 &&
            n_inner_rotations < kMaxNInnerRotations) {
            for (auto i = (i8)1; i < Cube::order - 1; i++) {
                for (auto direction = (i8)0; direction < 6; direction++) {
                    const auto mov = Move{(Move::Direction)direction, i};
                    if (!can_be_next_move(mov))
                        continue;
                    // inner_rotations を減らすものは除外する
                    const auto inv_mov = mov.Inv();
                    for (auto idx_inner_rotations = 0;
                         idx_inner_rotations < n_inner_rotations;
                         idx_inner_rotations++) {
                        if (inner_rotations[idx_inner_rotations] == inv_mov)
                            goto next_incremental_inner_rotation_move;
                    }
                    inner_rotations[n_inner_rotations++] = mov;
                    move_history[depth] = mov;
                    cube.Rotate(mov);
                    depth++;
                    Dfs();
                    depth--;
                    cube.Rotate(inv_mov);
                    n_inner_rotations--;
                next_incremental_inner_rotation_move:;
                }
            }
        }

        // 面以外の回転であって、面の変化を戻すもの
        if (depth >= 2 && n_inner_rotations >= 1 &&
            n_inner_rotations <= max_depth - depth) {
            auto sorted_inner_rotations_indices =
                array<int, kMaxNInnerRotations>();
            iota(sorted_inner_rotations_indices.begin(),
                 sorted_inner_rotations_indices.end(), 0);
            sort(sorted_inner_rotations_indices.begin(),
                 sorted_inner_rotations_indices.begin() + n_inner_rotations,
                 [&](const int l, const int r) {
                     return inner_rotations[l] < inner_rotations[r];
                 });
            for (auto i = 0; i < n_inner_rotations; i++) {
                const auto idx_inner_rotations =
                    sorted_inner_rotations_indices[i];
                if (i >= 1) {
                    const auto last_idx_inner_rotations =
                        sorted_inner_rotations_indices[i - 1];
                    if (inner_rotations[idx_inner_rotations] ==
                        inner_rotations[last_idx_inner_rotations])
                        continue; // 重複を除去
                }
                const auto inv_mov = inner_rotations[idx_inner_rotations];
                const auto mov = inv_mov.Inv();
                if (!can_be_next_move(mov))
                    continue;
                for (auto d = depth - 1;; d--) {
                    assert(d >= 0);
                    // 他の軸の回転を挟まずに戻すのはだめ
                    if (inv_mov == move_history[d])
                        goto next_decremental_inner_rotation_move;
                    // 他の軸の回転が見つかれば良し
                    else if (mov.GetAxis() != move_history[d].GetAxis())
                        break;
                }
                inner_rotations[idx_inner_rotations] =
                    inner_rotations[--n_inner_rotations]; // 削除
                move_history[depth] = mov;
                cube.Rotate(mov);
                depth++;
                Dfs();
                depth--;
                cube.Rotate(inv_mov);
                inner_rotations[n_inner_rotations++] =
                    inner_rotations[idx_inner_rotations];
                inner_rotations[idx_inner_rotations] = inv_mov;
            next_decremental_inner_rotation_move:;
            }
        }
    }
};

template <int order>
static auto SearchFaceFormulaWithOrder(const int max_depth) {
    auto searcher = FaceFormulaSearcher<order>(max_depth);
    const auto results = searcher.Search();
    const auto filename =
        format("out/face_formula_{}_{}.txt", order, max_depth);
    auto os = ofstream(filename);
    if (!os.good()) {
        cerr << format("Cannot open file `{}`.", filename) << endl;
        abort();
    }
    os << "# Number of formulas: " << results.size() << endl;
    for (const auto& formula : results) {
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

[[maybe_unused]] static auto SearchFaceFormula(const int order,
                                               const int max_depth) {
    switch (order) {
    case 2:
        return SearchFaceFormulaWithOrder<2>(max_depth);
    case 3:
        return SearchFaceFormulaWithOrder<3>(max_depth);
    case 4:
        return SearchFaceFormulaWithOrder<4>(max_depth);
    case 5:
        return SearchFaceFormulaWithOrder<5>(max_depth);
    case 6:
        return SearchFaceFormulaWithOrder<6>(max_depth);
    case 7:
        return SearchFaceFormulaWithOrder<7>(max_depth);
    case 8:
        return SearchFaceFormulaWithOrder<8>(max_depth);
    case 9:
        return SearchFaceFormulaWithOrder<9>(max_depth);
    case 10:
        return SearchFaceFormulaWithOrder<10>(max_depth);
    case 19:
        return SearchFaceFormulaWithOrder<19>(max_depth);
    case 33:
        return SearchFaceFormulaWithOrder<33>(max_depth);
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

    SearchFaceFormula(order, max_depth);
}

// clang++ -std=c++20 -Wall -Wextra -O3 search_face_formula.cpp