#include <algorithm>
#include <chrono>
#include <cmath>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <random>
#include <set>
#include <vector>

#include <raylib.h>
#include <raymath.h>

using namespace std;

struct Point {
  int x_ = 0;
  int y_ = 0;
  int y() const { return y_; }
  int x() const { return x_; }
  void setX(int x) { x_ = x; }
  void setY(int y) { y_ = y; }
  friend Point operator-(const Point &a, const Point &b);
  Point &operator-=(const Point &a) {
    *this = *this - a;
    return *this;
  }
};

Point operator-(const Point &a, const Point &b) {
  return Point{a.x_ - b.x_, a.y_ - b.y_};
}

using Pattern = std::vector<Point>;

struct SizedPattern {
  Pattern pat;
  int w;
  int h;
};

inline Pattern shift(Pattern &p) {
  int minX = std::numeric_limits<int>::max();
  int minY = std::numeric_limits<int>::max();
  for (Point &pt : p) {
    minX = std::min(pt.x(), minX);
    minY = std::min(pt.y(), minY);
  }
  Pattern res(p);
  for (Point &pt : res) {
    pt -= Point{minX, minY};
  }
  return res;
}

inline std::vector<Pattern> rotations(Pattern &p) {
  std::vector<Pattern> res(4);
  res[0] = p;
  for (int i = 1; i < 4; ++i) {
    Pattern rotated(p.size());
    for (auto j = 0u; j < p.size(); ++j) {
      rotated[j].setX(res[i - 1][j].y());
      rotated[j].setY(-res[i - 1][j].x());
    }
    res[i] = shift(rotated);
  }
  return res;
}

inline Pattern mirrored(Pattern &p) {
  Pattern m(p);
  for (auto i = 0u; i < p.size(); ++i) {
    m[i].setY(-p[i].y());
  }
  return shift(m);
}

inline SizedPattern sized(Pattern &p) {
  SizedPattern res;
  res.pat = p;
  int maxX = std::numeric_limits<int>::min();
  int maxY = std::numeric_limits<int>::min();
  for (Point &pt : p) {
    maxX = std::max(pt.x(), maxX);
    maxY = std::max(pt.y(), maxY);
  }
  res.w = maxX + 1;
  res.h = maxY + 1;
  return res;
}

inline std::vector<SizedPattern> generate(Pattern p, bool symmetric = false) {
  auto s = rotations(p);
  std::vector<Pattern> res1;
  if (!symmetric) {
    Pattern m = mirrored(p);
    auto r = rotations(m);
    res1.reserve(s.size() + r.size());
    std::copy(r.begin(), r.end(), std::back_inserter(res1));
  } else {
    res1.reserve(s.size());
  }
  std::copy(s.begin(), s.end(), std::back_inserter(res1));
  std::vector<SizedPattern> res2;
  res2.reserve(res1.size());
  std::transform(res1.begin(), res1.end(), std::back_inserter(res2),
                 [](Pattern &pt) { return sized(pt); });
  return res2;
}

const Pattern three_p_1 = {{0, 0}, {1, 1}, {0, 2}};
const Pattern three_p_2 = {{1, 0}, {0, 1}, {0, 2}};
const Pattern three_p_3 = {{0, 0}, {0, 1}, {0, 3}};
const Pattern four_p = {{0, 0}, {1, 1}, {0, 2}, {0, 3}};
const Pattern five_p_1 = {{0, 0}, {0, 1}, {1, 2}, {0, 3}, {0, 4}};
const Pattern five_p_2 = {{0, 0}, {1, 1}, {1, 2}, {2, 0}, {3, 0}};

const std::vector<SizedPattern> threes1 = generate(three_p_1);
const std::vector<SizedPattern> threes2 = generate(three_p_2);
const std::vector<SizedPattern> threes3 = generate(three_p_3);
const std::vector<SizedPattern> fours = generate(four_p);
const std::vector<SizedPattern> fives1 = generate(five_p_1);
const std::vector<SizedPattern> fives2 = generate(five_p_2);
const std::vector<SizedPattern> threes = []() {
  std::vector<SizedPattern> res;
  res.reserve(threes1.size() + threes2.size() + threes3.size());
  res.insert(res.end(), threes1.begin(), threes1.end());
  res.insert(res.end(), threes2.begin(), threes2.end());
  res.insert(res.end(), threes3.begin(), threes3.end());
  return res;
}();
const std::vector<SizedPattern> patterns = []() {
  std::vector<SizedPattern> res;
  res.reserve(fours.size() + fives1.size() + fives2.size());
  res.insert(res.end(), fours.begin(), fours.end());
  res.insert(res.end(), fives1.begin(), fives1.end());
  res.insert(res.end(), fives2.begin(), fives2.end());
  return res;
}();

class Board {
  std::vector<int> board;
  std::default_random_engine e1{static_cast<unsigned>(
      std::chrono::system_clock::now().time_since_epoch().count())};
  std::uniform_int_distribution<int> uniform_dist{1, 6};
  std::uniform_int_distribution<int> uniform_dist_2;
  std::uniform_int_distribution<int> uniform_dist_3;
  std::uniform_int_distribution<int> coin{1, 42};
  std::uniform_int_distribution<int> coin2{1, 69};

  size_t w;
  size_t h;
  std::set<std::pair<int, int>> matched_patterns;
  std::set<std::pair<int, int>> matched_threes;
  std::set<std::pair<int, int>> magic_tiles;
  std::set<std::pair<int, int>> magic_tiles2;
  std::vector<std::tuple<int, int, int>> rm_i;
  std::vector<std::tuple<int, int, int>> rm_j;
  std::vector<std::pair<int, int>> rm_b;

public:
  int width() { return w; }
  int height() { return h; }
  int score{};
  int normals{};
  int longers{};
  int longests{};
  int crosses{};
  Board(size_t _w, size_t _h) : w{_w}, h{_h} {
    board.resize(w * h);
    std::fill(std::begin(board), std::end(board), 0);
    uniform_dist_2 = std::uniform_int_distribution<int>(0, w);
    uniform_dist_3 = std::uniform_int_distribution<int>(0, h);
  }
  Board(const Board &b) {
    w = b.w;
    h = b.h;
    board = b.board;
    score = b.score;
    matched_patterns = b.matched_patterns;
    magic_tiles = b.magic_tiles;
    magic_tiles2 = b.magic_tiles2;
  }
  Board operator=(const Board &b) {
    w = b.w;
    h = b.h;
    board = b.board;
    score = b.score;
    matched_patterns = b.matched_patterns;
    magic_tiles = b.magic_tiles;
    magic_tiles2 = b.magic_tiles2;
    return *this;
  }
  friend bool operator==(const Board &a, const Board &b);
  friend std::ostream &operator<<(std::ostream &of, const Board &b);
  friend std::istream &operator>>(std::istream &in, Board &b);
  bool match_pattern(int x, int y, const SizedPattern &p) {
    int color = at(x + p.pat[0].x(), y + p.pat[0].y());
    for (auto i = 1u; i < p.pat.size(); ++i) {
      if (color != at(x + p.pat[i].x(), y + p.pat[i].y())) {
        return false;
      }
    }
    return true;
  }
  void match_patterns() {
    matched_patterns.clear();
    for (const SizedPattern &sp : patterns) {
      for (int i = 0; i <= w - sp.w; ++i) {
        for (int j = 0; j <= h - sp.h; ++j) {
          if (match_pattern(i, j, sp)) {
            for (const Point &p : sp.pat) {
              matched_patterns.insert({i + p.x(), j + p.y()});
            }
          }
        }
      }
    }
  }
  bool is_matched(int x, int y) { return matched_patterns.contains({x, y}); }
  bool is_magic(int x, int y) { return magic_tiles.contains({x, y}); }
  bool is_magic2(int x, int y) { return magic_tiles2.contains({x, y}); }
  void swap(int x1, int y1, int x2, int y2) {
    auto tmp = at(x1, y1);
    at(x1, y1) = at(x2, y2);
    at(x2, y2) = tmp;

    if (is_magic(x1, y1)) {
      magic_tiles.erase({x1, y1});
      magic_tiles.insert({x2, y2});
    }

    if (is_magic(x2, y2)) {
      magic_tiles.erase({x2, y2});
      magic_tiles.insert({x1, y1});
    }

    if (is_magic2(x1, y1)) {
      magic_tiles2.erase({x1, y1});
      magic_tiles2.insert({x2, y2});
    }

    if (is_magic2(x2, y2)) {
      magic_tiles2.erase({x2, y2});
      magic_tiles2.insert({x1, y1});
    }
  }
  void fill() {
    score = 0;
    for (auto &x : board) {
      x = uniform_dist(e1);
    }
  }
  int &at(int a, int b) { return board[a * h + b]; }
  int at(int a, int b) const { return board[a * h + b]; }
  bool reasonable_coord(int i, int j) {
    return i >= 0 && i < w && j >= 0 && j < h;
  }
  void remove_trios() {
    std::vector<std::tuple<int, int, int>> remove_i;
    std::vector<std::tuple<int, int, int>> remove_j;
    for (int i = 0; i < w; ++i) {
      for (int j = 0; j < h; ++j) {
        int offset_j = 1;
        int offset_i = 1;
        while (j + offset_j < h && at(i, j) == at(i, j + offset_j)) {
          offset_j += 1;
        }
        if (offset_j > 2) {
          remove_i.push_back({i, j, offset_j});
        }
        while (i + offset_i < w && at(i, j) == at(i + offset_i, j)) {
          offset_i += 1;
        }
        if (offset_i > 2) {
          remove_j.push_back({i, j, offset_i});
        }
      }
    }
    for (auto t : remove_i) {
      int i = std::get<0>(t);
      int j = std::get<1>(t);
      int offset = std::get<2>(t);
      if (offset == 4) {
        j = 0;
        offset = h;
        longers += 1;
        normals = std::max(0, normals - 1);
      }
      for (int jj = j; jj < j + offset; ++jj) {
        at(i, jj) = 0;
        if (is_magic(i, jj)) {
          score -= 3;
          magic_tiles.erase({i, jj});
        }
        if (is_magic2(i, jj)) {
          score += 3;
          magic_tiles2.erase({i, jj});
        }
        score += 1;
      }
      if (offset == 5) {
        for (int i = 0; i < w; ++i) {
          at(uniform_dist_2(e1), uniform_dist_3(e1)) = 0;
          score += 1;
        }
        longests += 1;
        normals = std::max(0, normals - 1);
      }
      normals += 1;
    }
    for (auto t : remove_j) {
      int i = std::get<0>(t);
      int j = std::get<1>(t);
      int offset = std::get<2>(t);
      if (offset == 4) {
        i = 0;
        offset = w;
        longers += 1;
        normals = std::max(0, normals - 1);
      }
      for (int ii = i; ii < i + offset; ++ii) {
        at(ii, j) = 0;
        if (is_magic(ii, j)) {
          score -= 3;
          magic_tiles.erase({ii, j});
        }
        if (is_magic2(ii, j)) {
          score += 3;
          magic_tiles2.erase({ii, j});
        }
        score += 1;
      }
      if (offset == 5) {
        for (int i = 0; i < w; ++i) {
          at(uniform_dist_2(e1), uniform_dist_3(e1)) = 0;
          score += 1;
        }
        longests += 1;
        normals = std::max(0, normals - 1);
      }
      normals += 1;
    }
    for (int i = 0; i < int(remove_i.size()); ++i) {
      for (int j = 0; j < int(remove_j.size()); ++j) {
        auto t1 = remove_i[i];
        auto t2 = remove_j[j];
        auto i1 = std::get<0>(t1);
        auto j1 = std::get<1>(t1);
        auto o1 = std::get<2>(t1);
        auto i2 = std::get<0>(t2);
        auto j2 = std::get<1>(t2);
        auto o2 = std::get<2>(t2);
        if (i1 >= i2 && i1 < (i2 + o2) && j2 >= j1 && j2 < (j1 + o1)) {
          for (int m = -1; m < 2; ++m) {
            for (int n = -1; n < 2; ++n) {
              if (reasonable_coord(i1 + m, j1 + n)) {
                at(i1 + m, j1 + n) = 0;
                score += 1;
              }
            }
          }
          crosses += 1;
          normals = std::max(0, normals - 2);
        }
      }
    }
  }
  void fill_up() {
    int curr_i = -1;
    for (int i = 0; i < w; ++i) {
      for (int j = 0; j < h; ++j) {
        if (at(i, j) == 0) {
          curr_i = i;
          while (curr_i < w - 1 && at(curr_i + 1, j) == 0) {
            curr_i += 1;
          }
          for (int k = curr_i; k >= 0; --k) {
            if (at(k, j) != 0) {
              at(curr_i, j) = at(k, j);
              if (is_magic(k, j)) {
                magic_tiles.erase({k, j});
                magic_tiles.insert({curr_i, j});
              }
              if (is_magic2(k, j)) {
                magic_tiles2.erase({k, j});
                magic_tiles2.insert({curr_i, j});
              }
              curr_i -= 1;
            }
          }
          for (int k = curr_i; k >= 0; --k) {
            at(k, j) = uniform_dist(e1);
            if (coin(e1) == 1) {
              magic_tiles.insert({k, j});
            }
            if (coin2(e1) == 1) {
              magic_tiles2.insert({k, j});
            }
          }
        }
      }
    }
  }
  void stabilize() {
    auto old_board = *this;
    do {
      old_board = *this;
      remove_trios();
      fill_up();
    } while (!(*this == old_board));
    match_patterns();
    match_threes();
  }
  void step() {
    remove_trios();
    fill_up();
    match_patterns();
    match_threes();
  }
  void zero() {
    score = 0;
    normals = 0;
    longers = 0;
    longests = 0;
    crosses = 0;
  }
  // New interface starts here
  std::vector<std::tuple<int, int, int>> remove_one_thing() {
    std::vector<std::tuple<int, int, int>> res;
    if (!rm_i.empty()) {
      auto t = rm_i.back();
      int i = std::get<0>(t);
      int j = std::get<1>(t);
      int offset = std::get<2>(t);
      if (offset == 4) {
        j = 0;
        offset = h;
        longers += 1;
        normals = std::max(0, normals - 1);
      }
      for (int jj = j; jj < j + offset; ++jj) {
        res.emplace_back(i, jj, at(i, jj));
        at(i, jj) = 0;
        if (is_magic(i, jj)) {
          score -= 3;
          magic_tiles.erase({i, jj});
        }
        if (is_magic2(i, jj)) {
          score += 3;
          magic_tiles2.erase({i, jj});
        }
        score += 1;
      }
      if (offset == 5) {
        std::set<std::pair<int, int>> r;
        for (int i = 0; i < w; ++i) {
          int x, y;
          do {
            x = uniform_dist_2(e1);
            y = uniform_dist_3(e1);
          } while (r.contains({x, y}));
          r.insert({x, y});
          res.emplace_back(x, y, at(x, y));
          at(x, y) = 0;
          score += 1;
        }
        longests += 1;
        normals = std::max(0, normals - 1);
      }
      normals += 1;
      rm_i.pop_back();
      return res;
    }
    if (!rm_j.empty()) {
      auto t = rm_j.back();
      int i = std::get<0>(t);
      int j = std::get<1>(t);
      int offset = std::get<2>(t);
      if (offset == 4) {
        i = 0;
        offset = w;
        longers += 1;
        normals = std::max(0, normals - 1);
      }
      for (int ii = i; ii < i + offset; ++ii) {
        res.emplace_back(ii, j, at(ii, j));
        at(ii, j) = 0;
        if (is_magic(ii, j)) {
          score -= 3;
          magic_tiles.erase({ii, j});
        }
        if (is_magic2(ii, j)) {
          score += 3;
          magic_tiles2.erase({ii, j});
        }
        score += 1;
      }
      if (offset == 5) {
        std::set<std::pair<int, int>> r;
        for (int i = 0; i < w; ++i) {
          int x, y;
          do {
            x = uniform_dist_2(e1);
            y = uniform_dist_3(e1);
          } while (r.contains({x, y}));
          r.insert({x, y});
          res.emplace_back(x, y, at(x, y));
          at(x, y) = 0;
          score += 1;
        }
        longests += 1;
        normals = std::max(0, normals - 1);
      }
      normals += 1;
      rm_j.pop_back();
      return res;
    }
    if (!rm_b.empty()) {
      auto t = rm_b.back();
      int i = std::get<0>(t);
      int j = std::get<1>(t);
      for (int m = -2; m < 3; ++m) {
        for (int n = -2; n < 3; ++n) {
          if (reasonable_coord(i + m, j + n)) {
            res.emplace_back(i + m, j + n, at(i + m, j + n));
            at(i + m, j + n) = 0;
            score += 1;
          }
        }
      }
      crosses += 1;
      normals = std::max(0, normals - 2);
      rm_b.pop_back();
      return res;
    }
    return res;
  }
  void prepare_removals() {
    rm_i.clear();
    rm_j.clear();
    rm_b.clear();
    matched_patterns.clear();
    matched_threes.clear();
    for (int i = 0; i < w; ++i) {
      for (int j = 0; j < h; ++j) {
        int offset_j = 1;
        int offset_i = 1;
        while (j + offset_j < h && at(i, j) == at(i, j + offset_j)) {
          offset_j += 1;
        }
        if (offset_j > 2) {
          rm_i.emplace_back(i, j, offset_j);
        }
        while (i + offset_i < w && at(i, j) == at(i + offset_i, j)) {
          offset_i += 1;
        }
        if (offset_i > 2) {
          rm_j.emplace_back(i, j, offset_i);
        }
      }
    }
    for (int i = 0; i < int(rm_i.size()); ++i) {
      for (int j = 0; j < int(rm_j.size()); ++j) {
        auto t1 = rm_i[i];
        auto t2 = rm_j[j];
        auto i1 = std::get<0>(t1);
        auto j1 = std::get<1>(t1);
        auto o1 = std::get<2>(t1);
        auto i2 = std::get<0>(t2);
        auto j2 = std::get<1>(t2);
        auto o2 = std::get<2>(t2);
        if (i1 >= i2 && i1 < (i2 + o2) && j2 >= j1 && j2 < (j1 + o1)) {
          rm_b.emplace_back(i1, j2);
        }
      }
    }
    auto sorter = [](auto &t1, auto &t2) {
      auto i1 = std::get<0>(t1);
      auto i2 = std::get<0>(t2);
      return i1 > i2;
    };
    std::sort(std::begin(rm_i), std::end(rm_i), sorter);
    std::sort(std::begin(rm_j), std::end(rm_j), sorter);
    std::sort(std::begin(rm_b), std::end(rm_b), sorter);
  }
  bool has_removals() { return rm_i.size() + rm_j.size() + rm_b.size(); }
  void match_threes() {
    matched_threes.clear();
    for (const SizedPattern &sp : threes) {
      for (int i = 0; i <= w - sp.w; ++i) {
        for (int j = 0; j <= h - sp.h; ++j) {
          if (match_pattern(i, j, sp)) {
            for (const Point &p : sp.pat) {
              matched_threes.insert({i + p.x(), j + p.y()});
            }
          }
        }
      }
    }
  }
  bool is_three(int i, int j) { return matched_threes.contains({i, j}); }
};

bool operator==(const Board &a, const Board &b) {
  if (a.w != b.w || a.h != b.h) {
    return false;
  }
  for (int i = 0; i < a.w * a.h; ++i) {
    if (a.board[i] != b.board[i]) {
      return false;
    }
  }
  return true;
}

std::ostream &operator<<(std::ostream &of, const Board &b) {
  of << b.score << "\n"
     << b.normals << "\n"
     << b.longers << "\n"
     << b.longests << "\n"
     << b.crosses << "\n"
     << b.w << " " << b.h << "\n";
  for (int i = 0; i < b.w; ++i) {
    for (int j = 0; j < b.h; ++j) {
      of << b.at(i, j) << " ";
    }
    of << std::endl;
  }
  of << b.magic_tiles.size() << "\n";
  for (auto it = b.magic_tiles.begin(); it != b.magic_tiles.end(); ++it) {
    of << it->first << " " << it->second << " ";
  }
  of << "\n";
  of << b.magic_tiles2.size() << "\n";
  for (auto it = b.magic_tiles2.begin(); it != b.magic_tiles2.end(); ++it) {
    of << it->first << " " << it->second << " ";
  }
  of << "\n";
  return of;
}

std::istream &operator>>(std::istream &in, Board &b) {
  in >> b.score >> b.normals >> b.longers >> b.longests >> b.crosses >> b.w >>
      b.h;
  b.board.resize(b.w * b.h);
  for (int i = 0; i < b.w; ++i) {
    for (int j = 0; j < b.h; ++j) {
      in >> b.at(i, j);
    }
  }
  int s;
  in >> s;
  b.magic_tiles.clear();
  int i, j;
  for (int k = 0; k < s; ++k) {
    in >> i >> j;
    b.magic_tiles.insert({i, j});
  }
  in >> s;
  for (int k = 0; k < s; ++k) {
    in >> i >> j;
    b.magic_tiles2.insert({i, j});
  }
  return in;
}

using Leaderboard = std::vector<std::pair<std::string, int>>;

Leaderboard ReadLeaderboard() {
  Leaderboard res;
  std::ifstream input("leaderboard.txt");
  std::string line;
  while (std::getline(input, line)) {
    auto idx = line.find(';');
    if (idx != std::string::npos) {
      std::string name = line.substr(0, idx);
      int score = std::stoi(line.substr(idx + 1));
      res.emplace_back(name, score);
    }
  }
  std::sort(std::begin(res), std::end(res),
            [](auto &a, auto &b) { return a.second > b.second; });
  return res;
}

void WriteLeaderboard(Leaderboard leaderboard) {
  std::string text;
  std::sort(std::begin(leaderboard), std::end(leaderboard),
            [](auto &a, auto &b) { return a.second > b.second; });
  for (auto it : leaderboard) {
    text += fmt::format("{};{}\n", it.first, it.second);
  }
  std::ofstream output("leaderboard.txt");
  output << text;
}

void DrawLeaderboard(Leaderboard leaderboard, size_t offset = 0) {
  auto w = GetRenderWidth();
  auto h = GetRenderHeight();
  DrawRectangle(w / 4, h / 4, w / 2, h / 2, WHITE);
  std::string text = "Leaderboard:\n";
  std::sort(std::begin(leaderboard), std::end(leaderboard),
            [](auto &a, auto &b) { return a.second > b.second; });
  offset = std::min(offset, leaderboard.size() - 1);
  auto finish = std::min(offset + 9, leaderboard.size());
  for (auto it = leaderboard.begin() + offset;
       it != leaderboard.begin() + finish; ++it) {
    text += fmt::format("{}. {}: {}\n", (it - leaderboard.begin()) + 1,
                        it->first, it->second);
  }
  if (finish != leaderboard.size()) {
    text += "...";
  }
  DrawText(text.c_str(), w / 4 + 10, h / 4 + 10, 20, BLACK);
}

struct Button {
  int x1;
  int y1;
  int x2;
  int y2;
};

bool in_button(Vector2 pos, Button button) {
  return pos.x > button.x1 && pos.x < button.x2 && pos.y > button.y1 &&
         pos.y < button.y2;
}

bool operator==(const Color& a, const Color& b) {
  return a.r == b.r && a.g == b.g && a.b == a.b && a.a == b.a;
}
class ButtonMaker {
  bool _play_sound;
  Sound _sound;
  float _volume;
  std::vector<Button> buttons;
  static bool enter;

public:
  ButtonMaker(bool play_sound, Sound sound, float volume)
      : _play_sound{play_sound}, _sound{sound}, _volume{volume} {}
  Button draw_button(Vector2 place, std::string text, bool enabled) {
    bool button_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    auto pos = GetMousePosition();
    if (in_button(pos, Button{int(place.x), int(place.y), int(place.x + 200),
                              int(place.y + 30)})) {
      Color c = enabled ? YELLOW : (button_down ? DARKGRAY : LIGHTGRAY);
      if (text == "SOUND") {
        int level{_volume * 200};
        DrawRectangle(place.x, place.y, level, 30, YELLOW);
        DrawRectangle(place.x + level, place.y, 200 - level, 30, LIGHTGRAY);
        if (button_down) {
          int x = GetMouseX();
          _volume = (x - place.x) / 200.0;
        }
      } else {
        DrawRectangle(place.x, place.y, 200, 30, c);
      }
    } else {
      Color c = enabled ? GOLD : GRAY;
      if (text == "SOUND") {
        int level{_volume * 200};
        DrawRectangle(place.x, place.y, level, 30, GOLD);
        DrawRectangle(place.x + level, place.y, 200 - level, 30, GRAY);
      } else {
        DrawRectangle(place.x, place.y, 200, 30, c);
      }
    }
    const char *label = text == "SOUND" ? fmt::format("SOUND ({}%)", int(_volume * 100)).c_str() : text.c_str();
    auto width = MeasureText(label, 20);
    DrawText(label, place.x + 100 - width / 2, place.y + 5, 20, BLACK);
    auto button = Button{int(place.x), int(place.y), int(place.x + 200),
                         int(place.y + 30)};
    buttons.push_back(button);
    return button;
  }
  void play_sound() {
    if (!_play_sound) {
      return;
    }
    bool in = false;
    for (auto it = buttons.begin(); it != buttons.end(); ++it) {
      auto pos = GetMousePosition();
      if (in_button(pos, *it)) {
        in = true;
        if (enter) {
          enter = false;
          if (IsSoundReady(_sound)) {
            PlaySound(_sound);
          }
        }
      }
    }
    if (!in) {
      enter = true;
    }
  }
  float volume() const {
    return _volume;
  }
};

bool ButtonMaker::enter = true;

class Game {
  std::string _name;
  Board _board;
  Board _old_board;
  bool work_board = false;
  bool first_work = true;
  std::vector<std::tuple<int, int, int>> removed_cells;

public:
  Game(size_t size) : _board{size, size}, _old_board{_board} {}
  int counter = 0;
  void new_game() {
    counter = 0;
    work_board = false;
    _board.fill();
    _board.stabilize();
    _board.zero();
  }
  void save() {
    std::ofstream save("save.txt");
    save << _name << " " << counter << "\n";
    save << _board;
  }
  bool load() {
    std::ifstream load("save.txt");
    if (load) {
      work_board = false;
      load >> _name >> counter >> _board;
      _board.match_patterns();
      _board.match_threes();
      return true;
    }
    return false;
  }
  void save_state() { _old_board = _board; }
  bool check_state() const { return _old_board == _board; }
  void restore_state() { _board = _old_board; }
  void match() {
    _board.match_patterns();
    _board.match_threes();
  }
  Board &board() { return _board; }
  std::string &name() { return _name; }
  void attempt_move(int row1, int col1, int row2, int col2) {
    first_work = true;
    work_board = true;
    save_state();
    _board.swap(row1, col1, row2, col2);
    _board.prepare_removals();
  }
  std::vector<std::tuple<int, int, int>> step() {
    std::vector<std::tuple<int, int, int>> res;
    if (!_board.has_removals()) {
      _board.prepare_removals();
    }
    if (!_board.has_removals()) {
      if (first_work) {
        restore_state();
      } else {
        counter += 1;
      }
      work_board = false;
      _board.match_patterns();
      _board.match_threes();
    }
    res = _board.remove_one_thing();
    _board.fill_up();
    first_work = false;
    return res;
  }
  bool is_finished() { return counter == 50; }
  bool is_processing() { return work_board; }
  std::string game_stats() {
    return fmt::format("Moves: {}\nScore: {}\nTrios: {}\nQuartets: "
                       "{}\nQuintets: {}\nCrosses: {}",
                       counter, _board.score, _board.normals, _board.longers,
                       _board.longests, _board.crosses);
  }
};

struct Particle {
  float dx = 0;
  float dy = 0;
  float da = 0;
  float x = 0;
  float y = 0;
  float a = 0;
  Color color;
  int lifetime = 0;
  int sides = 0;
};

struct Explosion {
  int x = 0;
  int y = 0;
  int lifetime = 0;
};

void button_flag(Vector2 pos, Button button, bool &flag) {
  if (in_button(pos, button)) {
    flag = !flag;
  }
}

int main() {
  auto w = 1280;
  auto h = 800;
  auto board_size = 16;
  Game game(board_size);
  bool first_click = true;
  int saved_row = 0;
  int saved_col = 0;
  bool draw_leaderboard = false;
  bool input_name = false;
  int frame_counter = 0;
  bool hints = false;
  bool particles = false;
  bool play_sound = false;
  bool nonacid_colors = false;
  bool ignore_r = false;
  size_t l_offset = 0;
  float volume = 0.0f;
  Leaderboard leaderboard = ReadLeaderboard();
  std::vector<Particle> flying;
  std::vector<Explosion> staying;
  std::default_random_engine eng{static_cast<unsigned>(
      std::chrono::system_clock::now().time_since_epoch().count())};
  std::uniform_int_distribution<int> dd{-10, 10};
  InitAudioDevice();
  Sound psound = LoadSound("p.ogg");
  Sound ksound = LoadSound("k.ogg");
  if (!game.load()) {
    game.new_game();
    input_name = true;
  }
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(w, h, "Tiar2");
  auto icon = LoadImage("icon.png");
  SetWindowIcon(icon);
  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    SetMasterVolume(volume);
    if (frame_counter == 60) {
      frame_counter = 0;
    } else {
      frame_counter += 1;
    }
    w = GetRenderWidth();
    h = GetRenderHeight();
    auto s = 0;
    if (w > h) {
      s = h;
    } else {
      s = w;
    }
    auto margin = 10;
    auto board_x = w / 2 - s / 2 + margin;
    auto board_y = h / 2 - s / 2 + margin;
    auto ss = (s - 2 * margin) / board_size;
    auto so = 2;
    auto mo = 0.5;
    if (game.is_processing() && frame_counter % 6 == 0) {
      auto f = game.step();
      if (play_sound && !f.empty() && IsSoundReady(psound)) {
        PlaySound(psound);
      }
      if (particles) {
        if (play_sound && !f.empty()) {
          board_x += dd(eng);
          board_y += dd(eng);
        }
        flying.reserve(flying.size() + f.size());
        staying.reserve(staying.size() + f.size());
        for (auto it = f.begin(); it != f.end(); ++it) {
          Color c;
          int s;
          switch (std::get<2>(*it)) {
          case 1: {
            c = nonacid_colors ? PINK : RED;
            s = 4;
            break;
          }
          case 2: {
            c = nonacid_colors ? LIME : GREEN;
            s = 0;
            break;
          }
          case 3: {
            c = nonacid_colors ? SKYBLUE : BLUE;
            s = 6;
            break;
          }
          case 4: {
            c = nonacid_colors ? GOLD : ORANGE;
            s = 3;
            break;
          }
          case 5: {
            c = nonacid_colors ? PURPLE : MAGENTA;
            s = 5;
            break;
          }
          case 6: {
            c = nonacid_colors ? BEIGE : YELLOW;
            s = 4;
            break;
          }
          }
          flying.emplace_back(dd(eng), dd(eng), dd(eng),
                              std::get<1>(*it) * ss + board_x + ss / 2,
                              std::get<0>(*it) * ss + board_y + ss / 2, 0, c, 0,
                              s);
          staying.emplace_back(std::get<1>(*it), std::get<0>(*it), 0);
        }
      }
    }
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawRectangle(board_x, board_y, ss * board_size, ss * board_size, BLACK);
    for (int i = 0; i < board_size; ++i) {
      for (int j = 0; j < board_size; ++j) {
        auto pos_x = board_x + i * ss + so;
        auto pos_y = board_y + j * ss + so;
        auto radius = (ss - 2 * so) / 2;
        if (game.board().is_matched(j, i) && hints) {
          DrawRectangle(pos_x, pos_y, ss - 2 * so, ss - 2 * so, DARKGRAY);
        } else if (game.board().is_three(j, i) && hints) {
          DrawRectangle(pos_x, pos_y, ss - 2 * so, ss - 2 * so, LIGHTGRAY);
        } else {
          DrawRectangle(pos_x, pos_y, ss - 2 * so, ss - 2 * so, GRAY);
        }
        switch (game.board().at(j, i)) {
        case 1:
          DrawPoly(Vector2{float(pos_x + radius), float(pos_y + radius)}, 4,
                   radius - mo, 45, nonacid_colors ? PINK : RED);
          break;
        case 2:
          DrawCircle(pos_x + radius, pos_y + radius, radius - mo,
                     nonacid_colors ? LIME : GREEN);
          break;
        case 3:
          DrawPoly(Vector2{float(pos_x + radius), float(pos_y + radius)}, 6,
                   radius - mo, 30, nonacid_colors ? SKYBLUE : BLUE);
          break;
        case 4:
          DrawPoly(
              Vector2{float(pos_x + radius), float(pos_y + radius + ss / 12)},
              3, radius - mo, 180, nonacid_colors ? GOLD : ORANGE);
          break;
        case 5:
          DrawPoly(
              Vector2{float(pos_x + radius), float(pos_y + radius + ss / 16)},
              5, radius - mo, 180, nonacid_colors ? PURPLE : MAGENTA);
          break;
        case 6:
          DrawPoly(Vector2{float(pos_x + radius), float(pos_y + radius)}, 4,
                   radius - mo, 0, nonacid_colors ? BEIGE : YELLOW);
          break;
        default:
          break;
        }
        if (game.board().is_magic(j, i)) {
          DrawCircleGradient(pos_x + radius, pos_y + radius, ss / 6, WHITE,
                             BLACK);
        }
        if (game.board().is_magic2(j, i)) {
          DrawCircleGradient(pos_x + radius, pos_y + radius, ss / 6, WHITE,
                             DARKPURPLE);
        }
      }
    }
    if (input_name) {
      char c = GetCharPressed();
      if ((std::isalnum(c) || c == '_') && game.name().length() < 22 &&
          !ignore_r) {
        game.name() += c;
      }
      ignore_r = false;
      DrawRectangle(w / 4, h / 2 - h / 16, w / 2, h / 8, WHITE);
      DrawText("Enter your name:", w / 4, h / 2 - h / 16, 50, BLACK);
      DrawText(game.name().c_str(), w / 4, h / 2 - h / 16 + 50, 50, BLACK);
    }
    if (draw_leaderboard && !input_name) {
      auto wheel_move = GetMouseWheelMove();
      auto kd = IsKeyPressed(KEY_DOWN);
      auto ku = IsKeyPressed(KEY_UP);
      if (wheel_move == 0) {
        if (kd) {
          wheel_move = -1;
        }
        if (ku) {
          wheel_move = 1;
        }
      }
      if (wheel_move != 0) {
        if (l_offset != 0 || wheel_move <= 0) {
          l_offset = l_offset - size_t(std::signbit(wheel_move) ? -1 : 1);
        }
        l_offset = std::min(l_offset, leaderboard.size() - 1);
      }
      DrawLeaderboard(leaderboard, l_offset);
    }
    DrawText(game.game_stats().c_str(), 3, 0, 30, BLACK);
    DrawText((fmt::format("Player:\n") + game.name()).c_str(), 3, h - 55, 20,
             BLACK);
    ButtonMaker bm(play_sound, ksound, volume);
    auto start_y = 0;
    auto sound_button = bm.draw_button(
        {float(w - 210), float(h - (start_y += 40))}, "SOUND", true);
    auto particles_button = bm.draw_button(
        {float(w - 210), float(h - (start_y += 40))}, "PARTICLES", particles);
    auto hints_button = bm.draw_button(
        {float(w - 210), float(h - (start_y += 40))}, "HINTS", hints);
    auto acid_button =
        bm.draw_button({float(w - 210), float(h - (start_y += 40))}, "NO ACID",
                       nonacid_colors);
    auto lbutton = bm.draw_button({float(w - 210), float(h - (start_y += 40))},
                                  "LEADERBOARD", draw_leaderboard);
    auto rbutton = bm.draw_button({float(w - 210), float(h - (start_y += 40))},
                                  "RESTART", false);
    auto load_button = bm.draw_button(
        {float(w - 210), float(h - (start_y += 40))}, "LOAD", false);
    auto save_button = bm.draw_button(
        {float(w - 210), float(h - (start_y += 40))}, "SAVE", false);
    bm.play_sound();
    volume = bm.volume();
    if (volume < 0.05f) {
      volume = 0.0f;
    }
    play_sound = volume != 0.0f;
    if (particles) {
      std::vector<Explosion> new_staying;
      new_staying.reserve(staying.size());
      for (auto it = staying.begin(); it != staying.end(); ++it) {
        Explosion p = *it;
        DrawRectangle(board_x + p.x * ss + so, board_y + p.y * ss + so,
                      ss - 2 * so, ss - 2 * so, WHITE);
        if (p.lifetime > 6) {
          continue;
        }
        p.lifetime += 1;
        new_staying.push_back(p);
      }
      new_staying.shrink_to_fit();
      staying = new_staying;

      std::vector<Particle> new_flying;
      new_flying.reserve(flying.size());
      for (auto it = flying.begin(); it != flying.end(); ++it) {
        Particle p = *it;
        auto c = p.color;
        c.a = 255 - p.lifetime;
        if (p.sides == 0) {
          DrawCircle(p.x, p.y, ss / 2, c);
        } else {
          DrawPoly(Vector2{float(p.x), float(p.y)}, p.sides, ss / 2, p.a, c);
        }
        p.y += p.dy;
        if (p.y > h || p.x < 0 || p.x > w || p.lifetime > 254) {
          continue;
        }
        p.x += p.dx;
        p.a += p.da;
        p.dy += 1;
        p.lifetime += 1;
        new_flying.push_back(p);
      }
      new_flying.shrink_to_fit();
      flying = new_flying;
    }
    EndDrawing();
    if (!input_name && !game.is_processing()) {
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        auto pos = GetMousePosition();
        button_flag(pos, particles_button, particles);
        button_flag(pos, hints_button, hints);
        button_flag(pos, acid_button, nonacid_colors);
        button_flag(pos, lbutton, draw_leaderboard);
        if (in_button(pos, rbutton)) {
          game.new_game();
          input_name = true;
        }
        if (in_button(pos, load_button)) {
          game.load();
        }
        if (in_button(pos, save_button)) {
          game.save();
        }
        if (draw_leaderboard) {
          goto outside;
        }
        pos = Vector2Subtract(pos, Vector2{float(board_x), float(board_y)});
        if (pos.x < 0 || pos.y < 0 || pos.x > ss * board_size ||
            pos.y > ss * board_size) {
          goto outside;
        }
        auto row = trunc(pos.y / ss);
        auto col = trunc(pos.x / ss);
        if (first_click) {
          saved_row = row;
          saved_col = col;
          first_click = false;
        } else {
          first_click = true;
          if (((abs(row - saved_row) == 1) ^ (abs(col - saved_col) == 1))) {
            game.attempt_move(row, col, saved_row, saved_col);
          }
        }
      } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        auto pos = GetMousePosition();
        pos = Vector2Subtract(pos, Vector2{float(board_x), float(board_y)});
        if (pos.x < 0 || pos.y < 0 || pos.x > ss * board_size ||
            pos.y > ss * board_size) {
          goto outside;
        }
        auto row = trunc(pos.y / ss);
        auto col = trunc(pos.x / ss);
        if (row != saved_row || col != saved_col) {
          if (!first_click) {
            first_click = true;
            if (((abs(row - saved_row) == 1) ^ (abs(col - saved_col) == 1))) {
              game.attempt_move(row, col, saved_row, saved_col);
            }
          }
        }
      }
    }
  outside:
    if (IsKeyPressed(KEY_ENTER) && input_name) {
      input_name = false;
      if (game.name().empty()) {
        game.name() = "dupa";
      }
    } else if (IsKeyPressed(KEY_BACKSPACE) && input_name) {
      if (!game.name().empty()) {
        game.name().pop_back();
      }
    } else if (!input_name) {
      while (int key = GetKeyPressed()) {
        switch (KeyboardKey(key)) {
        case KEY_R: {
          game.new_game();
          ignore_r = true;
          input_name = true;
          break;
        }
        case KEY_L: {
          draw_leaderboard = !draw_leaderboard;
          break;
        }
        case KEY_P: {
          particles = !particles;
          break;
        }
        case KEY_M: {
          play_sound = !play_sound;
          break;
        }
        case KEY_H: {
          hints = !hints;
          break;
        }
        case KEY_A: {
          nonacid_colors = !nonacid_colors;
          break;
        }
        case KEY_S: {
          game.save();
          break;
        }
        case KEY_O: {
          game.load();
          break;
        }
        default:
          break;
        }
      }
    }
    if (game.is_finished()) {
      leaderboard.emplace_back(game.name(), game.board().score);
      WriteLeaderboard(leaderboard);
      game.new_game();
      draw_leaderboard = true;
    }
  }
  game.save();
  WriteLeaderboard(leaderboard);
  CloseWindow();
  CloseAudioDevice();
  return 0;
}
