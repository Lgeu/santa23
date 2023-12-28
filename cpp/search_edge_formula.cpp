#include "cube.cpp"
#include <algorithm>
#include <numeric>
#include <type_traits>

using std::iota;
using std::is_same_v;
using std::sort;

struct EdgeFormulaSearcher {
    static constexpr auto kOrder = 5;
    static constexpr auto kMaxNInnerRotations = 3;

    using Cube = ::Cube<kOrder>;
    int max_depth;
    vector<Formula> results;

    EdgeFormulaSearcher(const int max_depth) : max_depth(max_depth) {}

    auto Search() {
        results.clear();
        Dfs();
        return results;
    }

    Cube cube;                                        // Dfs(), CheckValid()
    array<Move, 8> move_history;                      // Dfs(), CheckValid()
    array<Move, kMaxNInnerRotations> inner_rotations; // Dfs()
    int n_inner_rotations;                            // Dfs(), CheckValid()
    // cube の面が揃っているかチェックする
    bool CheckValid() const {
        // 手筋の長さは 4 以上
        if ((int)move_history.size() < 4)
            return false;
        // 最後が面の回転でない
        if (move_history.back().depth == 0 ||
            move_history.back().depth == Cube::order - 1)
            return false;
        // 面以外の回転の戻し残しが無い
        if (n_inner_rotations != 0)
            return false;
        for (auto face_id = 0; face_id < 5; face_id++) {
            const auto color = cube.faces[face_id].GetIgnoringOrientation(1, 1);
            for (auto y = 1; y < Cube::order - 1; y++) {
                for (auto x = 1; x < Cube::order - 1; x++)
                    if (cube.faces[face_id].GetIgnoringOrientation(y, x) !=
                        color)
                        return false;
            }
        }
        return true;
    }
    void Dfs(const int depth = 0) {
        // 方針: ややこしすぎるので後で回転とか言わず全部列挙する
        if (CheckValid()) {
            // TODO: 変更箇所と変化量を確認
            const auto moves = vector<Move>(move_history.begin(),
                                            move_history.begin() + depth);
            auto facelet_changes_array =
                array<Formula::FaceletChange, (Cube::order - 2) * 24>();
            auto n_facelet_changes = 0;
            for (auto face_id = 0; face_id < 6; face_id++) {
                for (const auto y : {0, Cube::order - 1}) {
                    for (auto x = 1; x < Cube::order - 1; x++) {
                        static_assert(is_same_v<Cube::ColorType, ColorType6>);
                        cube.Get(face_id, y, x);
                    }
                }
                for ()
            }

            auto facelet_changes = vector<Formula::FaceletChange>{};
            results.emplace_back(moves, facelet_changes);
        }
        if (depth == max_depth) {
            assert(n_inner_rotations == 0);
            return;
        }

        // TODO: 操作を即戻すのを避ける

        // 面の回転 (初手以外)
        if (depth != 0 && n_inner_rotations < max_depth - depth) {
            for (const i8 i : {0, Cube::order - 1}) {
                for (auto direction = (i8)0; direction < 6; direction++) {
                    const auto mov = Move{(Move::Direction)direction, i};
                    const auto inv_mov = mov.Inv();
                    move_history[depth] = mov;
                    cube.Rotate(mov);
                    Dfs(depth + 1);
                    cube.Rotate(inv_mov);
                }
                //
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
                            goto next_move;
                    }
                    inner_rotations[n_inner_rotations++] = mov;
                    move_history[depth] = mov;
                    cube.Rotate(mov);
                    Dfs(depth + 1);
                    cube.Rotate(inv_mov);
                    n_inner_rotations--;
                next_move:;
                }
            }
        }

        // 面以外の回転であって、面の変化を戻すもの
        if (n_inner_rotations >= 1 && n_inner_rotations <= max_depth - depth) {
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
                        continue;
                }
                const auto inv_mov = inner_rotations[idx_inner_rotations];
                const auto mov = inv_mov.Inv();
                inner_rotations[idx_inner_rotations] =
                    inner_rotations[--n_inner_rotations]; // 削除
                move_history[depth] = mov;
                cube.Rotate(mov);
                Dfs(depth + 1);
                cube.Rotate(inv_mov);
                inner_rotations[n_inner_rotations++] =
                    inner_rotations[idx_inner_rotations];
                inner_rotations[idx_inner_rotations] = inv_mov;
            }
        }
    }
};

void SearchEdgeFormula() {
    //
}