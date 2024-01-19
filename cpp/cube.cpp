#include <array>
#include <cassert>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

using std::array;
using std::cerr;
using std::cout;
using std::endl;
using std::format;
using std::getline;
using std::ifstream;
using std::is_same_v;
using std::istringstream;
using std::make_shared;
using std::ofstream;
using std::ostream;
using std::pair;
using std::same_as;
using std::shared_ptr;
using std::string;
using std::stringstream;
using std::tuple;
using std::vector;
using ios = std::ios;

using i8 = signed char;
using u64 = unsigned long long;

struct RandomNumberGenerator {
  private:
    u64 seed;

  public:
    inline RandomNumberGenerator(const u64 seed) : seed(seed) {}
    inline auto Next() {
        seed ^= seed << 9;
        seed ^= seed >> 7;
        return seed;
    }
};

template <int siz> struct RandomNumberTable {
  private:
    array<u64, siz> data;

  public:
    inline RandomNumberTable(const u64 seed) {
        auto rng = RandomNumberGenerator(seed);
        for (auto&& d : data)
            d = rng.Next();
    }
    inline auto Get(const int i) const {
        assert(i <= 0 && i < siz);
        return data[i];
    }
};

struct ColorType6 {
    static constexpr auto kNColors = 6;
    i8 data;

    inline auto operator<=>(const ColorType6&) const = default;

    inline void Print(ostream& os) const {
        if (data < 0)
            os << ' ';
        else
            os << (char)('A' + data);
    }

    inline void Display(ostream& os) const {
        // 青、赤、白、橙 (マゼンタ)、黄、緑
        static constexpr auto kColors = array<const char*, 6>{
            "\e[44m", "\e[41m", "\e[47m", "\e[45m", "\e[43m", "\e[42m",
        };
        if (data >= 0)
            os << kColors[data];
        Print(os);
        if (data >= 0)
            os << "\e[0m";
    }

    inline void PrintSingmaster(ostream& os) const {
        if (data < 0)
            os << ' ';
        else
            os << "UFRBLD"[data];
    }

    friend auto& operator<<(ostream& os, const ColorType6 t) {
        t.Print(os);
        return os;
    }
};

struct ColorType24 {
    static constexpr auto kNColors = 24;
    i8 data;

    inline auto operator<=>(const ColorType24&) const = default;

    inline void Print(ostream& os) const { os << (char)('A' + data); }

    inline void Display(ostream& os) const {
        // 青、赤、白、橙 (マゼンタ)、黄、緑
        static constexpr auto kColors = array<const char*, 6>{
            "\e[44m", "\e[41m", "\e[47m", "\e[45m", "\e[43m", "\e[42m",
        };
        os << kColors[data / 4];
        Print(os);
        os << "\e[0m";
    }

    inline void PrintSingmaster(ostream& os) const {
        if (data < 0)
            os << ' ';
        else
            os << "UFRBLD"[data / 4];
    }

    friend auto& operator<<(ostream& os, const ColorType24 t) {
        t.Print(os);
        return os;
    }
};

template <int order, typename ColorType_ = ColorType6, bool use_hash = false>
struct Face {
    using ColorType = ColorType_;
    static_assert(sizeof(ColorType) == 1); // 実質最大 24 色なので 1byte

    // 回転した時にいい感じに計算できるような乱数表
    struct RNT {
      public:
        inline RNT(const u64 /*seed*/) {
            // TODO
        }
        inline auto Get(const int /*y*/, const int /*x*/,
                        const ColorType /*color*/) const {
            // TODO
        }
    };

  private:
    array<array<ColorType, order>, order> facelets; // マス
    int orientation;                                // 軸
    u64 hash_value;                                 // ハッシュ値
    shared_ptr<RNT> rnt; // ハッシュ計算の乱数表

  public:
    static inline auto MakeRNT(const u64 seed)
        requires use_hash
    {
        return make_shared<RNT>(seed);
    }

    // 0 で初期化
    inline Face()
        requires(!use_hash)
        : facelets(), orientation(), hash_value(), rnt() {}

    // 0 で初期化
    inline Face(const RNT& rnt)
        requires use_hash
        : facelets(), orientation(), hash_value(), rnt(rnt) {
        Fill(0);
    }

    // 1 色で埋める
    void Fill(const ColorType color) {
        if constexpr (use_hash)
            hash_value = 0ull;
        for (auto y = 0; y < order; y++)
            for (auto x = 0; x < order; x++) {
                if constexpr (use_hash)
                    hash_value ^= rnt->Get(
                        ((y * order) + x) * ColorType::kNColors + color);
                facelets[y][x] = color;
            }
    }

    // ColorType に応じて初期化
    void Reset(const ColorType start_color) {
        if constexpr (use_hash)
            hash_value = 0ull;
        orientation = 0;
        if constexpr (is_same_v<ColorType, ColorType6>) {
            Fill(start_color);
        } else if constexpr (is_same_v<ColorType, ColorType24>) {
            assert(start_color.data % 4 == 0);
            for (auto y = 0; y < order; y++)
                for (auto x = 0; x < order; x++) {
                    // AAB
                    // DAB
                    // DCC
                    const auto color = ColorType{
                        (i8)(start_color.data +
                             (y < order / 2 && x < (order + 1) / 2          ? 0
                              : y < (order + 1) / 2 && x >= (order + 1) / 2 ? 1
                              : y >= (order + 1) / 2 && x >= order / 2      ? 2
                              : y >= order / 2 && x < order / 2             ? 3
                                                                : 0))};
                    if constexpr (use_hash)
                        hash_value ^=
                            rnt->Get(((y * order) + x) * ColorType::kNColors +
                                     color.data);
                    facelets[y][x] = color;
                }
        } else {
            static_assert([] { return false; }());
        }
    }

    // 時計回りに step * 90 度回転
    inline void RotateCW(const int step) {
        if constexpr (use_hash) {
            assert(false); // TODO
        }
        orientation = (orientation - step) & 3;
    }

    inline ColorType Get(const int y, const int x) const {
        assert(0 <= y && y < order);
        assert(0 <= x && x < order);
        switch (orientation) {
        case 0:
            return facelets[y][x];
        case 1:
            return facelets[x][order - 1 - y];
        case 2:
            return facelets[order - 1 - y][order - 1 - x];
        case 3:
            return facelets[order - 1 - x][y];
        default:
            assert(false);
            return {};
        }
    }

    inline ColorType Get(const int y, const int x,
                         const int orientation) const {
        assert(0 <= y && y < order);
        assert(0 <= x && x < order);
        switch (orientation) {
        case 0:
            return facelets[y][x];
        case 1:
            return facelets[x][order - 1 - y];
        case 2:
            return facelets[order - 1 - y][order - 1 - x];
        case 3:
            return facelets[order - 1 - x][y];
        default:
            assert(false);
            return {};
        }
    }

    inline void Set(const int y, const int x, const ColorType color) {
        assert(0 <= y && y < order);
        assert(0 <= x && x < order);

        // 回転操作に対してハッシュの計算が重すぎるような気がする
        if constexpr (use_hash) {
            assert(rnt.get() != nullptr);
            hash_value ^=
                rnt->Get(((y * order) + x) * ColorType::kNColors + Get(y, x));
            hash_value ^=
                rnt->Get(((y * order) + x) * ColorType::kNColors + color);
        }
        switch (orientation) {
        case 0:
            facelets[y][x] = color;
            return;
        case 1:
            facelets[x][order - 1 - y] = color;
            return;
        case 2:
            facelets[order - 1 - y][order - 1 - x] = color;
            return;
        case 3:
            facelets[order - 1 - x][y] = color;
            return;
        default:
            assert(false);
        }
    }

    inline ColorType GetIgnoringOrientation(const int y, const int x) const {
        assert(0 <= y && y < order);
        assert(0 <= x && x < order);
        return facelets[y][x];
    }

    inline auto Hash() const
        requires use_hash
    {
        return hash_value;
    }

    inline int GetOrientation() const { return orientation; }

    inline int SetOrientation(const int orientation_) {
        orientation = orientation_;
    }

    inline pair<int, int> GetInternalCoordinate(const int y,
                                                const int x) const {
        assert(0 <= y && y < order);
        assert(0 <= x && x < order);
        switch (orientation) {
        case 0:
            return {y, x};
        case 1:
            return {x, order - 1 - y};
        case 2:
            return {order - 1 - y, order - 1 - x};
        case 3:
            return {order - 1 - x, y};
        default:
            assert(false);
            return {};
        }
    }

    inline ColorType GetRaw(const int y, const int x) const {
        assert(0 <= y && y < order);
        assert(0 <= x && x < order);
        return facelets[y][x];
    }
};

// 手 (90 度回転)
struct Move {
    enum struct Direction : i8 { F, Fp, D, Dp, R, Rp }; // p は反時計回り
    enum struct Axis : i8 { F, D, R };
    Direction direction;
    i8 depth;

    inline bool IsClockWise() const { return ((int)direction & 1) == 0; }

    template <int order> inline bool IsFaceRotation() const {
        return depth == 0 || depth == order - 1;
    }

    inline void Print(ostream& os = cout) const {
        static constexpr auto direction_strings =
            array<const char*, 6>{"f", "-f", "d", "-d", "r", "-r"};
        os << direction_strings[(int)direction] << (int)depth;
    }

    inline Move Inv() const {
        return {(Direction)((i8)direction ^ (i8)1), depth};
    }

    inline Axis GetAxis() const { return (Axis)((int)direction >> 1); }

    inline auto operator<=>(const Move&) const = default;
};

// マスの座標
struct FaceletPosition {
    i8 face_id, y, x;
    inline auto operator<=>(const FaceletPosition&) const = default;
};

// orientation を考慮しないマスの位置
struct FaceletPositionRaw {
    i8 face_id, y, x;
    inline auto operator<=>(const FaceletPositionRaw&) const = default;
};

template <int order_, typename ColorType_> struct Cube;

template <class T>
concept Cubeish =
    requires(T& x, Move mov, ostream os) {
        x.Reset();
        x.Rotate(mov);
        x.Display();
        x.Display(os);
        T::AllFaceletPositions();
        {
            T::ComputeOriginalFaceletPosition(0, 0, typename T::ColorType())
            } -> same_as<FaceletPosition>;
        { T::order } -> same_as<const int&>;
    };

// 手筋
struct Formula {
    struct FaceletChange {
        FaceletPosition from, to;
    };
    struct FaceletChangeRaw {
        FaceletPositionRaw from, to;
    };

    // 手の列
    vector<Move> moves;
    // facelet_changes を使うかどうか
    bool use_facelet_changes;
    bool use_facelet_changes_raw;
    // 手筋を適用することでどのマスがどこに移動するか (TODO)
    vector<FaceletChange> facelet_changes;
    vector<FaceletChangeRaw> facelet_changes_raw;

    inline Formula()
        : moves(), use_facelet_changes(false), use_facelet_changes_raw(false),
          facelet_changes(), facelet_changes_raw() {}

    inline Formula(const vector<Move>& moves)
        : moves(moves), use_facelet_changes(false),
          use_facelet_changes_raw(false), facelet_changes(),
          facelet_changes_raw() {}

    inline Formula(const vector<Move>& moves,
                   const vector<FaceletChange>& facelet_changes)
        : moves(moves), use_facelet_changes(true),
          use_facelet_changes_raw(false), facelet_changes(facelet_changes),
          facelet_changes_raw() {}

    // f1.d0.-r0.-f1 のような形式を読み取る
    inline Formula(const string& s)
        : moves(), use_facelet_changes(), facelet_changes() {
        assert(s.size() >= 2);
        auto iss = istringstream(s);
        string token;
        while (getline(iss, token, '.')) {
            bool inv = false;
            if (token[0] == '-') {
                inv = true;
                token = token.substr(1);
            }
            Move::Direction direction;
            switch (token[0]) {
            case 'f':
                direction = Move::Direction::F;
                break;
            case 'd':
                direction = Move::Direction::D;
                break;
            case 'r':
                direction = Move::Direction::R;
                break;
            default:
                assert(false);
                direction = {};
            }
            direction = (Move::Direction)((int)direction | (int)inv);
            const auto depth = (i8)stoi(token.substr(1));
            moves.push_back({direction, depth});
        }
    }

    inline int Cost() const { return (int)moves.size(); }

    template <Cubeish CubeType> inline void EnableFaceletChanges() {
        if (use_facelet_changes)
            return;
        use_facelet_changes = true;
        facelet_changes.clear();
        auto cube = CubeType();
        cube.Reset();
        for (const auto& mov : moves)
            cube.Rotate(mov);

        for (const FaceletPosition pos : CubeType::AllFaceletPositions()) {
            const auto color = cube.Get(pos);
            const auto original_pos =
                CubeType::ComputeOriginalFaceletPosition(pos.y, pos.x, color);
            if (pos != original_pos)
                facelet_changes.push_back({original_pos, pos});
        }
    }

    template <Cubeish CubeType> inline void EnableFaceletChangesAll() {
        if (use_facelet_changes)
            return;
        use_facelet_changes = true;
        facelet_changes.clear();
        auto cube = CubeType();
        cube.Reset();
        for (const auto& mov : moves)
            cube.Rotate(mov);

        for (const FaceletPosition pos : CubeType::AllFaceletPositions()) {
            const auto color = cube.Get(pos);
            const auto original_pos =
                CubeType::ComputeOriginalFaceletPosition(pos.y, pos.x, color);
        }
    }

    inline void DisableFaceletChanges() {
        use_facelet_changes = false;
        facelet_changes.clear();
    }

    // FaceletPositionRaw が異なるものだけを残す
    // 目的: 面回転時に FaceletChange が計算さえれのを防ぐ
    template <Cubeish CubeType>
    inline void EnableFaceletChangesWithNoSameRaw() {
        // if (use_facelet_changes)
        //     return;
        use_facelet_changes = true;
        facelet_changes.clear();
        auto cube = CubeType();
        cube.Reset();
        for (const auto& mov : moves)
            cube.Rotate(mov);

        for (const FaceletPosition pos : CubeType::AllFaceletPositions()) {
            // const auto color = cube.Get(pos);
            // const auto original_pos =
            //     CubeType::ComputeOriginalFaceletPosition(pos.y, pos.x,
            //     color);

            // FaceletPositionRaw pos_raw = {pos.face_id, pos.y, pos.x};
            // auto [original_pos_raw_y, original_pos_raw_x] =
            //     cube.faces[original_pos.face_id].GetInternalCoordinate(
            //         original_pos.y, original_pos.x);
            // FaceletPositionRaw original_pos_raw = {original_pos.face_id,
            //                                        (i8)original_pos_raw_y,
            //                                        (i8)original_pos_raw_x};

            // if (pos != original_pos && pos_raw != original_pos_raw)
            //     facelet_changes.push_back({original_pos, pos});

            const auto color = cube.Get(pos);
            const auto original_pos =
                CubeType::ComputeOriginalFaceletPosition(pos.y, pos.x, color);
            auto [pos_raw_y, pos_raw_x] =
                cube.faces[pos.face_id].GetInternalCoordinate(pos.y, pos.x);
            FaceletPosition pos_raw = {pos.face_id, (i8)pos_raw_y,
                                       (i8)pos_raw_x};

            if (pos_raw != original_pos)
                facelet_changes.push_back({original_pos, pos});
        }
    }

    // 辺と角の FaceletChange を削除
    // 面ビーム用
    template <Cubeish CubeType> inline void DisableFaceletChangeEdgeCorner() {
        if (!use_facelet_changes)
            return;
        int idx = 0;
        int order = CubeType::order;
        while (idx < (int)facelet_changes.size()) {
            const auto& facelet_change = facelet_changes[idx];
            int y = facelet_change.from.y, x = facelet_change.from.x;
            if ((y == 0 || y == order - 1) || (x == 0 || x == order - 1)) {
                facelet_changes[idx] = facelet_changes.back();
                facelet_changes.pop_back();
            } else {
                idx++;
            }
        }
    }

    // 同じ面に移動する FaceletChange を削除
    // 面ビーム用
    template <Cubeish CubeType> inline void DisableFaceletChangeSameFace() {
        if (!use_facelet_changes)
            return;
        int idx = 0;
        while (idx < (int)facelet_changes.size()) {
            const auto& facelet_change = facelet_changes[idx];
            if (facelet_change.from.face_id == facelet_change.to.face_id) {
                // cerr << "DisableFaceletChangeSameFace: "
                //      << (int)facelet_change.from.face_id << " "
                //      << (int)facelet_change.from.y << " "
                //      << (int)facelet_change.from.x << " -> "
                //      << (int)facelet_change.to.y << " "
                //      << (int)facelet_change.to.x << endl;
                facelet_changes[idx] = facelet_changes.back();
                facelet_changes.pop_back();
            } else {
                idx++;
            }
        }
    }

    template <Cubeish CubeType> inline void EnableFaceletChangesRaw() {
        if (use_facelet_changes_raw)
            return;
        use_facelet_changes_raw = true;
        facelet_changes_raw.clear();
        auto cube = CubeType();
        cube.Reset();

        for (int i = 0; i < 6; i++) {
            assert(cube.faces[i].GetOrientation() == 0);
        }

        for (const auto& mov : moves)
            cube.Rotate(mov);

        for (const FaceletPosition pos : CubeType::AllFaceletPositions()) {
            const auto color = cube.Get(pos);

            // original_pos の orientation は 0
            const auto original_pos =
                CubeType::ComputeOriginalFaceletPosition(pos.y, pos.x, color);

            FaceletPositionRaw pos_raw = {pos.face_id, pos.y, pos.x};
            auto [original_pos_raw_y, original_pos_raw_x] =
                cube.faces[original_pos.face_id].GetInternalCoordinate(
                    original_pos.y, original_pos.x);
            FaceletPositionRaw original_pos_raw = {original_pos.face_id,
                                                   (i8)original_pos_raw_y,
                                                   (i8)original_pos_raw_x};
            if (pos_raw != original_pos_raw)
                facelet_changes_raw.push_back({original_pos_raw, pos_raw});
        }
    }

    template <Cubeish CubeType>
    inline void DisableFaceletChangeRawEdgeCorner() {
        if (!use_facelet_changes_raw)
            return;
        int idx = 0;
        int order = CubeType::order;
        while (idx < (int)facelet_changes_raw.size()) {
            const auto& facelet_change_raw = facelet_changes_raw[idx];
            int y = facelet_change_raw.from.y, x = facelet_change_raw.from.x;
            if ((y == 0 || y == order - 1) || (x == 0 || x == order - 1)) {
                facelet_changes_raw[idx] = facelet_changes_raw.back();
                facelet_changes_raw.pop_back();
            } else {
                idx++;
            }
        }
    }

    // f1.d0.-r0.-f1 のような形式で出力する
    inline void Print(ostream& os = cout) const {
        for (auto i = 0; i < (int)moves.size(); i++) {
            if (i != 0)
                os << '.';
            moves[i].Print(os);
        }
    }

    template <Cubeish CubeType> inline void Display(ostream& os = cout) const {
        auto cube = CubeType();
        cube.Reset();
        cube.Display(os);
        for (auto idx_moves = 0; idx_moves < (int)moves.size(); idx_moves++) {
            for (auto i = 0; i < (int)moves.size(); i++) {
                if (i != 0)
                    os << '.';
                if (i == idx_moves)
                    os << '[';
                moves[i].Print(os);
                if (i == idx_moves)
                    os << ']';
            }
            os << "\n";
            cube.Rotate(moves[idx_moves]);
            cube.Display(os);
        }
    }
};

// キューブ
template <int order_, typename ColorType_ = ColorType6> struct Cube {
    static constexpr auto order = order_;
    using ColorType = ColorType_;
    enum { D1, F0, R0, F1, R1, D0 };

    array<Face<order, ColorType>, 6> faces;

    inline static i8 GetOppositeFaceId(const i8 face_id) {
        if (face_id == D0)
            return D1;
        if (face_id == D1)
            return D0;
        if (face_id == F0)
            return F1;
        if (face_id == F1)
            return F0;
        if (face_id == R0)
            return R1;
        if (face_id == R1)
            return R0;
        assert(false);
        return -1;
    }

    inline static i8 GetFaceDistance(const i8 face_id1, const i8 face_id2) {
        if constexpr (is_same_v<ColorType, ColorType6>) {
            // normal face
            if (face_id1 == face_id2)
                return 0;
            if (face_id1 == GetOppositeFaceId(face_id2))
                return 2;
            return 1;
        } else
            assert(false);
        return -1;
    }

    inline static Move GetFaceRotateMove(const i8 face_id) {
        if (face_id == F0)
            return {Move::Direction::F, 0};
        if (face_id == F1)
            return {Move::Direction::F, order - 1};
        if (face_id == D0)
            return {Move::Direction::D, 0};
        if (face_id == D1)
            return {Move::Direction::D, order - 1};
        if (face_id == R0)
            return {Move::Direction::R, 0};
        if (face_id == R1)
            return {Move::Direction::R, order - 1};
        assert(false);
        return {};
    }

    inline static Move GetFaceRotateMoveInv(const i8 face_id) {
        if (face_id == F0)
            return {Move::Direction::Fp, 0};
        if (face_id == F1)
            return {Move::Direction::Fp, order - 1};
        if (face_id == D0)
            return {Move::Direction::Dp, 0};
        if (face_id == D1)
            return {Move::Direction::Dp, order - 1};
        if (face_id == R0)
            return {Move::Direction::Rp, 0};
        if (face_id == R1)
            return {Move::Direction::Rp, order - 1};
        assert(false);
        return {};
    }

    inline void Reset() {
        if constexpr (is_same_v<ColorType, ColorType6>) {
            for (auto face_id = 0; face_id < 6; face_id++)
                faces[face_id].Reset(ColorType{(i8)face_id});
        } else if constexpr (is_same_v<ColorType, ColorType24>) {
            for (auto face_id = 0; face_id < 6; face_id++)
                faces[face_id].Reset(ColorType{(i8)(face_id * 4)});
        } else {
            static_assert([] { return false; }());
        }
    }

    inline void Rotate(const Move& mov) {
#define FROM_BOTTOM order - 1 - mov.depth, i
#define FROM_LEFT i, mov.depth
#define FROM_TOP mov.depth, order - 1 - i
#define FROM_RIGHT order - 1 - i, order - 1 - mov.depth
        switch (mov.direction) {
        case Move::Direction::F:
            if (mov.depth == 0)
                faces[F0].RotateCW(1);
            else if (mov.depth == order - 1)
                faces[F1].RotateCW(-1);
            for (auto i = 0; i < order; i++) {
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
            for (auto i = 0; i < order; i++) {
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
            for (auto i = 0; i < order; i++) {
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
            for (auto i = 0; i < order; i++) {
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
            for (auto i = 0; i < order; i++) {
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
            for (auto i = 0; i < order; i++) {
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

    inline void RotateOrientation(const Move& mov) {
        switch (mov.direction) {
        case Move::Direction::F:
            if (mov.depth == 0)
                faces[F0].RotateCW(1);
            else if (mov.depth == order - 1)
                faces[F1].RotateCW(-1);
            break;
        case Move::Direction::Fp:
            if (mov.depth == 0)
                faces[F0].RotateCW(-1);
            else if (mov.depth == order - 1)
                faces[F1].RotateCW(1);
            break;
        case Move::Direction::D:
            if (mov.depth == 0)
                faces[D0].RotateCW(1);
            else if (mov.depth == order - 1)
                faces[D1].RotateCW(-1);
            break;
        case Move::Direction::Dp:
            if (mov.depth == 0)
                faces[D0].RotateCW(-1);
            else if (mov.depth == order - 1)
                faces[D1].RotateCW(1);
            break;
        case Move::Direction::R:
            if (mov.depth == 0)
                faces[R0].RotateCW(1);
            else if (mov.depth == order - 1)
                faces[R1].RotateCW(-1);
            break;
        case Move::Direction::Rp:
            if (mov.depth == 0)
                faces[R0].RotateCW(-1);
            else if (mov.depth == order - 1)
                faces[R1].RotateCW(1);
            break;
        }
    }

    inline void Rotate(const Formula& formula) {
        // if (formula.use_facelet_changes)
        //     assert(false); // TODO
        // else
        for (const auto& m : formula.moves)
            Rotate(m);
    }

    inline void RotateInv(const Formula& formula) {
        // if (formula.use_facelet_changes)
        //     assert(false); // TODO
        // else
        for (int i = (int)formula.Cost() - 1; i >= 0; i--)
            Rotate(formula.moves[i].Inv());
    }

    inline void RotateOrientation(const Formula& formula) {
        // if (formula.use_facelet_changes)
        //     assert(false); // TODO
        // else
        for (const auto& m : formula.moves)
            RotateOrientation(m);
    }

    inline void RotateOrientationInv(const Formula& formula) {
        // if (formula.use_facelet_changes)
        //     assert(false); // TODO
        // else
        for (int i = (int)formula.Cost() - 1; i >= 0; i--)
            RotateOrientation(formula.moves[i].Inv());
    }

    inline static constexpr auto AllFaceletPositions() {
        array<FaceletPosition, order * order * 6> positions;
        auto i = 0;
        for (auto face_id = 0; face_id < 6; face_id++)
            for (auto y = 0; y < order; y++)
                for (auto x = 0; x < order; x++)
                    positions[i++] = {(i8)face_id, (i8)y, (i8)x};
        return positions;
    }

    inline auto Get(const int face_id, const int y, const int x) const {
        return faces[face_id].Get(y, x);
    }

    inline auto Get(const FaceletPosition& facelet_position) const {
        return Get(facelet_position.face_id, facelet_position.y,
                   facelet_position.x);
    }

    inline auto GetRaw(const int face_id, const int y, const int x) const {
        return faces[face_id].GetRaw(y, x);
    }

    inline auto GetRaw(const FaceletPositionRaw& facelet_position_raw) const {
        return GetRaw(facelet_position_raw.face_id, facelet_position_raw.y,
                      facelet_position_raw.x);
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
        if constexpr (is_same_v<ColorType, ColorType24>) {
            // color / 4 で面が決まる
            // color % 4 で象限が決まる
            // y, x でその中の位置がきまる
            static constexpr auto table = []() {
                struct Pos {
                    i8 y, x;
                };
                auto table = array<array<array<Pos, 4>, order>, order>();
                for (auto y = 0; y < order / 2; y++)
                    for (auto x = 0; x < (order + 1) / 2; x++) {
                        for (auto [rotated_y, rotated_x] :
                             {array<int, 2>{y, x},
                              {x, order - 1 - y},
                              {order - 1 - y, order - 1 - x},
                              {order - 1 - x, y}}) {
                            table[rotated_y][rotated_x][0] = {(i8)y, (i8)x};
                            table[rotated_y][rotated_x][1] = {
                                (i8)x, (i8)(order - 1 - y)};
                            table[rotated_y][rotated_x][2] = {
                                (i8)(order - 1 - y), (i8)(order - 1 - x)};
                            table[rotated_y][rotated_x][3] = {
                                (i8)(order - 1 - x), (i8)(y)};
                        }
                    }
                if constexpr (order % 2 == 1) {
                    table[order / 2][order / 2][0] = {(i8)(order / 2),
                                                      (i8)(order / 2)};
                    table[order / 2][order / 2][1] = {(i8)-100, (i8)-100};
                    table[order / 2][order / 2][2] = {(i8)-100, (i8)-100};
                    table[order / 2][order / 2][3] = {(i8)-100, (i8)-100};
                }
                return table;
            }();
            const auto t = table[y][x][color.data % 4];
            return {(i8)(color.data / 4), t.y, t.x};
        } else if constexpr (
            is_same_v<decltype(color.template ComputeOriginalFaceletPosition<
                               Cube>(y, x)),
                      FaceletPosition>) {
            // ColorType 側に処理を移譲
            return color.template ComputeOriginalFaceletPosition<Cube>(y, x);
        } else {
            static_assert([] { return false; }());
        }
    }

    inline auto ComputeFaceDiff(const Cube& rhs) const {
        // TODO: 差分計算
        auto diff = 0;
        for (auto face_id = 0; face_id < 6; face_id++)
            for (auto y = 1; y < order - 1; y++)
                for (auto x = 1; x < order - 1; x++)
                    diff += Get(face_id, y, x) != rhs.Get(face_id, y, x);
        return diff;
    }

    // Kaggle フォーマットを読み取る
    inline void Read(const string& s) {
        assert(s.size() >= 6 * order * order * 2 - 1);
        auto i = 0;
        for (auto face_id = 0; face_id < 6; face_id++)
            for (auto y = 0; y < order; y++)
                for (auto x = 0; x < order; x++) {
                    Set(face_id, y, x, {s[i] - 'A'});
                    i += 2;
                }
    }

    // 面ソルバのフォーマットを読み取る
    inline void ReadNNN(const string& filename)
        requires is_same_v<ColorType, ColorType6>
    {
        auto ifs = ifstream(filename, ios::binary);
        if (!ifs.good()) {
            cerr << format("Cannot open file `{}`.", filename) << endl;
            abort();
        }
        static constexpr auto kColorMapping = array<i8, 6>{0, 5, 2, 4, 1, 3};
        for (const auto face_id : kColorMapping) {
            for (auto y = 0; y < order; y++)
                for (auto x = 0; x < order; x++) {
                    char c;
                    ifs.read(&c, 1);
                    assert(0 <= c && c < 6);
                    Set(face_id, y, x, {kColorMapping[c]});
                }
        }
    }

    // 面ソルバのフォーマットで書き出す
    inline void WriteNNN(const string& filename)
        requires is_same_v<ColorType, ColorType6>
    {
        auto ofs = ofstream(filename, ios::binary);
        if (!ofs.good()) {
            cerr << format("Cannot open file `{}`.", filename) << endl;
            abort();
        }
        static constexpr auto kColorMapping = array<i8, 6>{0, 5, 2, 4, 1, 3};
        static constexpr auto kInvColorMapping = array<i8, 6>{0, 4, 2, 5, 3, 1};
        for (const auto face_id : kColorMapping) {
            for (auto y = 0; y < order; y++)
                for (auto x = 0; x < order; x++) {
                    const auto color = Get(face_id, y, x);
                    auto c = (char)kInvColorMapping[color.data];
                    ofs.write(&c, 1);
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
                faces[D1].Get(y, x).Display(os);
            os << '\n';
        }
        for (auto y = 0; y < order; y++) {
            for (const auto face_id : {R1, F0, R0, F1}) {
                for (auto x = 0; x < order; x++)
                    faces[face_id].Get(y, x).Display(os);
                os << ' ';
            }
            os << '\n';
        }
        for (auto y = 0; y < order; y++) {
            for (auto x = 0; x < order + 1; x++)
                os << ' ';
            for (auto x = 0; x < order; x++)
                faces[D0].Get(y, x).Display(os);
            os << '\n';
        }
    }

    inline void PrintSingmaster(ostream& os = cout) const {
        // 既に辺まで揃っていることを仮定
#define TOP_ONE 0, 1
#define RIGHT_ONE 1, order - 1
#define BOTTOM_ONE order - 1, 1
#define LEFT_ONE 1, 0
        // UF
        faces[D1].Get(BOTTOM_ONE).PrintSingmaster(os);
        faces[F0].Get(TOP_ONE).PrintSingmaster(os);
        os << ' ';
        // UR
        faces[D1].Get(RIGHT_ONE).PrintSingmaster(os);
        faces[R0].Get(TOP_ONE).PrintSingmaster(os);
        os << ' ';
        // UB
        faces[D1].Get(TOP_ONE).PrintSingmaster(os);
        faces[F1].Get(TOP_ONE).PrintSingmaster(os);
        os << ' ';
        // UL
        faces[D1].Get(LEFT_ONE).PrintSingmaster(os);
        faces[R1].Get(TOP_ONE).PrintSingmaster(os);
        os << ' ';
        // DF
        faces[D0].Get(TOP_ONE).PrintSingmaster(os);
        faces[F0].Get(BOTTOM_ONE).PrintSingmaster(os);
        os << ' ';
        // DR
        faces[D0].Get(RIGHT_ONE).PrintSingmaster(os);
        faces[R0].Get(BOTTOM_ONE).PrintSingmaster(os);
        os << ' ';
        // DB
        faces[D0].Get(BOTTOM_ONE).PrintSingmaster(os);
        faces[F1].Get(BOTTOM_ONE).PrintSingmaster(os);
        os << ' ';
        // DL
        faces[D0].Get(LEFT_ONE).PrintSingmaster(os);
        faces[R1].Get(BOTTOM_ONE).PrintSingmaster(os);
        os << ' ';
        // FR
        faces[F0].Get(RIGHT_ONE).PrintSingmaster(os);
        faces[R0].Get(LEFT_ONE).PrintSingmaster(os);
        os << ' ';
        // FL
        faces[F0].Get(LEFT_ONE).PrintSingmaster(os);
        faces[R1].Get(RIGHT_ONE).PrintSingmaster(os);
        os << ' ';
        // BR
        faces[F1].Get(LEFT_ONE).PrintSingmaster(os);
        faces[R0].Get(RIGHT_ONE).PrintSingmaster(os);
        os << ' ';
        // BL
        faces[F1].Get(RIGHT_ONE).PrintSingmaster(os);
        faces[R1].Get(LEFT_ONE).PrintSingmaster(os);
        os << ' ';
#undef TOP_ONE
#undef RIGHT_ONE
#undef BOTTOM_ONE
#undef LEFT_ONE
#define TOP_LEFT 0, 0
#define TOP_RIGHT 0, order - 1
#define BOTTOM_LEFT order - 1, 0
#define BOTTOM_RIGHT order - 1, order - 1
        // UFR
        faces[D1].Get(BOTTOM_RIGHT).PrintSingmaster(os);
        faces[F0].Get(TOP_RIGHT).PrintSingmaster(os);
        faces[R0].Get(TOP_LEFT).PrintSingmaster(os);
        os << ' ';
        // URB
        faces[D1].Get(TOP_RIGHT).PrintSingmaster(os);
        faces[R0].Get(TOP_RIGHT).PrintSingmaster(os);
        faces[F1].Get(TOP_LEFT).PrintSingmaster(os);
        os << ' ';
        // UBL
        faces[D1].Get(TOP_LEFT).PrintSingmaster(os);
        faces[F1].Get(TOP_RIGHT).PrintSingmaster(os);
        faces[R1].Get(TOP_LEFT).PrintSingmaster(os);
        os << ' ';
        // ULF
        faces[D1].Get(BOTTOM_LEFT).PrintSingmaster(os);
        faces[R1].Get(TOP_RIGHT).PrintSingmaster(os);
        faces[F0].Get(TOP_LEFT).PrintSingmaster(os);
        os << ' ';
        // DRF
        faces[D0].Get(TOP_RIGHT).PrintSingmaster(os);
        faces[R0].Get(BOTTOM_LEFT).PrintSingmaster(os);
        faces[F0].Get(BOTTOM_RIGHT).PrintSingmaster(os);
        os << ' ';
        // DFL
        faces[D0].Get(TOP_LEFT).PrintSingmaster(os);
        faces[F0].Get(BOTTOM_LEFT).PrintSingmaster(os);
        faces[R1].Get(BOTTOM_RIGHT).PrintSingmaster(os);
        os << ' ';
        // DLB
        faces[D0].Get(BOTTOM_LEFT).PrintSingmaster(os);
        faces[R1].Get(BOTTOM_LEFT).PrintSingmaster(os);
        faces[F1].Get(BOTTOM_RIGHT).PrintSingmaster(os);
        os << ' ';
        // DBR
        faces[D0].Get(BOTTOM_RIGHT).PrintSingmaster(os);
        faces[F1].Get(BOTTOM_LEFT).PrintSingmaster(os);
        faces[R0].Get(BOTTOM_RIGHT).PrintSingmaster(os);
#undef TOP_LEFT
#undef TOP_RIGHT
#undef BOTTOM_LEFT
#undef BOTTOM_RIGHT
    }
};

// kaggleの入力を読む
tuple<int, bool, Formula> ReadKaggleInput(const string& filename_puzzles,
                                          const string& filename_sample,
                                          const int id) {
    // return {puzzle_size, is_normal, sample_formula}
    if (id < 0 || id > 284) {
        cerr << "id must be in [0, 284]" << endl;
        abort();
    }

    // id+1 行目を読む
    auto ifs_puzzles = ifstream(filename_puzzles);
    if (!ifs_puzzles.good()) {
        cerr << format("Cannot open file `{}`.", filename_puzzles) << endl;
        abort();
    }
    auto ifs_sample = ifstream(filename_sample);
    if (!ifs_sample.good()) {
        cerr << format("Cannot open file `{}`.", filename_sample) << endl;
        abort();
    }
    string s_puzzles, s_sample;
    for (auto i = 0; i <= id + 1; i++)
        getline(ifs_puzzles, s_puzzles);
    for (auto i = 0; i <= id + 1; i++)
        getline(ifs_sample, s_sample);

    string puzzle_type;
    string s_formula;
    string solution_state;
    int puzzle_size = 0;
    bool is_normal;
    Formula sample_formula;

    // s_puzzles = id,puzzle_type,solution_state,scramble_state
    // s_sample = id,formula
    // puzzle_type = "cube_2/2/2", "cube_3/3/3", ... "cube_33/33/33"
    stringstream ss_puzzles(s_puzzles);
    stringstream ss_sample(s_sample);
    string token;
    getline(ss_puzzles, token, ',');
    assert(stoi(token) == id);
    if (stoi(token) != id) {
        cerr << "id mismatch" << endl;
        exit(1);
    }
    getline(ss_puzzles, puzzle_type, ',');
    stringstream ss_puzzle_type(puzzle_type);
    getline(ss_puzzle_type, token, '/');
    getline(ss_puzzle_type, token, '/');
    puzzle_size = stoi(token);
    getline(ss_puzzles, solution_state, '/');
    {
        is_normal = true;
        cerr << solution_state << endl;
        if (solution_state[0] == 'N')
            is_normal = false;
        if (solution_state[2] == 'B' && puzzle_size % 2 == 0)
            is_normal = false;
    }
    getline(ss_sample, token, ',');
    getline(ss_sample, token, ',');
    sample_formula = Formula(token);

    cout << "puzzle_type: " << puzzle_type << endl;
    cout << "puzzle_size: " << puzzle_size << endl;
    cout << "is_normal: " << is_normal << endl;
    // sample_formula.Print(cerr);

    return {puzzle_size, is_normal, sample_formula};
}

[[maybe_unused]] static void TestCube() {
    // パリティ解消の辺の手筋
    constexpr auto kOrder = 7;
    const auto moves = {
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

    const auto formula = Formula(moves);
    formula.Print();
    cout << endl;
    formula.Display<Cube<kOrder, ColorType24>>();
}

// clang++ -std=c++20 -Wall -Wextra -O3 cube.cpp -DTEST_CUBE
#ifdef TEST_CUBE
int main() { TestCube(); }
#endif
