#ifndef _BOARD_
#define _BOARD_
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <cstring>
#include <sstream>
#include <chrono>


#define BOARD_SIZE 8
#define BOARD_AREA 64
#define MOVE_PASS BOARD_AREA
#define N_PLAYER 2
#define BLACK 0
#define WHITE 1
#define R_A 0
#define R_B 1
#define R_C 2
#define R_D 3
#define R_E 4
#define R_F 5
#define R_G 6
#define R_H 7
#define C_1 0
#define C_2 8
#define C_3 16
#define C_4 24
#define C_5 32
#define C_6 40
#define C_7 48
#define C_8 56

using namespace std;

using BoardPlane = uint64_t;

inline BoardPlane position_plane(int pos)
{
    return 1ULL << pos;
}

inline bool move_is_pass(int move)
{
    return move == MOVE_PASS;
}

class UndoInfo
{
public:
    BoardPlane planes[N_PLAYER];
    int turn; // BLACK / WHITE
    int pass_count;
};


inline string move_to_str(int move)
{
    if (move_is_pass(move))
    {
        return "pass";
    }
    char m[3];
    m[2] = 0;
    m[0] = 'a' + move % BOARD_SIZE;
    m[1] = '1' + move / BOARD_SIZE;
    return string(m);
}

inline int move_from_str(const string &move_str)
{
    if (move_str[0] == 'p')
    {
        return MOVE_PASS;
    }
    return (move_str[0] - 'a') + (move_str[1] - '1') * BOARD_SIZE;
}

class Board
{
    BoardPlane planes[N_PLAYER];
    int _turn; // BLACK / WHITE
    int _pass_count; // 連続パス回数

public:
    Board() {
        set_hirate();
    };

    void set(const Board &other)
    {
        planes[0] = other.planes[0];
        planes[1] = other.planes[1];
        _turn = other._turn;
        _pass_count = other._pass_count;
    }

    int turn() const
    {
        return _turn;
    }

    void set_hirate()
    {
        planes[0] = planes[1] = 0;
        planes[WHITE] |= position_plane(R_D + C_4);
        planes[BLACK] |= position_plane(R_E + C_4);
        planes[BLACK] |= position_plane(R_D + C_5);
        planes[WHITE] |= position_plane(R_E + C_5);
        _turn = BLACK;
        _pass_count = 0;
    }

    void set_position_codingame(const vector<string> &lines, int turn)
    {
        // lines[0]: a1 to h1, '.' = nothing, '0' = black, '1' = white
        planes[0] = planes[1] = 0;
        for (int row = 0; row < BOARD_SIZE; row++)
        {
            for (int col = 0; col < BOARD_SIZE; col++)
            {
                char c = lines[row][col];
                if (c == '.')
                {
                    continue;
                }
                int color = c - '0';
                planes[color] |= position_plane(row * BOARD_SIZE + col);
            }
        }
        // 相手が最後にパスしている可能性もあるが、codingameで指し手を求められるのは合法手がある場合のみであり、指し手生成に影響なし。
        this->_turn = turn;
        _pass_count = 0;
    }

    // 盤面を表現する、- (なし), O (白), X (黒)を64文字並べた文字列を返す
    string get_position_string()
    {
        char str[BOARD_AREA + 1];
        str[BOARD_AREA] = '\0';
        int i = 0;
        for (int row = 0; row < BOARD_SIZE; row++)
        {
            for (int col = 0; col < BOARD_SIZE; col++)
            {
                char c = '-';
                if (planes[BLACK] & position_plane(i))
                {
                    c = 'X';
                }
                else if (planes[WHITE] & position_plane(i))
                {
                    c = 'O';
                }
                str[i++] = c;
            }
        }

        return string(str);
    }

    // get_position_string()の末尾に、" b"(黒の手番)または" w"(白の手番)を付加した文字列を返す
    string get_position_string_with_turn()
    {
        string posstr = get_position_string();
        if (this->turn() == BLACK)
        {
            return posstr + " b";
        }
        else
        {
            return posstr + " w";
        }
    }

    // get_position_string()の結果を読み取る
    void set_position_string(const string &position, int turn)
    {
        // 注意: パス情報については保存できない
        const char *pchars = position.c_str();
        planes[0] = planes[1] = 0;
        int i = 0;
        for (int row = 0; row < BOARD_SIZE; row++)
        {
            for (int col = 0; col < BOARD_SIZE; col++)
            {
                char c = pchars[i];
                if (c == 'X')
                {
                    planes[BLACK] |= position_plane(i);
                }
                else if (c == 'O')
                {
                    planes[WHITE] |= position_plane(i);
                }
                i++;
            }
        }

        this->_turn = turn;
        _pass_count = 0;
    }

    // get_position_string_with_turn()の結果を読み取る
    void set_position_string_with_turn(const string &position_with_turn)
    {
        char turn_char = position_with_turn[BOARD_AREA + 1];
        set_position_string(position_with_turn, turn_char == 'b' ? BLACK : WHITE); // 余分な文字がついていても先頭64文字しか見ない
    }

    void do_move(int move, UndoInfo &undo_info)
    {
        undo_info.planes[0] = planes[0];
        undo_info.planes[1] = planes[1];
        undo_info.turn = _turn;
        undo_info.pass_count = _pass_count;
        if (!move_is_pass(move))
        {
            BoardPlane player = planes[_turn], opponent = planes[1 - _turn], position = position_plane(move);
            BoardPlane reverse_plane = reverse(player, opponent, position);
            planes[_turn] = player ^ position ^ reverse_plane;
            planes[1 - _turn] = opponent ^ reverse_plane;
            _pass_count = 0;
        }
        else
        {
            _pass_count++;
        }
        _turn = 1 - _turn;
    }

    void undo_move(const UndoInfo &undo_info)
    {
        planes[0] = undo_info.planes[0];
        planes[1] = undo_info.planes[1];
        _turn = undo_info.turn;
        _pass_count = undo_info.pass_count;
    }

    // 合法手を列挙する。
    void legal_moves(vector<int> &move_list, bool with_pass = false) const
    {
        // ビットボード参考 https://zenn.dev/kinakomochi/articles/othello-bitboard
        move_list.clear();
        BoardPlane bb;
        legal_moves_bb(bb);
        for (int pos = 0; pos < BOARD_AREA; pos++)
        {
            if (bb & position_plane(pos))
            {
                move_list.push_back(pos);
            }
        }

        if (with_pass && move_list.empty())
        {
            move_list.push_back(MOVE_PASS);
        }
    }

    void legal_moves_bb(BoardPlane &result) const
    {
        result = 0;
        BoardPlane buffer;
        BoardPlane player = planes[_turn], opponent = planes[1 - _turn];
        legal_calc(result, buffer, player, opponent, 0x7e7e7e7e7e7e7e7e, 1);
        legal_calc(result, buffer, player, opponent, 0x007e7e7e7e7e7e00, 7);
        legal_calc(result, buffer, player, opponent, 0x00ffffffffffff00, 8);
        legal_calc(result, buffer, player, opponent, 0x007e7e7e7e7e7e00, 9);

        result &= ~(player | opponent);
    }

    bool can_move(int move) const
    {
        BoardPlane player = planes[_turn], opponent = planes[1 - _turn], position = position_plane(move);
        if ((player | opponent) & position)
        {
            // すでにある場所には置けない
            return false;
        }
        BoardPlane reverse_plane = reverse(player, opponent, position);
        // ひっくりかえせない場所には置けない
        return reverse_plane ? true : false;
    }

    bool is_end() const
    {
        if (_pass_count == 2)
        {
            return true;
        }
        if (~(planes[0] | planes[1]) == 0)
        {
            return true;
        }
        return false;
    }

    int count_stone(int color) const
    {
        return __builtin_popcountll(planes[color]);
    }

    // 石の数の差。黒-白。
    int count_stone_diff() const
    {
        // when is_end(), this function is used for checking winner
        // ret > 0: BLACK wins, ret < 0: WHITE wins, ret == 0: draw
        return count_stone(BLACK) - count_stone(WHITE);
    }

    string pretty_print() const
    {
        string s;
        s.append(" |abcdefgh\n");
        s.append("-+--------\n");
        for (int row = 0; row < BOARD_SIZE; row++)
        {
            s.append(to_string(row + 1));
            s.append("|");
            for (int col = 0; col < BOARD_SIZE; col++)
            {
                int p = row * BOARD_SIZE + col;
                if (planes[BLACK] & position_plane(p))
                {
                    s.append("x");
                }
                else if (planes[WHITE] & position_plane(p))
                {
                    s.append("o");
                }
                else
                {
                    s.append(".");
                }
            }
            s.append("\n");
        }
        return s;
    }

private:
    template <typename ShiftFunc>
    void line(BoardPlane &result, BoardPlane position, BoardPlane mask, ShiftFunc shift, int n) const
    {
        result = mask & shift(position, n);
        result |= mask & shift(result, n);
        result |= mask & shift(result, n);
        result |= mask & shift(result, n);
        result |= mask & shift(result, n);
        result |= mask & shift(result, n);
    }

    void legal_calc(BoardPlane &result, BoardPlane &buffer, BoardPlane player, BoardPlane opponent, BoardPlane mask, int n) const
    {
        BoardPlane _mask = opponent & mask;
        line(
            buffer, player, _mask, [](BoardPlane p, int c)
            { return p << c; },
            n);
        result |= buffer << n;
        line(
            buffer, player, _mask, [](BoardPlane p, int c)
            { return p >> c; },
            n);
        result |= buffer >> n;
    }

    void reverse_calc(BoardPlane &result, BoardPlane &buffer, BoardPlane s, BoardPlane opponent, BoardPlane position, BoardPlane mask, int n) const
    {
        BoardPlane _mask = opponent & mask;
        line(
            buffer, position, _mask, [](BoardPlane p, int c)
            { return p << c; },
            n);
        if (s & (buffer << n))
        {
            result |= buffer;
        }
        line(
            buffer, position, _mask, [](BoardPlane p, int c)
            { return p >> c; },
            n);
        if (s & (buffer >> n))
        {
            result |= buffer;
        }
    }

    BoardPlane reverse(BoardPlane player, BoardPlane opponent, BoardPlane position) const
    {
        BoardPlane result = 0;
        BoardPlane buffer;
        reverse_calc(result, buffer, player, opponent, position, 0x7e7e7e7e7e7e7e7e, 1);
        reverse_calc(result, buffer, player, opponent, position, 0x007e7e7e7e7e7e00, 7);
        reverse_calc(result, buffer, player, opponent, position, 0x00ffffffffffff00, 8);
        reverse_calc(result, buffer, player, opponent, position, 0x007e7e7e7e7e7e00, 9);
        return result;
    }
};

#endif