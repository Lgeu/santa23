#include "cube.cpp"

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
};

template <int order_, typename ColorType_ = ColorType48> struct EdgeCube {
    static constexpr auto order = order_;
    using ColorType = ColorType_;
    enum { D1, F0, R0, F1, R1, D0 };

    static_assert(is_same_v<ColorType, ColorType48>);

    array<EdgeFace<order, ColorType>, 6> faces;

    inline void Reset() {
        for (auto face_id = 0; face_id < 6; face_id++)
            faces[face_id].Reset(ColorType{(i8)(face_id * 8)});
    }

    inline void Rotate(const Move& mov) {
#define FROM_BOTTOM order - 1 - mov.depth, i
#define FROM_LEFT i, mov.depth
#define FROM_TOP mov.depth, order - 1 - i
#define FROM_RIGHT order - 1 - i, order - 1 - mov.depth
        const auto is_face_rotation = mov.depth == 0 || mov.depth == order - 1;
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
                Set(formula.facelet_changes[i].from, new_colors[i]);
        } else
            for (const auto& m : formula.moves)
                Rotate(m);
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

    inline auto ComputeEdgeDiff(const EdgeCube& rhs) const {
        // ColorType48 であれば半分見れば良さそうではある
        // TODO: 差分計算
        auto diff = 0;
        for (auto face_id = 0; face_id < 6; face_id++)
            for (auto edge_id = 0; edge_id < 4; edge_id++)
                for (auto x = 0; x < order - 2; x++)
                    diff += faces[face_id].facelets[edge_id][x] !=
                            rhs.faces[face_id].facelets[edge_id][x];
        assert(diff % 2 == 0);
        return diff;
    }

    // Kaggle フォーマットを読み取る
    inline void Read(const string& s) {
        assert(s.size() >= 6 * order * order * 2 - 1);
        auto i = 0;
        for (auto face_id = 0; face_id < 6; face_id++)
            for (auto y = 0; y < order; y++)
                for (auto x = 0; x < order; x++) {
                    if ((y == 0 || y == order - 1) !=
                        (x == 0 || x == order - 1))
                        Set(face_id, y, x, {s[i] - 'A'});
                    i += 2;
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

// clang++ -std=c++20 -Wall -Wextra -O3 edge_cube.cpp -DTEST_EDGE_CUBE
#ifdef TEST_EDGE_CUBE
int main() { TestEdgeCube(); }
#endif