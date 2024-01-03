#include "cube.cpp"
#include <algorithm>
#include <numeric>

using std::iota;
using std::sort;

struct EdgeFormulaSearcher {
    static constexpr auto kOrder = 5;
    static constexpr auto kMaxNInnerRotations = 3;

    using Cube = ::Cube<kOrder, ColorType24>;
    int max_depth;
    Cube start_cube;

    EdgeFormulaSearcher(const int max_depth)
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

    auto ComputeOriginalFaceletPosition(const int y, const int x,
                                        const ColorType24 color) const {
        // color / 4 で面が決まる
        // color % 4 で象限が決まる
        // y, x でその中の位置がきまる
        static constexpr auto table = []() {
            struct Pos {
                i8 y, x;
            };
            auto table = array<array<array<Pos, 4>, kOrder>, kOrder>();
            for (auto y = 0; y < kOrder / 2; y++)
                for (auto x = 0; x < (kOrder + 1) / 2; x++) {
                    for (auto [rotated_y, rotated_x] :
                         {array<int, 2>{y, x},
                          {x, kOrder - 1 - y},
                          {kOrder - 1 - y, kOrder - 1 - x},
                          {kOrder - 1 - x, y}}) {
                        table[rotated_y][rotated_x][0] = {(i8)y, (i8)x};
                        table[rotated_y][rotated_x][1] = {(i8)x,
                                                          (i8)(kOrder - 1 - y)};
                        table[rotated_y][rotated_x][3] = {(i8)(kOrder - 1 - y),
                                                          (i8)(kOrder - 1 - x)};
                        table[rotated_y][rotated_x][2] = {(i8)(kOrder - 1 - x),
                                                          (i8)(y)};
                        // 2 と 3 が逆なのは、
                        // ここでは左上->右上->右下->左下の順で代入しているため
                    }
                }
            if constexpr (kOrder % 2 == 1) {
                table[kOrder / 2][kOrder / 2][0] = {(i8)(kOrder / 2),
                                                    (i8)(kOrder / 2)};
                table[kOrder / 2][kOrder / 2][1] = {(i8)-100, (i8)-100};
                table[kOrder / 2][kOrder / 2][2] = {(i8)-100, (i8)-100};
                table[kOrder / 2][kOrder / 2][3] = {(i8)-100, (i8)-100};
            }
            return table;
        }();
        const auto t = table[y][x][color.data % 4];
        return FaceletPosition{(i8)(color.data / 4), t.y, t.x};
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
        // 面が揃っている
        for (auto face_id = 0; face_id < 5; face_id++) {
            const auto color =
                cube.faces[face_id].GetIgnoringOrientation(1, 1).data / 4;
            for (auto y = 1; y < Cube::order - 1; y++) {
                for (auto x = 1; x < Cube::order - 1; x++)
                    if (cube.faces[face_id].GetIgnoringOrientation(y, x).data /
                            4 !=
                        color)
                        return false;
            }
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
                        auto original_pos =
                            ComputeOriginalFaceletPosition(y, x, color);
                        if (pos != original_pos)
                            facelet_changes_array[n_facelet_changes++] = {
                                original_pos, pos};
                    }
                for (const auto x : {0, Cube::order - 1})
                    for (auto y = 1; y < Cube::order - 1; y++) {
                        const auto pos =
                            FaceletPosition{(i8)face_id, (i8)y, (i8)x};
                        const auto color = cube.Get(pos);
                        auto original_pos =
                            ComputeOriginalFaceletPosition(y, x, color);
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
        }
        if (depth == max_depth) {
            assert(n_inner_rotations == 0);
            return;
        }

        // TODO: IDDFS にして、ハッシュを使って手順前後みたいな重複を除去？

        // 面の回転 (初手以外)
        if (depth != 0 && n_inner_rotations < max_depth - depth) {
            for (const i8 i : {0, Cube::order - 1}) {
                for (auto direction = (i8)0; direction < 6; direction++) {
                    const auto mov = Move{(Move::Direction)direction, i};
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

void SearchEdgeFormula() {
    auto s = EdgeFormulaSearcher(5);
    const auto results = s.Search();
    cout << results.size() << endl;
    for (const auto& formula : results) {
        formula.Print();
        cout << "  " << formula.facelet_changes.size();
        if (0) {
            for (const auto [from, to] : formula.facelet_changes) {
                cout << " " << (int)from.face_id << (int)from.y << (int)from.x
                     << "->" << (int)to.face_id << (int)to.y << (int)to.x;
            }
        }
        cout << endl;
    }
}

int main() {
    SearchEdgeFormula();
    //
}

// clang++ -std=c++20 -Wall -Wextra -O3 search_edge_formula.cpp