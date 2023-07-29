#include <iostream>
#include <random>
#include <algorithm>
#include <vector>
#include <set>
#include <chrono>
#include <fstream>
#include <fmt/format.h>

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
    friend Point operator-(const Point&a, const Point& b);
    Point& operator-=(const Point&a) { *this = *this - a; return *this; }
};

Point operator-(const Point&a, const Point& b) {
    return Point{a.x_ - b.x_, a.y_ - b.y_};
}

using Pattern = std::vector<Point>;

struct SizedPattern {
  Pattern pat;
  int w;
  int h;
};

inline Pattern shift(Pattern& p) {
  int minX = std::numeric_limits<int>::max();
  int minY = std::numeric_limits<int>::max();
  for(Point& pt : p) {
    minX = std::min(pt.x(), minX);
    minY = std::min(pt.y(), minY);
  }
  Pattern res(p);
  for(Point& pt: res) {
    pt -= Point{minX, minY};
  }
  return res;
}

inline std::vector<Pattern> rotations(Pattern& p) {
  std::vector<Pattern> res(4);
  res[0] = p;
  for (int i = 1; i < 4; ++i) {
    Pattern rotated(p.size());
    for (auto j = 0u; j < p.size(); ++j) {
      rotated[j].setX(res[i-1][j].y());
      rotated[j].setY(-res[i-1][j].x());
    }
    res[i] = shift(rotated);
  }
  return res;
}

inline Pattern mirrored(Pattern& p) {
  Pattern m(p);
  for (auto i = 0u; i < p.size(); ++i) {
    m[i].setY(-p[i].y());
  }
  return shift(m);
}

inline SizedPattern sized(Pattern& p) {
  SizedPattern res;
  res.pat = p;
  int maxX = std::numeric_limits<int>::min();
  int maxY = std::numeric_limits<int>::min();
  for(Point& pt : p) {
    maxX = std::max(pt.x(), maxX);
    maxY = std::max(pt.y(), maxY);
  }
  res.w = maxX + 1;
  res.h = maxY + 1;
  return res;
}

inline std::vector<SizedPattern> generate(Pattern p, bool symmetric=false) {
  auto s = rotations(p);
  std::vector<Pattern> res1;
  if (!symmetric) {
    Pattern m = mirrored(p);
    auto r = rotations(m);
    res1.reserve(s.size() + r.size());
    std::copy(r.begin(), r.end(), std::back_inserter(res1));
  }else{
    res1.reserve(s.size());
  }
  std::copy(s.begin(), s.end(), std::back_inserter(res1));
  std::vector<SizedPattern> res2;
  res2.reserve(res1.size());
  std::transform(res1.begin(), res1.end(), std::back_inserter(res2), [](Pattern& pt){
    return sized(pt);
  });
  return res2;
}

const Pattern three_p_1 = {{0, 0}, {1, 1}, {0, 2}};
const Pattern three_p_2 = {{1, 0}, {0, 1}, {0, 2}};
const Pattern three_p_3 = {{0, 0}, {0, 1}, {0, 3}};
const Pattern four_p = {{0, 0}, {1, 1}, {0, 2}, {0, 3}};
const Pattern five_p = {{0, 0}, {0, 1}, {1, 2}, {0, 3}, {0, 4}};
const Pattern seven_p = {{0, 0}, {1, 1}, {1, 2}, {1, 3}, {2, 0}, {3, 0}, {4, 0}};

const std::vector<SizedPattern> threes1 = generate(three_p_1);
const std::vector<SizedPattern> threes2 = generate(three_p_2);
const std::vector<SizedPattern> threes3 = generate(three_p_3);
const std::vector<SizedPattern> fours = generate(four_p);
const std::vector<SizedPattern> fives = generate(five_p);
const std::vector<SizedPattern> sevens = generate(seven_p);

class Board {
  std::vector<int> board;
  std::default_random_engine e1{static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count())};
  std::uniform_int_distribution<int> uniform_dist{1, 6};
  std::uniform_int_distribution<int> uniform_dist_2;
  std::uniform_int_distribution<int> uniform_dist_3;
  std::uniform_int_distribution<int> coin{1, 42};
  int w;
  int h;
  std::set<std::pair<int, int>> matched_patterns;
  std::set<std::pair<int, int>> matched_threes;
  std::set<std::pair<int, int>> magic_tiles;
  std::vector<std::tuple<int,int,int>> rm_i;
  std::vector<std::tuple<int,int,int>> rm_j;
  std::vector<std::pair<int, int>> rm_b;
public:
  int width() { return w; }
  int height() { return h; }
  int score{};
  int normals{};
  int longers{};
  int longests{};
  int crosses{};
  Board(int _w, int _h) : w{_w}, h{_h} {
    board.resize(w * h);
    std::fill(std::begin(board), std::end(board), 0);
    uniform_dist_2 = std::uniform_int_distribution<int>(0, w);
    uniform_dist_3 = std::uniform_int_distribution<int>(0, h);
  }
  Board(const Board& b){
    w = b.w;
    h = b.h;
    board = b.board;
    score = b.score;
    matched_patterns = b.matched_patterns;
    magic_tiles = b.magic_tiles;
  }
  Board operator=(const Board& b){
    w = b.w;
    h = b.h;
    board = b.board;
    score = b.score;
    matched_patterns = b.matched_patterns;
    magic_tiles = b.magic_tiles;
    return *this;
  }
  friend bool operator==(const Board& a, const Board& b);
  friend std::ostream& operator<<(std::ostream& of, const Board& b);
  bool match_pattern(int x, int y, const SizedPattern& p) {
    int color = at(x + p.pat[0].x(), y + p.pat[0].y());
    for(auto i = 1u; i < p.pat.size(); ++i) {
      if(color != at(x + p.pat[i].x(), y + p.pat[i].y())) {
        return false;
      }
    }
    return true;
  }
  void match_patterns() {
    matched_patterns.clear();
    for(const SizedPattern& sp : sevens){
      for(int i = 0; i <= w - sp.w; ++i){
        for(int j = 0; j <= h - sp.h; ++j){
          if(match_pattern(i, j, sp)){
            for(const Point& p: sp.pat){
              matched_patterns.insert({i + p.x(), j + p.y()});
            }
          }
        }
      }
    }
    for(const SizedPattern& sp : fours){
      for(int i = 0; i <= w - sp.w; ++i){
        for(int j = 0; j <= h - sp.h; ++j){
          if(match_pattern(i, j, sp)){
            for(const Point& p: sp.pat){
              matched_patterns.insert({i + p.x(), j + p.y()});
            }
          }
        }
      }
    }
    for(const SizedPattern& sp : fives){
      for(int i = 0; i <= w - sp.w; ++i){
        for(int j = 0; j <= h - sp.h; ++j){
          if(match_pattern(i, j, sp)){
            for(const Point& p: sp.pat){
              matched_patterns.insert({i + p.x(), j + p.y()});
            }
          }
        }
      }
    }
  }
  bool is_matched(int x, int y) {
    return matched_patterns.contains({x, y});
  }
  bool is_magic(int x, int y) {
    return magic_tiles.count({x, y}) > 0;
  }
  void swap(int x1, int y1, int x2, int y2) {
    auto tmp = at(x1,y1);
    at(x1, y1) = at(x2, y2);
    at(x2, y2) = tmp;

    if (is_magic(x1, y1)){
      magic_tiles.erase({x1, y1});
      magic_tiles.insert({x2, y2});
    }

    if (is_magic(x2, y2)){
      magic_tiles.erase({x2, y2});
      magic_tiles.insert({x1, y1});
    }
  }
  void fill(){
    score = 0;
    for(auto& x : board){
      x = uniform_dist(e1);
    }
  }
  int& at(int a, int b){
    return board[a * h + b];
  }
  int at(int a, int b) const{
    return board[a * h + b];
  }
  bool reasonable_coord(int i, int j){
    return i >= 0 && i < w && j >= 0 && j < h;
  }
  void remove_trios() {
    std::vector<std::tuple<int,int,int>> remove_i;
    std::vector<std::tuple<int,int,int>> remove_j;
    for(int i = 0; i < w; ++i){
      for(int j = 0; j < h; ++j){
        int offset_j = 1;
        int offset_i = 1;
        while(j + offset_j < h && at(i,j) == at(i, j+offset_j)){
          offset_j += 1;
        }
        if(offset_j > 2){
          remove_i.push_back({i, j, offset_j});
        }
        while(i + offset_i < w && at(i,j) == at(i+offset_i, j)){
          offset_i += 1;
        }
        if(offset_i > 2){
          remove_j.push_back({i, j, offset_i});
        }
      }
    }
    for(auto t : remove_i){
      int i = std::get<0>(t);
      int j = std::get<1>(t);
      int offset = std::get<2>(t);
      if(offset == 4){
        j = 0;
        offset = h;
        longers += 1;
        normals = std::max(0, normals-1);
      }
      for(int jj = j; jj < j + offset; ++jj){
        at(i, jj) = 0;
        if (is_magic(i, jj)){
            score -= 3;
            magic_tiles.erase({i, jj});
        }
        score += 1;
      }
      if(offset == 5){
        for(int i = 0; i < w; ++i){
          at(uniform_dist_2(e1), uniform_dist_3(e1)) = 0;
          score += 1;
        }
        longests += 1;
        normals = std::max(0, normals-1);
      }
      normals += 1;
    }
    for(auto t : remove_j){
      int i = std::get<0>(t);
      int j = std::get<1>(t);
      int offset = std::get<2>(t);
      if(offset == 4){
        i = 0;
        offset = w;
        longers += 1;
        normals = std::max(0, normals-1);
      }
      for(int ii = i; ii < i + offset; ++ii){
        at(ii, j) = 0;
        if (is_magic(ii, j)){
            score -= 3;
            magic_tiles.erase({ii, j});
        }
        score += 1;
      }
      if(offset == 5){
        for(int i = 0; i < w; ++i){
          at(uniform_dist_2(e1), uniform_dist_3(e1)) = 0;
          score += 1;
        }
        longests += 1;
        normals = std::max(0, normals-1);
      }
      normals += 1;
    }
    for(int i = 0; i < int(remove_i.size()); ++i){
      for(int j = 0; j < int(remove_j.size()); ++j){
        auto t1 = remove_i[i];
        auto t2 = remove_j[j];
        auto i1 = std::get<0>(t1);
        auto j1 = std::get<1>(t1);
        auto o1 = std::get<2>(t1);
        auto i2 = std::get<0>(t2);
        auto j2 = std::get<1>(t2);
        auto o2 = std::get<2>(t2);
        if(i1 >= i2 && i1 < (i2 + o2) && j2 >= j1 && j2 < (j1 + o1)){
          for(int m = -1; m < 2; ++m){
            for(int n = -1; n < 2; ++n){
              if(reasonable_coord(i1+m, j1+n)){
                at(i1+m, j1+n) = 0;
                score += 1;
              }
            }
          }
          crosses += 1;
          normals = std::max(0, normals-2);
        }
      }
    }
  }
  void fill_up(){
    int curr_i = -1;
    for(int i = 0; i < w; ++i){
      for(int j = 0; j < h; ++j){
        if(at(i,j) == 0){
          curr_i = i;
          while(curr_i < w - 1 && at(curr_i + 1, j) == 0){
            curr_i += 1;
          }
          for(int k = curr_i; k >= 0; --k){
            if(at(k, j) != 0){
              at(curr_i,j) = at(k, j);
              if (is_magic(k, j)) {
                  magic_tiles.erase({k, j});
                  magic_tiles.insert({curr_i, j});
              }
              curr_i -= 1;
            }
          }
          for(int k = curr_i; k >= 0; --k){
            at(k, j) = uniform_dist(e1);
            if (coin(e1) == 1) {
              magic_tiles.insert({k, j});
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
    }while(!(*this == old_board));
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
  void remove_one_thing() {
      if(!rm_i.empty()){
        auto t = rm_i.back();
        int i = std::get<0>(t);
        int j = std::get<1>(t);
        int offset = std::get<2>(t);
        if(offset == 4){
          j = 0;
          offset = h;
          longers += 1;
          normals = std::max(0, normals-1);
        }
        for(int jj = j; jj < j + offset; ++jj){
          at(i, jj) = 0;
          if (is_magic(i, jj)){
              score -= 3;
              magic_tiles.erase({i, jj});
          }
          score += 1;
        }
        if(offset == 5){
          for(int i = 0; i < w; ++i){
            at(uniform_dist_2(e1), uniform_dist_3(e1)) = 0;
            score += 1;
          }
          longests += 1;
          normals = std::max(0, normals-1);
        }
        normals += 1;
        rm_i.pop_back();
        return;
      }
      if(!rm_j.empty()){
        auto t = rm_j.back();
        int i = std::get<0>(t);
        int j = std::get<1>(t);
        int offset = std::get<2>(t);
        if(offset == 4){
          i = 0;
          offset = w;
          longers += 1;
          normals = std::max(0, normals-1);
        }
        for(int ii = i; ii < i + offset; ++ii){
          at(ii, j) = 0;
          if (is_magic(ii, j)){
              score -= 3;
              magic_tiles.erase({ii, j});
          }
          score += 1;
        }
        if(offset == 5){
          for(int i = 0; i < w; ++i){
            at(uniform_dist_2(e1), uniform_dist_3(e1)) = 0;
            score += 1;
          }
          longests += 1;
          normals = std::max(0, normals-1);
        }
        normals += 1;
        rm_j.pop_back();
        return;
      }
      if(!rm_b.empty()){
          auto t = rm_b.back();
          int i = std::get<0>(t);
          int j = std::get<1>(t);
          for(int m = -1; m < 2; ++m){
            for(int n = -1; n < 2; ++n){
              if(reasonable_coord(i+m, j+n)){
                at(i+m, j+n) = 0;
                score += 1;
              }
            }
          }
          crosses += 1;
          normals = std::max(0, normals-2);
          rm_b.pop_back();
          return;
      }
  }
  void prepare_removals() {
      rm_i.clear();
      rm_j.clear();
      rm_b.clear();
      matched_patterns.clear();
      matched_threes.clear();
      for(int i = 0; i < w; ++i){
          for(int j = 0; j < h; ++j){
              int offset_j = 1;
              int offset_i = 1;
              while(j + offset_j < h && at(i,j) == at(i, j+offset_j)){
                  offset_j += 1;
              }
              if(offset_j > 2){
                  rm_i.emplace_back(i, j, offset_j);
              }
              while(i + offset_i < w && at(i,j) == at(i+offset_i, j)){
                  offset_i += 1;
              }
              if(offset_i > 2){
                  rm_j.emplace_back(i, j, offset_i);
              }
          }
      }
      for(int i = 0; i < int(rm_i.size()); ++i){
          for(int j = 0; j < int(rm_j.size()); ++j){
              auto t1 = rm_i[i];
              auto t2 = rm_j[j];
              auto i1 = std::get<0>(t1);
              auto j1 = std::get<1>(t1);
              auto o1 = std::get<2>(t1);
              auto i2 = std::get<0>(t2);
              auto j2 = std::get<1>(t2);
              auto o2 = std::get<2>(t2);
              if(i1 >= i2 && i1 < (i2 + o2) && j2 >= j1 && j2 < (j1 + o1)){
                  rm_b.emplace_back(i1, j1);
              }
          }
      }
      std::sort(std::begin(rm_i), std::end(rm_i), [](auto& t1, auto& t2) {
          auto i1 = std::get<0>(t1);
          auto i2 = std::get<0>(t2);
          return i1 > i2;
      });
      std::sort(std::begin(rm_j), std::end(rm_j), [](auto& t1, auto& t2) {
          auto i1 = std::get<0>(t1);
          auto i2 = std::get<0>(t2);
          return i1 > i2;
      });
      std::sort(std::begin(rm_b), std::end(rm_b), [](auto& t1, auto& t2) {
          auto i1 = std::get<0>(t1);
          auto i2 = std::get<0>(t2);
          return i1 > i2;
      });
  }
  bool has_removals() {
      return rm_i.size() + rm_j.size() + rm_b.size();
  }
  void match_threes() {
      matched_threes.clear();
      for(const SizedPattern& sp : threes1){
        for(int i = 0; i <= w - sp.w; ++i){
          for(int j = 0; j <= h - sp.h; ++j){
            if(match_pattern(i, j, sp)){
              for(const Point& p: sp.pat){
                matched_threes.insert({i + p.x(), j + p.y()});
              }
            }
          }
        }
      }
      for(const SizedPattern& sp : threes2){
        for(int i = 0; i <= w - sp.w; ++i){
          for(int j = 0; j <= h - sp.h; ++j){
            if(match_pattern(i, j, sp)){
              for(const Point& p: sp.pat){
                matched_threes.insert({i + p.x(), j + p.y()});
              }
            }
          }
        }
      }
      for(const SizedPattern& sp : threes3){
        for(int i = 0; i <= w - sp.w; ++i){
          for(int j = 0; j <= h - sp.h; ++j){
            if(match_pattern(i, j, sp)){
              for(const Point& p: sp.pat){
                matched_threes.insert({i + p.x(), j + p.y()});
              }
            }
          }
        }
      }
  }
  bool is_three(int i, int j) {
      return matched_threes.contains({i, j});
  }
};

bool operator==(const Board& a, const Board& b){
  if (a.w != b.w || a.h != b.h){
      return false;
  }
  for(int i = 0; i < a.w * a.h; ++i){
    if (a.board[i] != b.board[i]){
      return false;
    }
  }
  return true;
}

std::ostream& operator<<(std::ostream& of, const Board& b) {
    for(int i = 0; i < b.w; ++i){
        for(int j = 0; j < b.h; ++j){
            of << b.at(i, j) << " ";
        }
        of << std::endl;
    }
    return of;
}

using Leaderboard = std::vector<std::pair<std::string, int>>;

Leaderboard ReadLeaderboard() {
    Leaderboard res;
    std::ifstream input("leaderboard.txt");
    std::string line;
    while(std::getline(input, line)){
        auto idx = line.find(';');
        if (idx != std::string::npos) {
            std::string name = line.substr(0, idx);
            int score = std::stoi(line.substr(idx+1));
            res.emplace_back(name, score);
        }
    }
    std::sort(std::begin(res), std::end(res), [](auto& a, auto& b) {return a.second > b.second; });
    return res;
}

void WriteLeaderboard(Leaderboard leaderboard) {
    std::string text;
    std::sort(std::begin(leaderboard), std::end(leaderboard), [](auto& a, auto& b) {return a.second > b.second; });
    for(auto it: leaderboard){
        text += fmt::format("{};{}\n", it.first, it.second);
    }
    std::ofstream output("leaderboard.txt");
    output << text;
}


void DrawLeaderboard(Leaderboard leaderboard, size_t offset = 0) {
    auto w = GetRenderWidth();
    auto h = GetRenderHeight();
    DrawRectangle(w/4, h/4, w/2, h/2, WHITE);
    std::string text = "Leaderboard:\n";
    std::sort(std::begin(leaderboard), std::end(leaderboard), [](auto& a, auto& b) {return a.second > b.second; });
    offset = std::min(offset, leaderboard.size() - 1);
    auto finish = std::min(offset + 9, leaderboard.size());
    for(auto it = leaderboard.begin() + offset; it != leaderboard.begin() + finish; ++it){
        text += fmt::format("{}. {}: {}\n", (it - leaderboard.begin()) + 1, it->first, it->second);
    }
    if (finish != leaderboard.size()) {
        text += "...";
    }
    DrawText(text.c_str(), w/4 + 10, h/4 + 10, 20, BLACK);
}

int main() {
    auto w = 1280;
    auto h = 800;
    auto board_size = 16;
    auto board = Board(board_size, board_size);
    auto old_board = board;
    bool first_click = true;
    int saved_row = 0;
    int saved_col = 0;
    int counter = 0;
    bool draw_leaderboard = false;
    bool input_name = true;
    bool work_board = false;
    bool first_work = true;
    int frame_counter = 0;
    bool hints = false;
    bool nonacid_colors = false;
    size_t l_offset = 0;
    std::string name;
    Leaderboard leaderboard = ReadLeaderboard();
    board.fill();
    board.stabilize();
    board.zero();
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(w, h, "Tiar2");
    auto icon = LoadImage("icon.png");
    SetWindowIcon(icon);
    SetTargetFPS(60);
    while(!WindowShouldClose()){
        if(frame_counter == 60){
            frame_counter = 0;
        }else{
            frame_counter += 1;
        }
        if (work_board && frame_counter % 15 == 0) {
            auto new_board = board;
            if (!board.has_removals()) {
                board.prepare_removals();
            }
            board.remove_one_thing();
            board.fill_up();
            if (new_board == board) {
                if (first_work) {
                    board = old_board;
                }else{
                    counter += 1;
                }
                work_board = false;
                board.match_patterns();
                board.match_threes();
            }
            first_work = false;
        }
        BeginDrawing();
        ClearBackground(RAYWHITE);
        w = GetRenderWidth();
        h = GetRenderHeight();
        auto s = 0;
        if (w>h){
            s = h;
        }else{
            s = w;
        }
        auto margin = 10;
        auto board_x = w/2- s/2 + margin;
        auto board_y = h/2 - s/2 + margin;
        auto ss = (s - 2*margin) / board_size;
        auto so = 2;
        auto mo = 0.5;
        DrawRectangle(board_x, board_y, ss * board_size, ss * board_size, BLACK);
        for(int i = 0; i < board_size; ++i){
            for(int j = 0; j < board_size; ++j){
                auto pos_x = board_x + i*ss + so;
                auto pos_y = board_y + j*ss + so;
                auto radius = (ss-2*so)/2;
                if (board.is_matched(j, i) && hints) {
                    DrawRectangle(pos_x, pos_y, ss - 2*so, ss - 2*so, DARKGRAY);
                }else if (board.is_three(j, i) && hints) {
                    DrawRectangle(pos_x, pos_y, ss - 2*so, ss - 2*so, LIGHTGRAY);
                }else{
                    DrawRectangle(pos_x, pos_y, ss - 2*so, ss - 2*so, GRAY);
                }
                switch(board.at(j, i)){
                case 1:
                    DrawPoly(Vector2{float(pos_x + radius), float(pos_y + radius)}, 4, radius - mo, 45, nonacid_colors ? Color{255, 127, 127, 255} : RED);
                    break;
                case 2:
                    DrawCircle(pos_x + radius, pos_y + radius, radius - mo, nonacid_colors ? Color{127, 255, 127, 255} : GREEN);
                    break;
                case 3:
                    DrawPoly(Vector2{float(pos_x + radius), float(pos_y + radius)}, 6, radius - mo, 30, nonacid_colors ? Color{127, 127, 255, 255} : BLUE);
                    break;
                case 4:
                    DrawPoly(Vector2{float(pos_x + radius), float(pos_y + radius + ss/12)}, 3, radius - mo, 180, nonacid_colors ? Color{255, 128, 0, 255} : ORANGE);
                    break;
                case 5:
                    DrawPoly(Vector2{float(pos_x + radius), float(pos_y + radius + ss/16)}, 5, radius - mo, 180, nonacid_colors ? Color{192, 0, 192, 255} : MAGENTA);
                    break;
                case 6:
                    DrawPoly(Vector2{float(pos_x + radius), float(pos_y + radius)}, 4, radius - mo, 0, nonacid_colors ? Color{192, 192, 9, 255} : YELLOW);
                    break;
                default:
                    break;
                }
                if (board.is_magic(j, i)){
                    DrawEllipse(pos_x + radius, pos_y + radius, mo, mo, BLACK);
                }
            }
        }
        if (!draw_leaderboard && !input_name && !work_board) {
            if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                auto pos = GetMousePosition();
                pos = Vector2Subtract(pos, Vector2{float(board_x), float(board_y)});
                auto row = trunc(pos.y / ss);
                auto col = trunc(pos.x / ss);
                if (first_click) {
                    saved_row = row;
                    saved_col = col;
                    first_click = false;
                } else {
                    first_click = true;
                    if(((abs(row - saved_row) == 1) ^ (abs(col - saved_col) == 1))){
                        first_work = true;
                        work_board = true;
                        old_board = board;
                        board.swap(row, col, saved_row, saved_col);
                        board.prepare_removals();
//                        auto new_board = board;
//                        board.stabilize();
//                        if (new_board == board) {
//                            board = old_board;
//                        }else{
//                            counter += 1;
//                        }
                    }
                }
            } else if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                auto pos = GetMousePosition();
                pos = Vector2Subtract(pos, Vector2{float(board_x), float(board_y)});
                auto row = trunc(pos.y / ss);
                auto col = trunc(pos.x / ss);
                if(row != saved_row || col != saved_col){
                    if(!first_click){
                        first_click = true;
                        if(((abs(row - saved_row) == 1) ^ (abs(col - saved_col) == 1))){
                            first_work = true;
                            work_board = true;
                            old_board = board;
                            board.swap(row, col, saved_row, saved_col);
                            board.prepare_removals();
//                            auto new_board = board;
//                            board.stabilize();
//                            if (new_board == board) {
//                                board = old_board;
//                            }else{
//                                counter += 1;
//                            }
                        }
                    }
                }
            }
        }
        if(input_name){
            char c = GetCharPressed();
            if (c && c != ';' && name.length() < 22) {
                name += c;
            }
            DrawRectangle(w/4, h/2-h/16, w/2, h/8, WHITE);
            DrawText("Enter your name:", w/4, h/2-h/16, 50, BLACK);
            DrawText(name.c_str(), w/4, h/2-h/16 + 50, 50, BLACK);
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
                if (l_offset != 0 || wheel_move != 1) {
                    l_offset = l_offset - size_t(wheel_move);
                }
                l_offset = std::min(l_offset, leaderboard.size() - 1);
            }
            DrawLeaderboard(leaderboard, l_offset);
        }
        DrawText(fmt::format("Moves: {}\nScore: {}\nTrios: {}\nQuartets: {}\nQuintets: {}\nCrosses: {}", counter, board.score, board.normals, board.longers, board.longests, board.crosses).c_str(), 3, 0, 30, BLACK);
        EndDrawing();
        if(IsKeyPressed(KEY_ENTER) && input_name){
            input_name = false;
            if (name.empty()) {
                name = "dupa";
            }
        }else if(IsKeyPressed(KEY_BACKSPACE) && input_name){
            if (!name.empty()) {
                name.pop_back();
            }
        }else if(!input_name)  {
            if(IsKeyPressed(KEY_R)){
                counter = 0;
                board.score = 0;
                board.zero();
                input_name = true;
            }else if(IsKeyPressed(KEY_L)){
                draw_leaderboard = !draw_leaderboard;
            }else if(IsKeyPressed(KEY_H)){
                hints = !hints;
            }else if(IsKeyPressed(KEY_A)){
                nonacid_colors = !nonacid_colors;
            }
        }
        if (counter == 50){
            leaderboard.emplace_back(name, board.score);
            WriteLeaderboard(leaderboard);
            counter = 0;
            board.score = 0;
            board.zero();
            draw_leaderboard = true;
        }
    }
    WriteLeaderboard(leaderboard);
    return 0;
}
