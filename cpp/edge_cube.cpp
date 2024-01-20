#include "cube.cpp"

using std::min;

// 辺のための色
struct ColorType48 {
    static constexpr auto kNColors = 48;
    i8 data;

    inline auto operator<=>(const ColorType48&) const = default;

    inline void Print(ostream& os) const {
        if (data < 0)
            os << ' ';
        else if (data < 24)
            os << (char)('A' + data);
        else
            os << (char)('a' - 24 + data);
    }

    inline void Display(ostream& os) const {
        // 青、赤、白、橙 (マゼンタ)、黄、緑
        static constexpr auto kColors = array<const char*, 6>{
            "\e[44m", "\e[41m", "\e[47m", "\e[45m", "\e[43m", "\e[42m",
        };
        if (data >= 0)
            os << kColors[data / 8];
        Print(os);
        if (data >= 0)
            os << "\e[0m";
    }

    inline void PrintSingmaster(ostream& os) const {
        if (data < 0)
            os << ' ';
        else
            os << "UFRBLD"[data / 8];
    }

    template <Cubeish CubeType>
    inline FaceletPosition ComputeOriginalFaceletPosition(const int y,
                                                          const int x) const {
        const auto as_color_type_24 =
            ColorType24{(i8)(((data & 0b111000) + (data + 1 & 0b111)) >> 1)};
        return ::Cube<CubeType::order, ColorType24>::
            ComputeOriginalFaceletPosition(y, x, as_color_type_24);
    }

    friend auto& operator<<(ostream& os, const ColorType48 t) {
        t.Print(os);
        return os;
    }
};

// 辺だけのキューブの 1 つの面
// 方針: ビームサーチしやすい感じにする
template <int order, typename ColorType_ = ColorType48> struct EdgeFace {
    using ColorType = ColorType_;

    // 辺を揃える上では 6 も 24 も同じ問題になる
    // 1 面を 8 つに分けると良さそう
    static_assert(is_same_v<ColorType, ColorType48>);

    array<array<ColorType, order - 2>, 4> facelets; // マス

    // 0 で初期化
    inline EdgeFace() : facelets() {}

    // 1 色で埋める
    void Fill(const ColorType color) {
        for (auto edge_id = 0; edge_id < 4; edge_id++)
            for (auto x = 0; x < order - 2; x++)
                facelets[edge_id][x] = color;
    }

    // ColorType に応じて初期化
    void Reset(const ColorType start_color) {
        assert(start_color.data % 4 == 0);
        for (auto edge_id = 0; edge_id < 4; edge_id++)
            for (auto x = 1; x < order - 1; x++) {
                //  AAB
                // H   C
                // G   C
                // G   D
                //  FEE
                const auto color =
                    ColorType{(i8)(start_color.data + edge_id * 2 +
                                   (int)(x >= (order + 1) / 2))};
                facelets[edge_id][x - 1] = color;
            }
    }

    // 時計回りに step * 90 度回転
    inline void RotateCW(const int step) {
        switch ((unsigned)step % 4) {
        case 0:
            return;
        case 1: {
            const auto tmp = facelets[3];
            facelets[3] = facelets[2];
            facelets[2] = facelets[1];
            facelets[1] = facelets[0];
            facelets[0] = tmp;
            return;
        }
        case 2:
            swap(facelets[0], facelets[2]);
            swap(facelets[1], facelets[3]);
            return;
        case 3: {
            const auto tmp = facelets[0];
            facelets[0] = facelets[1];
            facelets[1] = facelets[2];
            facelets[2] = facelets[3];
            facelets[3] = tmp;
            return;
        }
        }
    }

    inline ColorType Get(const int y, const int x) const {
        if (y == 0) { // 上
            assert(1 <= x && x < order - 1);
            return facelets[0][x - 1];
        } else if (y == order - 1) { // 下
            assert(1 <= x && x < order - 1);
            return facelets[2][order - 2 - x];
        } else if (x == 0) { // 左
            assert(1 <= y && y < order - 1);
            return facelets[3][order - 2 - y];
        } else if (x == order - 1) { // 右
            assert(1 <= y && y < order - 1);
            return facelets[1][y - 1];
        } else {
            assert(false);
            return {};
        }
    }

    inline ColorType TryGet(const int y, const int x) const {
        if ((y == 0 || y == order - 1) == (x == 0 || x == order - 1))
            return {(i8)-1};
        return Get(y, x);
    }

    inline void Set(const int y, const int x, const ColorType color) {
        if (y == 0) { // 上
            assert(1 <= x && x < order - 1);
            facelets[0][x - 1] = color;
        } else if (y == order - 1) { // 下
            assert(1 <= x && x < order - 1);
            facelets[2][order - 2 - x] = color;
        } else if (x == 0) { // 左
            assert(1 <= y && y < order - 1);
            facelets[3][order - 2 - y] = color;
        } else if (x == order - 1) { // 右
            assert(1 <= y && y < order - 1);
            facelets[1][y - 1] = color;
        } else {
            assert(false);
        }
    }

    inline void TrySet(const int y, const int x, ColorType color) {
        if ((y == 0 || y == order - 1) == (x == 0 || x == order - 1))
            return;
        Set(y, x, color);
    }
};

template <int order_, typename ColorType_ = ColorType48> struct EdgeCube {
    static constexpr auto order = order_;
    using ColorType = ColorType_;
    enum { D1, F0, R0, F1, R1, D0 };

    static_assert(is_same_v<ColorType, ColorType48>);

    array<EdgeFace<order, ColorType>, 6> faces;
    bool corner_parity;

    inline void Reset() {
        for (auto face_id = 0; face_id < 6; face_id++)
            faces[face_id].Reset(ColorType{(i8)(face_id * 8)});
        corner_parity = false;
    }

    inline void Rotate(const Move& mov) {
#define FROM_BOTTOM order - 1 - mov.depth, i
#define FROM_LEFT i, mov.depth
#define FROM_TOP mov.depth, order - 1 - i
#define FROM_RIGHT order - 1 - i, order - 1 - mov.depth
        const auto is_face_rotation = mov.IsFaceRotation<order>();
        if constexpr (order % 2 == 0)
            if (is_face_rotation)
                corner_parity = !corner_parity;
        switch (mov.direction) {
        case Move::Direction::F:
            if (mov.depth == 0)
                faces[F0].RotateCW(1);
            else if (mov.depth == order - 1)
                faces[F1].RotateCW(-1);
            for (auto i = (int)is_face_rotation;
                 i < order - (int)is_face_rotation;
                 i += (is_face_rotation ? 1 : order - 1)) {
                // clang-format off
                const auto tmp =           faces[R1].Get(FROM_RIGHT);
                faces[R1].Set(FROM_RIGHT,  faces[D0].Get(FROM_TOP));
                faces[D0].Set(FROM_TOP,    faces[R0].Get(FROM_LEFT));
                faces[R0].Set(FROM_LEFT,   faces[D1].Get(FROM_BOTTOM));
                faces[D1].Set(FROM_BOTTOM, tmp);
                // clang-format on
            }
            break;
        case Move::Direction::Fp:
            if (mov.depth == 0)
                faces[F0].RotateCW(-1);
            else if (mov.depth == order - 1)
                faces[F1].RotateCW(1);
            for (auto i = (int)is_face_rotation;
                 i < order - (int)is_face_rotation;
                 i += (is_face_rotation ? 1 : order - 1)) {
                // clang-format off
                const auto tmp =           faces[D1].Get(FROM_BOTTOM);
                faces[D1].Set(FROM_BOTTOM, faces[R0].Get(FROM_LEFT));
                faces[R0].Set(FROM_LEFT,   faces[D0].Get(FROM_TOP));
                faces[D0].Set(FROM_TOP,    faces[R1].Get(FROM_RIGHT));
                faces[R1].Set(FROM_RIGHT,  tmp);
                // clang-format on
            }
            break;
        case Move::Direction::D:
            if (mov.depth == 0)
                faces[D0].RotateCW(1);
            else if (mov.depth == order - 1)
                faces[D1].RotateCW(-1);
            for (auto i = (int)is_face_rotation;
                 i < order - (int)is_face_rotation;
                 i += (is_face_rotation ? 1 : order - 1)) {
                // clang-format off
                const auto tmp =           faces[R1].Get(FROM_BOTTOM);
                faces[R1].Set(FROM_BOTTOM, faces[F1].Get(FROM_BOTTOM));
                faces[F1].Set(FROM_BOTTOM, faces[R0].Get(FROM_BOTTOM));
                faces[R0].Set(FROM_BOTTOM, faces[F0].Get(FROM_BOTTOM));
                faces[F0].Set(FROM_BOTTOM, tmp);
                // clang-format on
            }
            break;
        case Move::Direction::Dp:
            if (mov.depth == 0)
                faces[D0].RotateCW(-1);
            else if (mov.depth == order - 1)
                faces[D1].RotateCW(1);
            for (auto i = (int)is_face_rotation;
                 i < order - (int)is_face_rotation;
                 i += (is_face_rotation ? 1 : order - 1)) {
                // clang-format off
                const auto tmp =           faces[F0].Get(FROM_BOTTOM);
                faces[F0].Set(FROM_BOTTOM, faces[R0].Get(FROM_BOTTOM));
                faces[R0].Set(FROM_BOTTOM, faces[F1].Get(FROM_BOTTOM));
                faces[F1].Set(FROM_BOTTOM, faces[R1].Get(FROM_BOTTOM));
                faces[R1].Set(FROM_BOTTOM, tmp);
                // clang-format on
            }
            break;
        case Move::Direction::R:
            if (mov.depth == 0)
                faces[R0].RotateCW(1);
            else if (mov.depth == order - 1)
                faces[R1].RotateCW(-1);
            for (auto i = (int)is_face_rotation;
                 i < order - (int)is_face_rotation;
                 i += (is_face_rotation ? 1 : order - 1)) {
                // clang-format off
                const auto tmp =           faces[F0].Get(FROM_RIGHT);
                faces[F0].Set(FROM_RIGHT,  faces[D0].Get(FROM_RIGHT));
                faces[D0].Set(FROM_RIGHT,  faces[F1].Get(FROM_LEFT));
                faces[F1].Set(FROM_LEFT,   faces[D1].Get(FROM_RIGHT));
                faces[D1].Set(FROM_RIGHT,  tmp);
                // clang-format on
            }
            break;
        case Move::Direction::Rp:
            if (mov.depth == 0)
                faces[R0].RotateCW(-1);
            else if (mov.depth == order - 1)
                faces[R1].RotateCW(1);
            for (auto i = (int)is_face_rotation;
                 i < order - (int)is_face_rotation;
                 i += (is_face_rotation ? 1 : order - 1)) {
                // clang-format off
                const auto tmp =           faces[D1].Get(FROM_RIGHT);
                faces[D1].Set(FROM_RIGHT,  faces[F1].Get(FROM_LEFT));
                faces[F1].Set(FROM_LEFT,   faces[D0].Get(FROM_RIGHT));
                faces[D0].Set(FROM_RIGHT,  faces[F0].Get(FROM_RIGHT));
                faces[F0].Set(FROM_RIGHT,  tmp);
                // clang-format on
            }
            break;
        }
#undef FROM_BOTTOM
#undef FROM_LEFT
#undef FROM_TOP
#undef FROM_RIGHT
    }

    inline void Rotate(const Formula& formula) {
        if (formula.use_facelet_changes) {
            array<ColorType, 6 * 4 * (order - 2)> new_colors;
            for (auto i = 0; i < (int)formula.facelet_changes.size(); i++)
                new_colors[i] = Get(formula.facelet_changes[i].from);
            for (auto i = 0; i < (int)formula.facelet_changes.size(); i++)
                Set(formula.facelet_changes[i].to, new_colors[i]);
            if constexpr (order % 2 == 0)
                for (const auto& mov : formula.moves)
                    if (mov.IsFaceRotation<order>())
                        corner_parity = !corner_parity;
        } else
            for (const auto& m : formula.moves)
                Rotate(m);
    }

    inline static constexpr auto AllFaceletPositions() {
        array<FaceletPosition, 6 * 4 * (order - 2)> facelet_positions;
        auto i = 0;
        for (auto face_id = 0; face_id < 6; face_id++) {
            for (auto x = 1; x < order - 1; x++)
                facelet_positions[i++] = {(i8)face_id, (i8)0, (i8)x};
            for (auto y = 1; y < order - 1; y++)
                facelet_positions[i++] = {(i8)face_id, (i8)y, (i8)(order - 1)};
            for (auto x = order - 2; x >= 1; x--)
                facelet_positions[i++] = {(i8)face_id, (i8)(order - 1), (i8)x};
            for (auto y = order - 2; y >= 1; y--)
                facelet_positions[i++] = {(i8)face_id, (i8)y, (i8)0};
        }
        assert(i == (int)facelet_positions.size());
        return facelet_positions;
    }

    inline auto Get(const int face_id, const int y, const int x) const {
        return faces[face_id].Get(y, x);
    }

    inline auto Get(const FaceletPosition& facelet_position) const {
        return Get(facelet_position.face_id, facelet_position.y,
                   facelet_position.x);
    }

    inline auto Set(const int face_id, const int y, const int x,
                    ColorType color) {
        faces[face_id].Set(y, x, color);
    }

    inline void Set(const FaceletPosition& facelet_position, ColorType color) {
        Set(facelet_position.face_id, facelet_position.y, facelet_position.x,
            color);
    }

    static FaceletPosition
    ComputeOriginalFaceletPosition(const int y, const int x,
                                   const ColorType color) {
        return ::Cube<order, ColorType>::ComputeOriginalFaceletPosition(y, x,
                                                                        color);
    }

    inline auto ComputeParities() const {
        auto result = array<bool, (order - 2) / 2>();
        for (auto i = 0; i < (order - 2) / 2; i++) {
            auto positions = array<int, 48>();
            for (auto face_id = 0; face_id < 6; face_id++)
                for (auto edge_id = 0; edge_id < 4; edge_id++) {
                    positions[face_id * 8 + edge_id * 2] =
                        faces[face_id].facelets[edge_id][i].data;
                    positions[face_id * 8 + edge_id * 2 + 1] =
                        faces[face_id].facelets[edge_id][order - 3 - i].data;
                }
            auto n_swapped = 0;
            for (auto i = 0; i < 48; i++) {
                auto p = positions[i];
                while (i != p) {
                    std::swap(positions[i], positions[p]);
                    n_swapped++;
                    p = positions[i];
                }
            }
            assert(n_swapped % 2 == 0);
            result[i] = (n_swapped / 2) % 2;
        }
        return result;
    }

    inline auto ComputeParityResolvingFormula(
        const array<bool, (order - 2) / 2>& parities) const {
        static constexpr auto base_moves = {
            Move{Move::Direction::R, 4},  // 3L'
            Move{Move::Direction::Dp, 6}, // U2
            Move{Move::Direction::Dp, 6}, //
            Move{Move::Direction::R, 4},  // 3L'
            Move{Move::Direction::Dp, 6}, // U2
            Move{Move::Direction::Dp, 6}, //
            Move{Move::Direction::F, 0},  // F2
            Move{Move::Direction::F, 0},  //
            Move{Move::Direction::R, 4},  // 3L'
            Move{Move::Direction::F, 0},  // F2
            Move{Move::Direction::F, 0},  //
            Move{Move::Direction::R, 2},  // 3R
            Move{Move::Direction::Dp, 6}, // U2
            Move{Move::Direction::Dp, 6}, //
            Move{Move::Direction::Rp, 2}, // 3R'
            Move{Move::Direction::Dp, 6}, // U2
            Move{Move::Direction::Dp, 6}, //
            Move{Move::Direction::Rp, 4}, // 3L2
            Move{Move::Direction::Rp, 4}, //
        };
        for (const auto& p : parities)
            if (p)
                goto not_all_zero;
        return Formula();
    not_all_zero:
        auto moves = vector<Move>();
        for (const auto mov : base_moves) {
            switch (mov.depth) {
            case 0:
                moves.push_back(mov);
                break;
            case 6:
                moves.push_back({mov.direction, (i8)(order - 1)});
                break;
            case 2:
                for (auto i = 0; i < (order - 2) / 2; i++)
                    if (parities[i])
                        moves.push_back({mov.direction, (i8)(i + 1)});
                break;
            case 4:
                for (auto i = 0; i < (order - 2) / 2; i++)
                    if (parities[i])
                        moves.push_back({mov.direction, (i8)(order - 2 - i)});
                break;
            }
        }
        return Formula(moves);
    }

    inline auto ComputePLLParity() const
        requires(order % 2 == 0)
    {
        static constexpr auto kCL = (order - 4) / 2;
        static constexpr auto kCR = (order - 2) / 2;
        static constexpr auto adjecent_edge_facelets =
            array<array<array<i8, 2>, 2>, 12>{
                array<array<i8, 2>, 2>{array<i8, 2>{D1, 0}, {F1, 0}},
                {array<i8, 2>{D1, 1}, {R0, 0}},
                {array<i8, 2>{D1, 2}, {F0, 0}},
                {array<i8, 2>{D1, 3}, {R1, 0}},
                {array<i8, 2>{D0, 0}, {F0, 2}},
                {array<i8, 2>{D0, 1}, {R0, 2}},
                {array<i8, 2>{D0, 2}, {F1, 2}},
                {array<i8, 2>{D0, 3}, {R1, 2}},
                {array<i8, 2>{F0, 1}, {R0, 3}},
                {array<i8, 2>{F0, 3}, {R1, 1}},
                {array<i8, 2>{F1, 1}, {R1, 3}},
                {array<i8, 2>{F1, 3}, {R0, 1}},
            };
        static const auto color_to_edge_position = [] {
            auto cube = EdgeCube();
            cube.Reset();
            auto result = array<int, 48>();
            for (auto i = 0; i < 12; i++) {
                const auto [facelet0, facelet1] = adjecent_edge_facelets[i];
                const auto [edge_id_0, x_0] = facelet0;
                const auto [edge_id_1, x_1] = facelet1;
                const auto color0l =
                    cube.faces[edge_id_0].facelets[x_0][kCL].data;
                const auto color0r =
                    cube.faces[edge_id_0].facelets[x_0][kCR].data;
                const auto color1l =
                    cube.faces[edge_id_1].facelets[x_1][kCL].data;
                const auto color1r =
                    cube.faces[edge_id_1].facelets[x_1][kCR].data;
                result[color0l] = i;
                result[color0r] = i;
                result[color1l] = i;
                result[color1r] = i;
            }
            return result;
        }();
        // 置換を計算する
        array<int, 12> positions;
        for (auto i = 0; i < 12; i++) {
            const auto [facelet, _] = adjecent_edge_facelets[i];
            const auto [edge_id, x] = facelet;
            const auto color0 = faces[edge_id].facelets[x][kCL].data;
            positions[i] = color_to_edge_position[color0];
        }
        // パリティ以前に、同じ色がある場合は true を返す
        auto tmp_positions = positions;
        sort(tmp_positions.begin(), tmp_positions.end());
        for (auto i = 0; i < 12; i++)
            if (tmp_positions[i] != i)
                return true;
        // パリティを計算する
        auto parity = corner_parity;
        for (auto i = 0; i < 12; i++) {
            auto p = positions[i];
            while (i != p) {
                std::swap(positions[i], positions[p]);
                parity = !parity;
                p = positions[i];
            }
        }
        return parity;
    }

    // 各辺で中央のマスと一致していない数を数える
    inline auto ComputeEdgeScore() const {
        // あれ、1 面 4 色でよかったんじゃ……
        auto score = 0;
        for (auto face_id = 0; face_id < 6; face_id++)
            for (auto edge_id = 0; edge_id < 4; edge_id++)
                for (auto x = 0; x < order - 2; x++)
                    // これ偶数キューブでも動くのか？
                    score +=
                        faces[face_id].facelets[edge_id][x].data / 2 !=
                        faces[face_id].facelets[edge_id][(order - 3) / 2].data /
                            2;
        if constexpr (order % 2 == 0)
            score += ComputePLLParity() * 100;
        return score;
    }

    // 異なる面の隣接するマスを取得する
    inline static FaceletPosition
    GetAdjacentPosition(const FaceletPosition& pos) {
        switch (pos.face_id) {
        case D1:
            if (pos.y == 0)
                return {F1, 0, (i8)(order - 1 - pos.x)};
            else if (pos.y == order - 1)
                return {F0, 0, pos.x};
            else if (pos.x == 0)
                return {R1, 0, pos.y};
            else if (pos.x == order - 1)
                return {R0, 0, (i8)(order - 1 - pos.y)};
            else
                assert(false);
        case F0:
            if (pos.y == 0)
                return {D1, (i8)(order - 1), pos.x};
            else if (pos.y == order - 1)
                return {D0, 0, pos.x};
            else if (pos.x == 0)
                return {R1, pos.y, (i8)(order - 1)};
            else if (pos.x == order - 1)
                return {R0, pos.y, 0};
            else
                assert(false);
        case R0:
            if (pos.y == 0)
                return {D1, (i8)(order - 1 - pos.x), (i8)(order - 1)};
            else if (pos.y == order - 1)
                return {D0, pos.x, (i8)(order - 1)};
            else if (pos.x == 0)
                return {F0, pos.y, (i8)(order - 1)};
            else if (pos.x == order - 1)
                return {F1, pos.y, 0};
            else
                assert(false);
        case F1:
            if (pos.y == 0)
                return {D1, 0, (i8)(order - 1 - pos.x)};
            else if (pos.y == order - 1)
                return {D0, (i8)(order - 1), (i8)(order - 1 - pos.x)};
            else if (pos.x == 0)
                return {R0, pos.y, (i8)(order - 1)};
            else if (pos.x == order - 1)
                return {R1, pos.y, 0};
            else
                assert(false);
        case R1:
            if (pos.y == 0)
                return {D1, pos.x, 0};
            else if (pos.y == order - 1)
                return {D0, (i8)(order - 1 - pos.x), 0};
            else if (pos.x == 0)
                return {F1, pos.y, (i8)(order - 1)};
            else if (pos.x == order - 1)
                return {F0, pos.y, 0};
            else
                assert(false);
        case D0:
            if (pos.y == 0)
                return {F0, (i8)(order - 1), pos.x};
            else if (pos.y == order - 1)
                return {F1, (i8)(order - 1), (i8)(order - 1 - pos.x)};
            else if (pos.x == 0)
                return {R1, (i8)(order - 1), (i8)(order - 1 - pos.y)};
            else if (pos.x == order - 1)
                return {R0, (i8)(order - 1), pos.y};
            else
                assert(false);
        }
        return {};
    }

    // Kaggle フォーマットを読み取る
    inline void Read(const string& s) {
        assert(s.size() >= 6 * order * order * 2 - 1);
        auto i = 0;
        for (auto face_id = 0; face_id < 6; face_id++)
            for (auto y = 0; y < order; y++)
                for (auto x = 0; x < order; x++) {
                    TrySet(face_id, y, x, {s[i] - 'A'});
                    i += 2;
                }
    }

    // 注: corner_parity は変更されない
    // 面倒すぎる
    inline void FromCube(const Cube<order, ColorType6>& rhs) {
        static auto color6_and_position_to_color48 = [] {
            auto result = array<array<EdgeFace<order>, 6>, 6>{};
            for (auto&& result_i : result)
                for (auto&& result_ij : result_i)
                    for (auto&& result_ijk : result_ij.facelets)
                        for (auto&& result_ijkl : result_ijk)
                            result_ijkl = {(i8)-100};
            auto cube6 = Cube<order, ColorType6>();
            cube6.Reset();
            auto cube48 = EdgeCube();
            cube48.Reset();
            for (auto face_id = 0; face_id < 6; face_id++) {
                for (auto x = 1; x < order - 1; x++) {
                    for (const auto y : {0, order - 1}) {
                        const auto pos =
                            FaceletPosition{(i8)face_id, (i8)y, (i8)x};
                        const auto adj_pos = GetAdjacentPosition(pos);
                        assert(pos == GetAdjacentPosition(adj_pos));
                        const auto color6 = cube6.Get(pos);
                        const auto adj_color6 = cube6.Get(adj_pos);
                        const auto color48 = cube48.Get(pos);
                        result[color6.data][adj_color6.data].Set(y, x, color48);
                        result[color6.data][adj_color6.data].Set(
                            x, order - 1 - y, color48);
                        result[color6.data][adj_color6.data].Set(
                            order - 1 - y, order - 1 - x, color48);
                        result[color6.data][adj_color6.data].Set(order - 1 - x,
                                                                 y, color48);
                    }
                }
                for (auto y = 1; y < order - 1; y++) {
                    for (const auto x : {0, order - 1}) {
                        const auto pos =
                            FaceletPosition{(i8)face_id, (i8)y, (i8)x};
                        const auto adj_pos = GetAdjacentPosition(pos);
                        assert(pos == GetAdjacentPosition(adj_pos));
                        const auto color6 = cube6.Get(pos);
                        const auto adj_color6 = cube6.Get(adj_pos);
                        const auto color48 = cube48.Get(pos);
                        result[color6.data][adj_color6.data].Set(y, x, color48);
                        result[color6.data][adj_color6.data].Set(
                            x, order - 1 - y, color48);
                        result[color6.data][adj_color6.data].Set(
                            order - 1 - y, order - 1 - x, color48);
                        result[color6.data][adj_color6.data].Set(order - 1 - x,
                                                                 y, color48);
                    }
                }
            }
            return result;
        }();

        for (auto face_id = 0; face_id < 6; face_id++) {
            for (auto y = 0; y < order; y++)
                for (auto x = 0; x < order; x++) {
                    if (((y == 0 || y == order - 1) ==
                         (x == 0 || x == order - 1)))
                        continue;
                    const auto pos = FaceletPosition{(i8)face_id, (i8)y, (i8)x};
                    const auto color6 = rhs.Get(pos);
                    const auto adj_color6 = rhs.Get(GetAdjacentPosition(pos));
                    const auto color48 =
                        color6_and_position_to_color48[color6.data]
                                                      [adj_color6.data]
                                                          .Get(y, x);
                    Set(face_id, y, x, color48);
                }
        }
    }

    inline void Print() const {
        assert(false);
        // TODO
    }

    inline void Display(ostream& os = cout) const {
        for (auto y = 0; y < order; y++) {
            for (auto x = 0; x < order + 1; x++)
                os << ' ';
            for (auto x = 0; x < order; x++)
                faces[D1].TryGet(y, x).Display(os);
            os << '\n';
        }
        for (auto y = 0; y < order; y++) {
            for (const auto face_id : {R1, F0, R0, F1}) {
                for (auto x = 0; x < order; x++)
                    faces[face_id].TryGet(y, x).Display(os);
                os << ' ';
            }
            os << '\n';
        }
        for (auto y = 0; y < order; y++) {
            for (auto x = 0; x < order + 1; x++)
                os << ' ';
            for (auto x = 0; x < order; x++)
                faces[D0].TryGet(y, x).Display(os);
            os << '\n';
        }
    }

    // TODO: Cube との相互変換
};

using EdgeAction = Formula;

template <int order> struct EdgeState {
    using EdgeCube = ::EdgeCube<order>;
    EdgeCube cube;
    int score;   // target との距離
    int n_moves; // これまでに回した回数

    inline EdgeState(const EdgeCube& cube, const EdgeCube& /*target_cube*/)
        : cube(cube), score(cube.ComputeEdgeScore()), n_moves() {}

    // inplace に変更する
    inline void Apply(const EdgeAction& action,
                      const EdgeCube& /*target_cube*/) {
        cube.Rotate(action);
        score = cube.ComputeEdgeScore();
        n_moves += action.Cost();
    }
};

// yield を使って EdgeAction を生成する？
template <int order> struct EdgeActionCandidateGenerator {
    using EdgeCube = ::EdgeCube<order>;
    using EdgeState = ::EdgeState<order>;
    vector<EdgeAction> actions;

    // ファイルから手筋を読み取る
    // ファイルには f1.d0.-r0.-f1 みたいなのが 1 行に 1 つ書かれている想定
    inline void FromFile(const string& filename, const bool is_normal) {
        actions.clear();

        // 面の回転 1 つだけからなる手筋を別途加える
        for (auto i = 0; i < 6; i++) {
            actions.emplace_back(vector<Move>{Move{(Move::Direction)i, (i8)0}});
            actions.back().EnableFaceletChanges<EdgeCube>();
            actions.emplace_back(
                vector<Move>{Move{(Move::Direction)i, (i8)(order - 1)}});
            actions.back().EnableFaceletChanges<EdgeCube>();
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
            if (!is_normal && line[0] == '0')
                continue;
            actions.emplace_back(line.substr(2));
            actions.back().EnableFaceletChanges<EdgeCube>();
        }

        // TODO: 重複があるかもしれないので確認した方が良い
    }

    inline const auto& Generate(const EdgeState&) const { return actions; }
};

template <int order> struct EdgeNode {
    using EdgeState = ::EdgeState<order>;
    EdgeState state;
    shared_ptr<EdgeNode> parent;
    EdgeAction last_action;
    inline EdgeNode(const EdgeState& state, const shared_ptr<EdgeNode>& parent,
                    const EdgeAction& last_action)
        : state(state), parent(parent), last_action(last_action) {}
};

template <int order> struct EdgeBeamSearchSolver {
    using EdgeCube = ::EdgeCube<order>;
    using EdgeState = ::EdgeState<order>;
    using EdgeNode = ::EdgeNode<order>;
    using EdgeActionCandidateGenerator = ::EdgeActionCandidateGenerator<order>;

    EdgeCube target_cube;
    EdgeActionCandidateGenerator action_candidate_generator;
    int beam_width;
    vector<vector<shared_ptr<EdgeNode>>> nodes;

    inline EdgeBeamSearchSolver(const EdgeCube& target_cube,
                                const bool is_normal, const int beam_width,
                                const string& formula_file)
        : target_cube(target_cube), action_candidate_generator(),
          beam_width(beam_width), nodes() {
        action_candidate_generator.FromFile(formula_file, is_normal);
    }

    inline shared_ptr<EdgeNode> Solve(const EdgeCube& start_cube) {
        auto rng = RandomNumberGenerator(42);

        const auto start_state = EdgeState(start_cube, target_cube);
        const auto start_node = make_shared<EdgeNode>(
            start_state, nullptr, EdgeAction{vector<Move>()});
        nodes.clear();
        nodes.resize(1);
        nodes[0].push_back(start_node);

        auto minimum_scores = array<int, 16>();
        fill(minimum_scores.begin(), minimum_scores.end(), 9999);
        for (auto current_cost = 0; current_cost < 100000; current_cost++) {
            auto current_minimum_score = 9999;
            for (const auto& node : nodes[current_cost]) {
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
                            new EdgeNode(new_state, node, action));
                    } else {
                        const auto idx = rng.Next() % beam_width;
                        if (new_state.score <
                            nodes[new_state.n_moves][idx]->state.score)
                            nodes[new_state.n_moves][idx].reset(
                                new EdgeNode(new_state, node, action));
                    }
                }
            }

            cout << format("current_cost={} current_minimum_score={}",
                           current_cost, current_minimum_score)
                 << endl;
            if (nodes[current_cost][0]->state.score ==
                minimum_scores[current_cost % 16]) {
                cerr << "Failed." << endl;
                nodes[current_cost][0]->state.cube.Display();
                return nullptr;
            }
            minimum_scores[current_cost % 16] = current_minimum_score;
            nodes[current_cost].clear();
        }
        cerr << "Failed." << endl;
        return nullptr;
    }
};

[[maybe_unused]] static void TestEdgeCube() {
    constexpr auto kOrder = 5;
    const auto moves = {
        Move{Move::Direction::F, (i8)2},  //
        Move{Move::Direction::D, (i8)0},  //
        Move{Move::Direction::F, (i8)0},  //
        Move{Move::Direction::D, (i8)0},  //
        Move{Move::Direction::Fp, (i8)2}, //
    };

    const auto formula = Formula(moves);
    formula.Print();
    cout << endl;
    formula.Display<EdgeCube<kOrder>>();
}

[[maybe_unused]] static void TestFromCube() {
    auto cube = Cube<5, ColorType6>();
    cube.Reset();
    auto reference_edge_cube = EdgeCube<5>();
    reference_edge_cube.Reset();
    auto converted_edge_cube = EdgeCube<5>();

    // ランダムに回す
    auto rng = RandomNumberGenerator(42);
    for (auto i = 0; i < 100; i++) {
        const auto mov =
            Move{(Move::Direction)(rng.Next() % 6), (i8)(rng.Next() % 5)};
        cube.Rotate(mov);
        reference_edge_cube.Rotate(mov);
    }

    cube.Display();
    reference_edge_cube.Display();

    converted_edge_cube.FromCube(cube);
    converted_edge_cube.Display();
}

[[maybe_unused]] static void TestReadNNN() {
    const auto filename = "input_example.nnn";
    auto cube = Cube<7, ColorType6>();
    cube.ReadNNN(filename);
    cube.Display();
    auto edge_cube = EdgeCube<7>();
    edge_cube.FromCube(cube);
    edge_cube.Display();
}

[[maybe_unused]] static void TestEdgeActionCandidateGenerator() {
    constexpr auto kOrder = 5;
    const auto formula_file = "out/edge_formula_5_4.txt";

    using EdgeActionCandidateGenerator = EdgeActionCandidateGenerator<kOrder>;
    using EdgeCube = typename EdgeActionCandidateGenerator::EdgeCube;

    auto action_candidate_generator = EdgeActionCandidateGenerator();
    action_candidate_generator.FromFile(formula_file, true);

    for (auto i : {0, 1, 2, 12, 13}) {
        const auto action = action_candidate_generator.actions[i];
        cout << format("# {}", i) << endl;
        action.Print();
        cout << endl;
        action.Display<EdgeCube>();
        cout << endl << endl;
    }
}

[[maybe_unused]] static void TestEdgeBeamSearch() {
    constexpr auto kOrder = 7;
    const auto formula_file = "out/edge_formula_7_5.txt";
    const auto beam_width = 32;

    using Solver = EdgeBeamSearchSolver<kOrder>;
    using EdgeCube = typename Solver::EdgeCube;

    auto initial_cube = EdgeCube();
    if (false) {
        // ランダムな initial_cube を用意する
        // 解けたり解けなかったりする
        initial_cube.Reset();
        auto rng = RandomNumberGenerator(42);
        for (auto i = 0; i < 200; i++) {
            const auto mov = Move{(Move::Direction)(rng.Next() % 6),
                                  (i8)(rng.Next() % kOrder)};
            initial_cube.Rotate(mov);
        }
    } else {
        const auto input_cube_file = "in/input_example_1.nnn";
        // const auto input_cube_file = "in/input_example_2.nnn";
        // const auto input_cube_file = "in/input_example_3.nnn";
        auto cube = Cube<7, ColorType6>();
        cube.ReadNNN(input_cube_file);
        initial_cube.FromCube(cube);
    }

    auto target_cube = EdgeCube();
    target_cube.Reset();

    auto solver = Solver(target_cube, true, beam_width, formula_file);

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

template <int order>
static void SolveWithOrder(const int problem_id, const bool is_normal,
                           const Formula& sample_formula) {
    constexpr auto beam_width = 32;
    constexpr auto formula_depth = 8;
    const auto formula_file =
        format("out/edge_formula_{}_{}.txt", order, formula_depth);

    // 面ソルバの出力を読み込む
    const auto face_solution_file =
        format("solution_face/{}_best.txt", problem_id);
    auto face_solution_string = string();
    auto ifs = ifstream(face_solution_file);
    if (!ifs.good()) {
        cerr << format("Cannot open file `{}`.", face_solution_file) << endl;
        abort();
    }
    getline(ifs, face_solution_string);
    ifs.close();
    const auto face_solution = Formula(face_solution_string);

    // キューブを表示するラムダ式
    const auto display_cube = [&sample_formula, &face_solution, &is_normal](
                                  const Formula& solution = Formula()) {
        if (is_normal) {
            auto cube = Cube<order, ColorType6>();
            cube.Reset();
            cube.RotateInv(sample_formula);
            cube.Rotate(face_solution);
            cube.Rotate(solution);
            cube.Display();
        } else {
            auto cube = Cube<order, ColorType24>();
            cube.Reset();
            cube.RotateInv(sample_formula);
            cube.Rotate(face_solution);
            cube.Rotate(solution);
            cube.Display();
        }
    };
    display_cube(); // 初期状態を描画する

    // 初期値の設定など
    const auto initial_cube = [&sample_formula, &face_solution] {
        auto initial_cube = Cube<order, ColorType6>();
        initial_cube.Reset();
        initial_cube.RotateInv(sample_formula);
        initial_cube.Rotate(face_solution);
        return initial_cube;
    }();
    const auto initial_edge_cube = [&initial_cube, &sample_formula,
                                    &face_solution] {
        auto initial_edge_cube = EdgeCube<order>();
        initial_edge_cube.FromCube(initial_cube);
        if (order % 2 == 0) {
            for (const auto mov : sample_formula.moves)
                if (mov.IsFaceRotation<order>())
                    initial_edge_cube.corner_parity =
                        !initial_edge_cube.corner_parity;
            for (const auto mov : face_solution.moves)
                if (mov.IsFaceRotation<order>())
                    initial_edge_cube.corner_parity =
                        !initial_edge_cube.corner_parity;
        }
        return initial_edge_cube;
    }();
    auto target_edge_cube = EdgeCube<order>();
    target_edge_cube.Reset();

    // パリティは予め解消しておく
    auto parity_resolved_edge_cube = initial_edge_cube;
    const auto parities = parity_resolved_edge_cube.ComputeParities();
    const auto parity_resolving_formula =
        parity_resolved_edge_cube.ComputeParityResolvingFormula(parities);
    parity_resolved_edge_cube.Rotate(parity_resolving_formula);
    auto result_moves = parity_resolving_formula.moves;
    cout << "parity: ";
    for (const auto p : parities)
        cout << p;
    cout << endl;
    cout << "parity_resolving_formula: " << parity_resolving_formula.Cost()
         << " ";
    parity_resolving_formula.Print();
    cout << endl;
    display_cube(parity_resolving_formula);

    // 解く
    auto solver = EdgeBeamSearchSolver<order>(target_edge_cube, is_normal,
                                              beam_width, formula_file);
    const auto node = solver.Solve(parity_resolved_edge_cube);
    if (node == nullptr) // 失敗
        return;

    // 結果を表示する
    cout << node->state.n_moves << endl;
    for (auto p = node; p->parent != nullptr; p = p->parent)
        for (auto i = (int)p->last_action.moves.size() - 1; i >= 0; i--)
            result_moves.emplace_back(p->last_action.moves[i]);
    reverse(result_moves.begin() + parity_resolving_formula.Cost(),
            result_moves.end());
    const auto solution = Formula(result_moves);
    solution.Print();
    cout << endl;
    display_cube(solution);

    // 結果を保存する
    const auto all_solutions_file =
        format("solution_edge/{}_all.txt", problem_id);
    const auto best_solution_file =
        format("solution_edge/{}_best.txt", problem_id);
    auto ofs_all = ofstream(all_solutions_file, ios::app);
    if (ofs_all.good()) {
        face_solution.Print(ofs_all);
        ofs_all << endl;
        solution.Print(ofs_all);
        ofs_all << endl
                << format("face_solution_score={} edge_solution_score={} "
                          "beam_width={} formula_depth={}",
                          face_solution.Cost(), solution.Cost(), beam_width,
                          formula_depth)
                << endl;
        ofs_all.close();
    } else {
        cerr << format("Cannot open file `{}`.", all_solutions_file) << endl;
    }
    auto ifs_best = ifstream(best_solution_file);
    auto best_score = 99999;
    if (ifs_best.good()) {
        string line;
        getline(ifs_best, line); // 面の解を読み飛ばす
        getline(ifs_best, line); // 面のスコアを読む
        best_score = stoi(line);
        getline(ifs_best, line); // 辺の解を読み飛ばす
        getline(ifs_best, line); // 辺のスコアの行を読む
        best_score += stoi(line);
        ifs_best.close();
    }
    if (face_solution.Cost() + solution.Cost() < best_score) {
        auto ofs_best = ofstream(best_solution_file);
        if (ofs_best.good()) {
            // 面の解とスコアを書き込む
            face_solution.Print(ofs_best);
            ofs_best << endl << face_solution.Cost() << endl;
            // 辺の解とスコアを書き込む
            solution.Print(ofs_best);
            ofs_best << endl << solution.Cost() << endl;
            // 3x3x3 に帰着したキューブの状態を書き込む
            auto cube = Cube<order, ColorType6>();
            cube.Reset();
            cube.RotateInv(sample_formula);
            cube.Rotate(face_solution);
            cube.Rotate(solution);
            cube.PrintSingmaster(ofs_best);
            ofs_best << endl;
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

// clang++ -std=c++20 -Wall -Wextra -O3 edge_cube.cpp -DTEST_EDGE_CUBE
#ifdef TEST_EDGE_CUBE
int main() { TestEdgeCube(); }
#endif

// clang++ -std=c++20 -Wall -Wextra -O3 edge_cube.cpp -DTEST_FROM_CUBE
#ifdef TEST_FROM_CUBE
int main() { TestFromCube(); }
#endif

// clang++ -std=c++20 -Wall -Wextra -O3 edge_cube.cpp -DTEST_READ_NNN
#ifdef TEST_READ_NNN
int main() { TestReadNNN(); }
#endif

// clang-format off
// clang++ -std=c++20 -Wall -Wextra -O3 edge_cube.cpp -DTEST_EDGE_ACTION_CANDIDATE_GENERATOR
#ifdef TEST_EDGE_ACTION_CANDIDATE_GENERATOR
int main() { TestEdgeActionCandidateGenerator(); }
#endif
// clang-format on

// clang++ -std=c++20 -Wall -Wextra -O3 edge_cube.cpp -DTEST_EDGE_BEAM_SEARCH
#ifdef TEST_EDGE_BEAM_SEARCH
int main() { TestEdgeBeamSearch(); }
#endif

// clang++ -std=c++20 -Wall -Wextra -O3 edge_cube.cpp -DSOLVE
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