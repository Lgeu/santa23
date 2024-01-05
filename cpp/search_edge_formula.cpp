#include "cube.cpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <numeric>

using std::format;
using std::ifstream;
using std::iota;
using std::ofstream;
using std::sort;

template <int order> struct EdgeFormulaSearcher {
    static constexpr auto kMaxNInnerRotations = 3;

    using Cube = ::Cube<order, ColorType24>;
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
        }
        if (depth == max_depth) {
            assert(n_inner_rotations == 0);
            return;
        }

        // TODO: IDDFS にして、ハッシュを使って手順前後みたいな重複を除去？
        // 現在は f2.d0.d0.-f2 と f2.-d0.-d0.-f2 のような実質同じ手筋がある

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

template <int order>
static auto SearchEdgeFormulaWithOrder(const int max_depth) {
    auto searcher = EdgeFormulaSearcher<order>(max_depth);
    const auto results = searcher.Search();
    auto os = ofstream(format("out/edge_formula_{}_{}.txt", order, max_depth));
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