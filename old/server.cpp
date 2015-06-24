#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include "base64.h"

#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <random>
#include <thread>

using namespace std;
using boost::asio::ip::tcp;

class ProblemContainer {
public:
  void addMaze(pair<bool, vector<int>> maze) {
    mazeProblems.push_back(maze);
    ++mazeSize;
    ++globalSize;
  }
  void addSudoku(pair<bool, vector<int>> sudoku) {
    sudokuProblems.push_back(sudoku);
    ++sudokuSize;
    ++globalSize;
  }
  void addArray(pair<bool, pair<int, vector<int>>> array) {
    arrayProblems.push_back(array);
    ++arraySize;
    ++globalSize;
  }
  void addTree(pair<bool, vector<int>> array) {
    treeProblems.push_back(array);
    ++treeSize;
    ++globalSize;
  }
  void addPassword(pair<bool, string> array) {
    passwordProblems.push_back(array);
    ++passwordSize;
    ++globalSize;
  }
  void addRLE(pair<bool, pair<int, string>> array) {
    RLEProblems.push_back(array);
    ++rleSize;
    ++globalSize;
  }

  pair<bool, vector<int>> getMaze(int index) { return mazeProblems[index]; }
  pair<bool, vector<int>> getSudoku(int index) { return sudokuProblems[index]; }
  pair<bool, pair<int, vector<int>>> getArray(int index) {
    return arrayProblems[index];
  }
  pair<bool, vector<int>> getTree(int index) { return treeProblems[index]; }
  pair<bool, string> getPassword(int index) { return passwordProblems[index]; }
  pair<bool, pair<int, string>> getRLE(int index) { return RLEProblems[index]; }

  int getSize(int index) {
    return index == 0 ? mazeSize : index == 1
                                       ? sudokuSize
                                       : index == 2
                                             ? arraySize
                                             : index == 3
                                                   ? treeSize
                                                   : index == 4 ? passwordSize
                                                                : rleSize;
  }
  int getGlobalSize() { return globalSize; }

private:
  vector<pair<bool, vector<int>>> mazeProblems;             // +4
  vector<pair<bool, vector<int>>> sudokuProblems;           // +4
  vector<pair<bool, pair<int, vector<int>>>> arrayProblems; // +2
  vector<pair<bool, vector<int>>> treeProblems;             // +4
  vector<pair<bool, string>> passwordProblems;              // +2
  vector<pair<bool, pair<int, string>>> RLEProblems;        // +2
  int mazeSize;
  int sudokuSize;
  int arraySize;
  int treeSize;
  int passwordSize;
  int rleSize;

  int globalSize;
};

const string welcome =
    "KysrKyMjIyMjJzs6Ojo6OiwsLCwsLCwsLCwsLCwsLCwsLCwsOjo6Ojo6Ojo7Ozs7OzsnJycnKy"
    "sjIyMjIyMjIyMjIw0KKysrKyMjQCMjJzs6Ojo6OjosLCwsLCwsLCwsLCw6LCwsLCw6Ojo6Ojo6"
    "Ojo7Ozs7OycnJycrKyMjQEBAQEBAQEBAQA0KKysrKyNAQCMjJzs6Ojo6OjosLCwsLCwsLCwsLC"
    "wsLCwsOjo6Ojo6Ojo6Ozs7Ozs7JycnJysrKyMjQEBAQEBAQEBAQA0KKysrIyNAQCNAOzs6Ojo6"
    "OjosLCwsLCwsLCwsLCwsLCwsOjo6Ojo6Ojo6Ojs7Ozs7OycnJycrKyMjI0BAQEBAQEBAQA0KKy"
    "srIyMjQEBAOzs6Ojo6OiwsLCwsLCwsLCwsLCwsLCwsLCwsOjo6Ojo6Ojo7Ozs7OycnJycnJysj"
    "I0BAQEBAQEBAQA0KKysrIyMjI0BAOzs6Ojo6LCwsLCwsLCwsLCwsLCwsLCwsOiw6Ojo6Ojo6Oj"
    "o6Ozs7OycnJycnJysrIyNAQEBAQEBAQA0KKysrIyMjI0AjOzo6Ojo6LDosLCwsLCwsLCwsLCws"
    "LCwsLDo6Ojo6Ojo6Ojo6Ojs7OzsnJycnJycnKyMjI0BAQEBAQA0KKysrKysjQEBAOzo6Ojo6Oj"
    "o6LCwsLCwsLCwsLCwsLCwsOjo6Ojo6Ojo6Ojo6Ozs7Ozs7JycnKycnKysjI0BAQCNAQA0KKysr"
    "KyMjQCMjOzo6LCwsOjo6OiwsLCwsLCwsOiwsOjo6Ojo6Ojo6Ojo7OzsnOzs7Ozs7JycnJycnJy"
    "crI0AjQCNAIw0KKysrKyMjI0ArOjo6Ojo6Ojo6Ojo6LCw6Ojo6Ojo6Ojo6Ozs7Ozs7OzsnKysn"
    "JycnJycnJycnJycnJycnKyMjQEBAQA0KKysrKysjI0A7Ozs6Ojo6Ojo6Ojo6Ojo6Ojo6Ojo6Oj"
    "s7OzsnJysrKysrKysrKysrKycnJycnJycnJycnJysrIyNAQA0KKysrKysjIyM7Jyc7Ozs7Ozo6"
    "Ojs7Ozo6Ojo6Ojo6Ozs7JycrKyMjIyMjIysrKyMrKysrJycrKycrJycnJysrKyMjQA0KKysrKy"
    "sjKyM7KysrKycrKysnJzs7Ozs7Ozo6Ojs7OzsnJysrKyMjIyMrKysrJysrKysrKysrKysrJycn"
    "JycrKysjIw0KKysrKysjQEAnKysjIyMjIyMjKysnJycnOjo6Ojs7JycnKysrKyMjIyMjIyMjKy"
    "srKycrKysrKysrJycnJycnJysjQA0KKysrKysjQEArKysrKysjIyMjIyMjKysnOzs7JycnJysr"
    "KysjIyMjIysjK0BAQCMjIysrKycnKycrJycnJycnJycjIw0KKysrKysjI0ArKycrKyMjIyMjI0"
    "AjIyMnJzs6Ojs7OycrKyMjIysrOjojQEBAIysnIyMrKycnJycnJycnJycnJysrIw0KKysrKysj"
    "I0AnKysjIyMnO0AsQEBAIyMjOzo6Ojo7OycrIyMjKys7LCwjQEAjIycnKyMrJycnOycnJycnJy"
    "cnJycrIw0KKysrKysrI0A7JysjIyc6OiMjQEAjJysjOzosOjo7OycrKysrIzs6LCwnIyMjKzsn"
    "KysrJyc7Ozs7OycnOycnJycrIw0KKysrKysrIyM7OysrJzssOiMjIyMjJyc7OzosOjs7OycnJy"
    "c7Ozs7Ojo6Ojs6OzsnKycnJzs7Ojs7OzsnOycnJycrKw0KKysrKysrIysnOycnJzs6OjojKysn"
    "Ozs7Ojo6Ojs7OycnJyc7Ozs7Ozs7JzsnJycnJzs7Ozs7Ozs7OzsnOycnJycnKw0KKysrKysrKy"
    "c7Ozs7Jzs7Ojo6OjsnJzs7Ojo6Ojs7Ozs7OycnOzsnJyc7OycnJzsnOzo6Ojo7Ozs7Ozs7Jycn"
    "JycnKw0KKysrKysrKyc7Ozs7Ozs7JycnOycnOzs7Ojo6Ojs7Ozs7OzsnOzs6Ozs7Jyc7Ozs7Oj"
    "o6Ojo7Ozs7Ozs7JycnJycnKw0KKysrKysrKzs7Ojo6Ojs7Ozs7Ozs6Ojs6Ojo6Ojs7Ozs7Ozs7"
    "Ozs7Ojo6Ozo6Ojo6OiwsOjo6Ozs7Ozs7JycnJycnKw0KKysrKysrKzs6Ojo6Ojo7Ozs7Ojo6Oz"
    "s6Ojo6Ozs7Ozs7Ozs7Ozo6Ojo6Ojo6OjosLCwsOjo6Ozs7Ozs7JycnJycnKw0KKysrKysrKzo6"
    "Ojo6Ojo6Ojo6Ojo6Ozo6Ojo6Ozs7Ozs7Ozo7Ojo6Ojo6Ojo6Ojo6LCwsOjo6Ozs7OzsnJycrKy"
    "snKw0KKysrKysrJzo6Ojo6Ojo6Ojo6Ojo6Ojo6Ojo6Ozs7Ozs7Ozo6Ojo6Ojo6OjosOjo6Oiw6"
    "Ojo6Ojs7OzsnJycrKysnKw0KKysrKysrOzo6Ojo6Ojo6Ojo6Ojo6Ojo6Ojo6Ozs7Jzs7Ozs7Oj"
    "o6Ojo6Ojo6Ojo6Ojo6Ojo6Ojs7OzsnJycrKysrKw0KKysrKysrOzo6Ojo6Ojo6Ojo6Ojo6Ojo6"
    "Ojo6Ojs7OycnOzs7Ozs6LCwsLDo6Ojo6Ojo6Ojs6Ozs7OzsnJycrKysrKw0KKysrKysrOzo6Oj"
    "o6LDo6Ojo6Ojs6OjosLDo6Ojs7Jyc7Ozs7Ozs6LCwsLCwsOjo6Ojo6Ojs7Ozs7OycnJysrKysr"
    "Jw0KKysrKysrOzo6Ojo6Ojo6Oiw6Ojs6OiwuLjo6Ojo7Oyc7Ozs7Jyc6OiwsLCwsOjo6Ojo6Oj"
    "o7Ozs7OycnJysrKysrJw0KKysrKysrOjo6Ojo6Ojo6Oiw6Ojo6Oiw6Oiw6Ojs7Ozs7Ojs7Oyc7"
    "Ojo6LCwsOjo6Ojo6Ojo7OzsnJycnJysrKysrKw0KKysrKysrOjo6Ojo6Ojo6Oiw6Ojo6Ojo6Oj"
    "o6Ozs7Ozs7Ojs7OycnOjo6OjosOjo6Ojo6Ojs7OzsnJycnKysrKysrKw0KKysrKysrOzo6Ojo6"
    "Ojo6Ojo6Ojo7Ozo6Ojo7OycnOzs7Ozs7JysrOjo6Ojo6Ojo6Ojo6Ozs7OycnJycnKysrKysrKw"
    "0KKysrKysrOzo6Ojo6Ojo6Ojo6Ozs7Ozs7OzsnJycrIyMrKycnJyMrOzo6Ojo6Ojo6Ojo7Ozs7"
    "OycnJycnKysrKysrKw0KKysrKysrOzo6Ojo6Ojo6Ojo6OysjIysnJycnKyMjIyMjIysjIyMrOz"
    "s6Ojo6Ojo6Ozs7Ozs7JycnJycnKysrKysrKw0KKysrKysrOzs6Ojo6Ojo6Ojs7OycjIyMjIysr"
    "IyMjIyMjIysrKysnOzs7Ojo6Ojo7Ozs7Ozs7JycnJycrKysrKysrKw0KKysrKysrOzs6Ozo6Oz"
    "s7Ozs6OzonJycnKysjIyMjIysrJycnJzs7Ozs7Ozs6Ojs7Ozs7Ozs7JycnJycrKysrKysrKw0K"
    "KysrKysrJzs7Ozs7Ozs7Ozs7Ojs7Ozs7OycrKysnJycnJycnJzs7Ozs7Ozs7Ozs7Ozs7OzsnJy"
    "cnJycrKysrKysrKw0KKysrKysrKzs7Ozs7OzsnOzs7Ozs7Ozs7Ozs7Oyc7Ozs7Ozs7Jzs7Jyc7"
    "Ozs7Ozs7Ozs7OzsnJycnJycrKysrKysrKw0KKysrKysrKzs7Ozs7OycnOzs7Ozs7Ozs7Ozs6Oj"
    "s7Ojs7Ozs7OycnJycnOzs7Ozs7Ozs7OycnJycnJycrKysrKysrKw0KKysrKysrKyc7Ozs7Oyc7"
    "Ozs7Ozs7Ojo6Ojo6Ozo6Ojo7Ozs7Ozs7OycnOzs7Ozs7Ozs7OzsnJyc7JycrKysrKysrKw0KKy"
    "srKysrKyc7Ozs7Oyc7Ozs7Ojs7Ozs7Ozo6Ojo7OzsnJycnJzs7JycnOzs7Ozs7Ozs7Ozs7Jycn"
    "JycrKysrKysrKw0KKysrKysrKyc7Ozs7Ozs7Ozs7Ozs7Ozs7Ozs7Ozs7JycnJysrKysrJysnOz"
    "s7Ozs7Ozs7OzsnJyc7JycnKysrKysrKw0KKysrKysrKys7Ozo6Ojs7OjsnJycnJzs7OycnJycn"
    "KyMjIyMjIyMrKysjJzs7Ozs7Ojo7OzsnJzs7JycnKysrKysrKw0KKysrKysrKys7Ozs6Ojo7Oz"
    "s7JyMjIyMjIysrKyMjIyMrJycnKysjIyMrOzs7Ozs7Ojo7OzsnOzs7JycnKysrKysrKw0KKysr"
    "KysrKysnOzs6Ojo7Ozo7Ozs7Ozs7Ozs7Ozs7Ozs7Ozs7OycnJyc7Ojs7Ozs6Ojs7OzsnJzs7Jy"
    "crKysrKysrKw0KKysrKysrKysrOzs6Ojo6Ozs6Ojs7Ozs7Ojo6Ojo6Ojs7Ozs7Oyc7Jzs7Ojo7"
    "Ozo7Ojs7Ozs7Ozs7OycrKysrKysrKw0KKysrKysrKysrOzs7Ojo6Ojs6Ojs7Ozs7OiwsLCwsOj"
    "s7Ozs7JzsnOzo6Ojo7Ojo6Ozs7Jyc7Ozs7JycrKysrKysrKw0KKysrKysrKysrJzs7Ojo6Ojo6"
    "Ojo7Ozs7Ozs7Ojo6OzsnJycnJzs7Ozo6Ojo6Ojo6Ozs7JzsnOzsnJysrKysrKysrKw0KKysrKy"
    "srKysrKzs7Ozo6Ojo6Ojo7Ozs7Ozs7OzsnJycnJzs7Ozs7Ojo6Ozs6Ojo7Ozs7Ozs7OzsnKysr"
    "KysrKysrKw0KKysrKysrKysrKyc7Ozo6Ojo6Ojo6Ozs7OzsnOyc7Ozs7Ozo7Ozs6Ozs7Ozs6Oz"
    "s7Ozs7Ozs7OycnKysrKysrKysrKw0KKysrKysrKysrKys7Ozs6Ojo6Ojo6Ozs7Ozs7Ozs7Ozo6"
    "Ojo6Ojs7Ozs7Ozs7Ozs7Ozs7OzsnJycrKysrKysrKysrKw0KKysrKysrKysrKysnOzs6Ojs7Oj"
    "o6Ojo6Ojo6Ozs7Ozo6Ojo6Ojs7Ozs7Ojs7Ozs7Ozs7OycnJysrKysrKysrKysrKw0KKysrKysr"
    "KysrKysrOzs7Ojs6Ojo6Ojo6Ojo7Ojo6Ojo6Ojo6Ojs7Ozs7Ozs7Ozs7OzsnJycnJysrKysrKy"
    "srKysrKw0KKysrKysrKysrKysrJzs7Ozs7Ojo6Ojo6Ojo6Ojo6Ojo6Ojo6Ozs7Ozs7Ozs7Ozs7"
    "OycnJycnKysrKysrKysrKysrJw0KKysrKysrKysrKysrKyc7Ozs7Ojo6Ojo6Ojo6OjosOjo6Oj"
    "o6Ojo7Ozo7Ozs7Ozs7JycnJycrKysrKysrKyMrKysnJw0KKysrKysrKysrKysrKyc7Ozs7Ojo6"
    "Ojo6Ojo6Ojo6Ojo6Ojo6Ojo6Ozs7OzsnJycnJycnJysrKysrKysrIyMrKysnJw0KKysrKysrKy"
    "srKysrKysnOzs7Ojo6Ojo6Ojo6LCwsLDo6Ojo6Ojo6Ozs7OzsnJycnJycnKysrKysjIyMjIyMj"
    "KycnOw0KKysrKysrKysrKysrKysrJzs7Ozo6Ojo6OiwsLDosLDo7Ojo6Ojo6Ozs7OycnJycrKy"
    "crKysjIyMjIyMjIysrJycnJw0KKysrKysrKysrKysrKysrKyc7Ozo6Ojs6Ojo6OjosLDo7Ozo7"
    "Ojs7OzsnJycnKysrKysrIyMjIyMjIyMjIysnJycnJw0KKysrKysrKysrKysrKysrKycnOzs6Oj"
    "s7Ojs7OjosOjs6Ozs6Ozs7OycnJysrKysrKysjIyMjIyMjIyMjKycnJycnJw0KKysrKysrKysr"
    "KysrKysrKysrOzs6Ojo7Ozs7Ozs7Ozs6Ojo7Ozs7JycnKysrKysrKyMjIyMjIyMjIyMrJycnJy"
    "cnJw0KKysrKysrKysrKysrKysrKysrJzs7Ojs7Ozs7Ozs7Ozs6Ozs7Ozs7JycrKysrKysjIyMj"
    "IyMjIyMjIysnJycnJycnJw0KKysrKysrKysrKysrKysrKysrKyc7Ojs7Ozs7Ozs7Ozs7Ozs7Oy"
    "cnJysrKysrKysrIysjIyMjIyMjKysnJycnJycnOw0KKysrKysrKysrKysrKysrKysrKys7Ozs7"
    "OzsnJzsnJyc7Jzs7JycrKysrKysrKysrIyMjIyMjIyMrKycnJycnJzs7Ow0KKysrKysrKysrKy"
    "srKysrKysrKyMrJzs7OzsnJzs7Jyc7JycnKysrKysrKysrKysjIyMjIyMjIysrJycnJycnOzs7"
    "Ow0KKysrKysrKysrKysrKysrKysrKysrKycnJzsnOzs7OycnJysrKysrKysrKysrKysrIysjIy"
    "MjKysnJycnJyc7Ozo6Og0KICAgICAgICAgX19fICBfX18gICAgICAgX19fX19fICAgICAgICAg"
    "ICBfICAgICAgIA0KICAgICAgICAgfCAgXC8gIHwgICAgICAgfCBfX18gXCAgICAgICAgIHwgfC"
    "AgICAgIA0KICAgICAgICAgfCAuICAuIHxfICAgXyAgfCB8Xy8gLyBfX18gICBfX3wgfF8gICBf"
    "IA0KICAgICAgICAgfCB8XC98IHwgfCB8IHwgfCBfX18gXC8gXyBcIC8gX2AgfCB8IHwgfA0KIC"
    "AgICAgICAgfCB8ICB8IHwgfF98IHwgfCB8Xy8gLyAoXykgfCAoX3wgfCB8X3wgfA0KICAgICAg"
    "ICAgXF98ICB8Xy9cX18sIHwgXF9fX18vIFxfX18vIFxfXyxffFxfXywgfA0KICAgICAgICAgIC"
    "AgICAgICAgX18vIHwgICAgICAgICAgICAgICAgICAgICBfXy8gfA0KICAgICAgICAgICAgICAg"
    "ICB8X19fLyAgICAgICAgICAgICAgICAgICAgIHxfX18vIA0KICAgICAgICAgIF9fX19fICAgIC"
    "BfX19fX18gICAgICAgICAgICAgICBfICAgICAgIA0KICAgICAgICAgfF8gICBffCAgICB8IF9f"
    "XyBcICAgICAgICAgICAgIHwgfCAgICAgIA0KICAgICAgICAgICB8IHwgX19fICB8IHxfLyAvX1"
    "9fICBfXyBfICBfX3wgfF8gICBfIA0KICAgICAgICAgICB8IHwvIF9ffCB8ICAgIC8vIF8gXC8g"
    "X2AgfC8gX2AgfCB8IHwgfA0KICAgICAgICAgIF98IHxcX18gXCB8IHxcIFwgIF9fLyAoX3wgfC"
    "AoX3wgfCB8X3wgfA0KICAgICAgICAgIFxfX18vX19fLyBcX3wgXF9cX19ffFxfXyxffFxfXyxf"
    "fFxfXywgfA0KICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICBfXy"
    "8gfA0KICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIHxfX18vIA="
    "=";
const string not_welcome =
    "ICAgICAgICAgICAgKyAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAjIC"
    "AgICAgICAgICAgDQogICAgICAgICAgICMjIyMgICAgICAgICAgICAgICAgICAgICAgICAgICAg"
    "ICAgICAgICAjIyM6ICAgICAgICAgICANCiAgICAgICAgICArIyMjIyMrICAgICAgICAgICAgIC"
    "AgICAgICAgICAgICAgICAgICArIyMjIyMgICAgICAgICAgIA0KICAgICAgICAgICMjOiMjIyMj"
    "LCAgICAgICAgICAgICAgICAgICAgICAgICAgICAuIyMjIyM7IyMgICAgICAgICAgDQogICAgIC"
    "AgICAuIyc7OiMjIyMjIyAgICAgICAgICAgICAgICAgICAgICAgICAgIyMjIyMjOjsnI2AgICAg"
    "ICAgICANCiAgICAgICAgICMjOzs7OyMjIyMjIywgICAgICAgICAgICAgICAgICAgICAgLiMjIy"
    "MjIzs7OzsjIyAgICAgICAgIA0KICAgICAgICA6IyM6Ozs7OyMjIyMjIysgICAgICAgICAgICAg"
    "ICAgICAgICsjIyMjIyM7Ozs7OyMjICAgICAgICAgDQogICAgICAgICMjOzs7Ozs7OyMjIyMjIy"
    "MgICAgICAgICAgICAgICAgICAjIyMjIyMjOzs7Ozs7OyM7ICAgICAgICANCiAgICAgICAgIyM7"
    "Ozs7Ozs7OyMjIyMjIyMgICAgICAgICAgICAgICAgIyMjIyMjKzs7Ozs7Ozs7IyMgICAgICAgIA"
    "0KICAgICAgICAjIzs7Ozs7Ozs7OyMjIyMjIyMgICAgICAgICAgICAgICMjIyMjIyM6Ozs7Ozs7"
    "OzsjIyAgICAgICAgDQogICAgICAgJyMjOzs7Ozs7Ozs7OyMjIyMjIyMgICAgICAgICAgICAjIy"
    "MjIyMjOzs7Ozs7Ozs7OyMjYCAgICAgICANCiAgICAgICAnIys7Ozs7Jzs7Ozs7JyMjIyMjIysg"
    "ICAgICAuICAgKyMjIyMjIyc7Ozs7Ozs7Ozs7KyM6ICAgICAgIA0KICAgICAgICsjJzs7OzsjOj"
    "s7Ozs7IyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjOzsrOjs7Ozs7JzsnIyMgICAgICAgDQogICAg"
    "ICAgKyMnOyM7OycjOzs7OjojIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMnOyMjOyMnOys7OycjIy"
    "AgICAgICANCiAgICAgICAjIzs7OyMjOyMjKyMjKyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMj"
    "IyMjIysrJzsnJyMjICAgICAgIA0KICAgICAgICMjJzsrOyMjIyMjIyMjIyMjIyMjIyMjIyMjIy"
    "MjIyMjIyMjIyMjIyMjIyMjIyM7IzsnIyMgICAgICAgDQogICAgICAgIyMnOzs7IyMjIyMjIyMj"
    "IyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyM7OycjI2AgICAgICANCiAgICAgICAjIy"
    "srIyMjIyMjIyMjIyMjIyMjIyA7KyMjIyMjIyMjIyM7OiMjIyMjIyMjIyMjIyM7KyMjICAgICAg"
    "IA0KICAgICAgICMjIzsnIyMjIyMjIyMjIyMjIyA7Ozs7IyMjIyMjIyNgOi4jIyMjIyMjIyMjIy"
    "MjIzsjIyMgICAgICAgDQogICAgICAgIyMjIyMjIyMjIyMjIyMjIyMjLDs7Ozs7KyMjIyM6Ozs6"
    "YDojIyMjIyMjIyMjIyMjIyMjIyAgICAgICANCiAgICAgYCMjIyMjIyMjIyMjIyMjIyMjIyMjYD"
    "s7Ozs6IyMjOjs7OzsgIyMjIyMjIyMjIyMjIyMjIyMjJyAgICAgIA0KICAgICA6IyMjIyMjIGAg"
    "ICArOysjIyMjIyMjOzs7OzsnIyM7Ozs7YCMjIyM6ICAgICBgOyMjIyMjIyMjOiAgICAgDQogIC"
    "AgICMjIyMjLiAnOiMjIyM7JyAjKyMjIyM7Ozs7OzsjOjs7OyAjIyMjKzsgICAgICAgICBgIyMj"
    "IyMjICAgICANCiAgICA7IyMjIycgIyMjIyMjIywgOzorIyMnKyMuOzs7Ozs7OzpgIyMjIyMnOy"
    "AuICAgICAgICAgOiMjIyMrICAgIA0KICAgICMjIyMjYCAjIyMjIzsgICMjICwrIyMjI2A7Ozs7"
    "Ozs7LiMjIyMjIyM7ICAgYDs6YGAgICAgLCMjIyNgICAgDQogICAjIyMjIyMgYCMjIyMsICAjIy"
    "NgIC4jIyMjIyw7Ozs7OzsrIyMjIyMjKzogICBgIysjIyMgICAgIyMjIyMgICANCiAgICMjIyMj"
    "YCAuIyMjOyBgJyMrIy4gKyMjIyMjIzs7Ozs7OysjIyMjIyMnLiAgICAuIyMjIyMgICBgIyMjIy"
    "cgIA0KICArIyMjIyMgIGAjIztgYDojIyMjICArIyMjIyMjOzs7Ozs7OiMjIyMjIyMuICAgICMj"
    "IyMjIy4gICAjIyMjIyAgDQogICMjIyMjIyAgICMnLiAnIyMjKzonOiMjIyMjOzs7Ozs7Ozs7Li"
    "MjIyMjIyAgLCAjIyMjIyMjICwgOyMjIyMjICANCiAgIyMjIyMjOyAgLCAsLCMjIyMjLCAjIyMj"
    "IyA7Ozs7Ozs7Ozs7LCMjIyMjICAgLCMjIyMjIysuLGAjIyMjIyMsIA0KIGAjIyMjIyMuICAgIC"
    "csIysjIywsIyMjIyMjOjs7OzsgLDs7Ozs7OyMjIyMuICArIyMjIyMrLiBgIyMjIyMjIzogDQog"
    "OiMjIyMjIyssICAgICAgOy5gOiMjIyMjIyc7Ozs7LiMjIDs7Ozs7KyMjIzsgICAgOiAgICBgOi"
    "MjIyMjIyMjIyANCiA7IyMjIyMjIyMjIzsuLCw6IyMjIyMjIyMjOzs7OmAjIyMjLDs7OzssIyMj"
    "Iy5gJy4uYDonIyMjIyMjIyMjIyMsIA0KICAjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIy47O2A6Iy"
    "MjIyMjOzs6LmBgIyMjIyMjOiMjIyMjIyMjIyMjIyMjIyAgDQogICMjIyMjIyMjIyMjIyMjIyMj"
    "IyMjIyMnOjsgIyMsICMuICMjYC4sYCsjIyMjIyMjIyMjIyMjIyMjIyMjIyMjICANCiAgIyMjIy"
    "MjIyMjIyMjIyMjIyMjIyMjIyBgICMjIzsjIyMsIyMjIC4rIyMjIyMjIyMjIyMjIyMjIyMjIyMj"
    "IyMgIA0KICBgIyMjIyMjIyMjIyMjIyMjIyMjIyMjYCMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIy"
    "MjIyMjIyMjIyMjIyMjOyAgDQogICAjIyMjIyM6IyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMj"
    "IyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMgICANCiAgICAjIyMjIysgICsgICMjICAgIyM7Oi"
    "AuJzssLCs7I2A7Oi4uIC5gIDsjLCAgYCMsICAgKyBgIyMjIyMjOyAgIA0KICAgIDsjIyMjIyMj"
    "KyAgIyAgICAuLCAgICAgICAgICBAICAgICAgICAgICMgICAgLCAgICAsICMjIyMjIyMgICAgDQ"
    "ogICAgICMjIyMsIyMjICAgICAgICArICAgICAgICAgICMgICAgICAgICAgOiAgICAjICAgIyM6"
    "IyMjIyMjICAgICANCiAgICAgICMjIyMjYCwjLCAnICAgICMgICAgICAgICAgJyAgICAgICAgIC"
    "A7ICAgICMjIztgIyMjIyMjIyAgICAgIA0KICAgICAgICMjIyMjI2AgJyMjICAgLC4gICAgICAg"
    "ICA7ICAgICAgICAgJyMsIyMjLCAjIyMjIyMjIyMgICAgICAgDQogICAgICAgIDsjIyMjIyMgKy"
    "M6LCwrI2AgICAgICAgICMgICAgICAgOiMjIyxgIyMgIyMjIyMjIyM7ICAgICAgICANCiAgICAg"
    "ICAgICAjIyMjIyMjLiAgICAgIyMnOzsnKyMjIyMjIyMrOzsjIyAgIGAjIyMjIyMjIyMjICAgIC"
    "AgICAgIA0KICAgICAgICAgICAsIyMjIyMjIycjOyMjICAgICAgICAjICAgICAgICAjICBgIyMj"
    "IyMjIyMjOyAgICAgICAgICAgDQogICAgICAgICAgICAgLiMjIyMjIyMjIyMgLiAgICAgICsgIC"
    "AgOyMjIyMjIyMjIyMjIyMjOyAgICAgICAgICAgICANCiAgICAgICAgICAgICAgICAnIyMjIyMj"
    "IyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjYCAgICAgICAgICAgICAgIA0KICAgICAgICAgIC"
    "AgICAgICAgICwrIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIzsgICAgICAgICAgICAgICAgICAg"
    "DQogICAgICAgICAgICAgICAgICAgICAgICAgYDojIyMjIyMjIyMjKyc6LiAgICAgICAgICAgIC"
    "AgICAgICAgICAgICANCiAgICAgICAgICAgICAgICAgICAgICAgICAgICA6YCcjIyMnKy4gICAg"
    "ICAgICAgICAgICAgICAgICAgICAgICAgIA0KICAgICAgICAgICAgICAgICAgICAgICAgICAgIC"
    "AgICAnLg==";
const string go_away[] = {
    "None of this is real", "Whatever you do, don't turn around", "It's me...",
    "You can't save them", "I'm so sorry", "They know... all of it",
    "Count the shadows", "Go back to sleep", "I need your skin",
    "I'm behind you", "I'm coming to find you...", "Shrek is Love",
    "It's inside you",
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN NYAN "
    "NYAN NYAN NYAN NYAN"};

atomic<int> score;
ProblemContainer problems;
boost::array<bool, 4> expectedResult;
vector<int> problemIndex;
atomic<int> problemScore;
atomic<bool> expired;

std::random_device rd;
std::default_random_engine e1(rd());
std::uniform_int_distribution<int> uniform_dist(0, 5);
std::uniform_int_distribution<int> uniform_dist2(0, 13);
std::bernoulli_distribution bool_dist1(0.2);
std::bernoulli_distribution bool_dist2(0.01);

class TCPConnection : public boost::enable_shared_from_this<TCPConnection> {
public:
  typedef boost::shared_ptr<TCPConnection> pointer;

  static pointer create(boost::asio::io_service &IOService) {
    return pointer(new TCPConnection(IOService));
  }

  tcp::socket &socket() { return Socket; }

  void start() {
    sendData();
    readData();
  }

private:
  TCPConnection(boost::asio::io_service &IOService)
      : Socket(IOService), Timer(IOService) {}

  void stop() {
    Socket.close();
    Timer.cancel();
  }

  void readData() {
    boost::asio::async_read(Socket, boost::asio::buffer(ReadMessage),
                            boost::bind(&TCPConnection::onDataReceived,
                                        shared_from_this(),
                                        boost::asio::placeholders::error));
  }

  void onDataReceived(const boost::system::error_code &ec) {
    if (ec) {
      cout << "I f****** quit !!!" << endl
           << "Oh btw, network error, client crashed, connection reset, "
              "armagueddon, 9/11 or somethin'...." << endl;
      stop();
      // throw exception("bye bye !");
      throw exception();
      return;
    }

    Timer.expires_from_now(boost::posix_time::seconds(5));
    Timer.async_wait(boost::bind(&TCPConnection::onDataTimerExpired,
                                 shared_from_this(),
                                 boost::asio::placeholders::error, &Timer));
    for (int i = 0; i < 4; ++i) {
      if (ReadMessage[i] != expectedResult[i]) {
        score -= problemScore;
        cout << "YOU'RE WRONG !!!! -" << problemScore << endl;
        sendData();
        readData();
        return;
      }
    }
    score += problemScore * 2;
    cout << "well alright... +" << problemScore * 2 << endl;
    sendData();
    readData();
  }

  void sendData() {
    int next;
    int index;
    // BREAK
    if (std::accumulate(problemIndex.begin(), problemIndex.end(), 0) >=
        problems.getGlobalSize()) {
      cout << "bye bye !" << endl;
      stop();
      // throw exception("bye bye !");
      throw exception();
    }
    do {
      next = uniform_dist(e1);
      index = problemIndex[next];
    } while (index >= problems.getSize(next));
    int size;
    vector<boost::asio::const_buffer> bufs;

    bufs.push_back(boost::asio::buffer(&next, sizeof next));
    switch (next) {
    // MAZE
    case 0:
      problemScore = 2;
      for (int i = index; i < index + 4; ++i) {
        auto problem = problems.getMaze(i);
        size = problem.second.size();
        bufs.push_back(boost::asio::buffer(&size, sizeof size));
        bufs.push_back(
            boost::asio::buffer(reinterpret_cast<const char *>(&problem.second),
                                problem.second.size() * sizeof(int)));
        expectedResult[i % 4] = problem.first;
      }
      break;
    // SUDOKU
    case 1:
      problemScore = 2;
      for (int i = index; i < index + 4; ++i) {
        auto problem = problems.getSudoku(i);
        size = problem.second.size();
        bufs.push_back(boost::asio::buffer(&size, sizeof size));
        bufs.push_back(
            boost::asio::buffer(reinterpret_cast<const char *>(&problem.second),
                                problem.second.size() * sizeof(int)));
        expectedResult[i % 4] = problem.first;
      }
      break;
    // ARRAY
    case 2:
      problemScore = 1;
      for (int i = index; i < index + 4; ++i) {
        auto problem = problems.getArray(i);
        size = problem.second.first;
        bufs.push_back(boost::asio::buffer(&size, sizeof size));
        size = problem.second.second.size();
        bufs.push_back(boost::asio::buffer(&size, sizeof size));
        bufs.push_back(boost::asio::buffer(
            reinterpret_cast<const char *>(&problem.second.second),
            problem.second.second.size() * sizeof(int)));
        expectedResult[i % 4] = problem.first;
      }
      break;
    // SYM TREE
    case 3:
      problemScore = 2;
      for (int i = index; i < index + 4; ++i) {
        auto problem = problems.getTree(i);
        size = problem.second.size();
        bufs.push_back(boost::asio::buffer(&size, sizeof size));
        bufs.push_back(
            boost::asio::buffer(reinterpret_cast<const char *>(&problem.second),
                                problem.second.size() * sizeof(int)));
        expectedResult[i % 4] = problem.first;
      }
      break;
    // PASSWORD
    case 4:
      problemScore = 1;
      for (int i = index; i < index + 4; ++i) {
        auto problem = problems.getPassword(i);
        size = problem.second.size();
        bufs.push_back(boost::asio::buffer(&size, sizeof size));
        bufs.push_back(
            boost::asio::buffer(reinterpret_cast<const char *>(&problem.second),
                                problem.second.size() * sizeof(int)));
        expectedResult[i % 4] = problem.first;
      }
      break;
    // RLE
    case 5:
      problemScore = 1;
      for (int i = index; i < index + 4; ++i) {
        auto problem = problems.getRLE(i);
        size = problem.second.first;
        bufs.push_back(boost::asio::buffer(&size, sizeof size));
        size = problem.second.second.size();
        bufs.push_back(boost::asio::buffer(&size, sizeof size));
        bufs.push_back(boost::asio::buffer(
            reinterpret_cast<const char *>(&problem.second.second),
            problem.second.second.size() * sizeof(int)));
        expectedResult[i % 4] = problem.first;
      }
      break;
    default:
      break;
    }
    // cout << "Problem type : " << next << "\tscore : " << problemScore <<
    // endl;
    problemIndex[next] += 4;

    boost::asio::async_write(
        Socket, bufs,
        boost::bind(&TCPConnection::handleWrite, shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));

    Timer.expires_from_now(boost::posix_time::seconds(5));
    Timer.async_wait(boost::bind(&TCPConnection::onDataTimerExpired,
                                 shared_from_this(),
                                 boost::asio::placeholders::error, &Timer));
  }

  void onDataTimerExpired(const boost::system::error_code &ec,
                          boost::asio::deadline_timer *timer) {
    if (ec == boost::asio::error::operation_aborted) { // 3.
      {
        cout << "Abort" << endl;
        return;
      }

      cout << "Awww.... too slow -" << problemScore << endl;
      score -= problemScore;
      sendData();
    }
  }

  void handleWrite(const boost::system::error_code & /*error*/,
                   size_t /*bytes_transferred*/) {
    cout << "You have " << score << " points, sending new problem" << endl;
  }

private:
  tcp::socket Socket;
  boost::array<bool, 4> ReadMessage;
  boost::asio::deadline_timer Timer;
};

class TCPServer {
public:
  TCPServer(boost::asio::io_service &IOService)
      : Acceptor(IOService, tcp::endpoint(tcp::v4(), 22022)) {
    startAccept();
  }

private:
  void startAccept() {
    TCPConnection::pointer NewConnection =
        TCPConnection::create(Acceptor.get_io_service());

    Acceptor.async_accept(NewConnection->socket(),
                          boost::bind(&TCPServer::handleAccept, this,
                                      NewConnection,
                                      boost::asio::placeholders::error));
  }

  void handleAccept(TCPConnection::pointer NewConnection,
                    const boost::system::error_code &error) {
    if (!error) {
      std::cout << "Here we go !" << std::endl;
      NewConnection->start();
    }

    startAccept();
  }

  tcp::acceptor Acceptor;
};

void readMaze(string name, ProblemContainer &problems) {
  ifstream file(name, ios::in | ios::binary);

  file.seekg(0, file.end);
  int length = file.tellg();
  file.seekg(0, file.beg);
  // Read Mazes
  while (length > 0) {
    // Read the flag
    unsigned char flag = file.get();
    --length;
    // Read the 4 mazes
    for (int i = 0; i < 4; ++i) {
      // Read single maze size
      int size;
      file.read((char *)&size, sizeof size);
      length -= sizeof size;
      // Read the whole maze
      vector<int> maze;
      maze.reserve(size);
      for (int j = 0; j < size; ++j) {
        int value;
        file.read((char *)&value, sizeof(value));
        length -= sizeof value;
        maze.push_back(value);
      }
      problems.addMaze(make_pair(flag & (1 << i), maze));
    }
  }
}

void readSudoku(string name, ProblemContainer &problems) {
  ifstream file(name, ios::in | ios::binary);

  file.seekg(0, file.end);
  int length = file.tellg();
  file.seekg(0, file.beg);
  // Read Mazes
  while (length > 0) {
    // Read the flag
    unsigned char flag = file.get();
    --length;
    // Read the 4 mazes
    for (int i = 0; i < 4; ++i) {
      // Read single maze size
      int size;
      file.read((char *)&size, sizeof size);
      length -= sizeof size;
      // Read the whole maze
      vector<int> sudoku;
      sudoku.reserve(size);
      for (int j = 0; j < size; ++j) {
        int value;
        file.read((char *)&value, sizeof(value));
        length -= sizeof value;
        sudoku.push_back(value);
      }
      problems.addSudoku(make_pair(flag & (1 << i), sudoku));
    }
  }
}

void readArray(string name, ProblemContainer &problems) {
  ifstream file(name, ios::in | ios::binary);

  file.seekg(0, file.end);
  int length = file.tellg();
  file.seekg(0, file.beg);
  // Read Mazes
  while (length > 0) {
    // Read the flag
    unsigned char flag = file.get();
    --length;
    // Read the 4 mazes
    for (int i = 0; i < 4; ++i) {
      // Read single maze size
      int size;
      file.read((char *)&size, sizeof size);
      length -= sizeof size;
      // Read the whole maze
      pair<int, vector<int>> array;
      array.second.reserve(size - 1);
      for (int j = 0; j < size; ++j) {
        int value;
        file.read((char *)&value, sizeof(value));
        length -= sizeof value;
        if (j == 0)
          array.first = value;
        else
          array.second.push_back(value);
      }
      problems.addArray(make_pair(flag & (1 << i), array));
    }
  }
}

void readTree(string name, ProblemContainer &problems) {
  ifstream file(name, ios::in | ios::binary);

  file.seekg(0, file.end);
  int length = file.tellg();
  file.seekg(0, file.beg);
  // Read Mazes
  while (length > 0) {
    // Read the flag
    unsigned char flag = file.get();
    --length;
    // Read the 4 mazes
    for (int i = 0; i < 4; ++i) {
      // Read single maze size
      int size;
      file.read((char *)&size, sizeof size);
      length -= sizeof size;
      // Read the whole maze
      vector<int> array;
      array.reserve(size);
      for (int j = 0; j < size; ++j) {
        int value;
        file.read((char *)&value, sizeof(value));
        length -= sizeof value;
        array.push_back(value);
      }
      problems.addTree(make_pair(flag & (1 << i), array));
    }
  }
}

void readPassword(string name, ProblemContainer &problems) {
  ifstream file(name, ios::in | ios::binary);

  file.seekg(0, file.end);
  int length = file.tellg();
  file.seekg(0, file.beg);
  // Read Mazes
  while (length > 0) {
    // Read the flag
    unsigned char flag = file.get();
    --length;
    // Read the 4 mazes
    for (int i = 0; i < 4; ++i) {
      // Read single maze size
      int size;
      file.read((char *)&size, sizeof size);
      length -= sizeof size;
      // Read the whole maze
      char *buffer = new char[size];
      file.read(buffer, size);
      length -= size;

      string value(buffer);
      value.resize(size);
      problems.addPassword(make_pair(flag & (1 << i), value));
      delete[] buffer;
    }
  }
}

void readRLE(string name, ProblemContainer &problems) {
  ifstream file(name, ios::in | ios::binary);

  file.seekg(0, file.end);
  int length = file.tellg();
  file.seekg(0, file.beg);
  // Read Mazes
  while (length > 0) {
    // Read the flag
    unsigned char flag = file.get();
    --length;

    int answer;
    file.read((char *)&answer, sizeof answer);
    length -= sizeof answer;
    // Read the 4 mazes
    for (int i = 0; i < 4; ++i) {
      // Read single maze size
      int size;
      file.read((char *)&size, sizeof size);
      length -= sizeof size;
      // Read the whole maze
      char *buffer = new char[size];
      file.read(buffer, size);
      length -= size;

      string value(buffer);
      value.resize(size);
      problems.addRLE(make_pair(flag & (1 << i), make_pair(answer, value)));
      delete[] buffer;
    }
  }
}

int main() {
  // e1.seed(1);

  readMaze("maze_small.bin", problems);
  readSudoku("sudoku_small.bin", problems);
  readArray("array_small.bin", problems);
  readPassword("password_small.bin", problems);
  readTree("tree_small.bin", problems);
  readRLE("RLE_small.bin", problems);

  cout << problems.getGlobalSize() << " problems loaded" << endl;

  problemIndex.resize(6);

  try {
    boost::asio::io_service IOService;
    TCPServer Server(IOService);
    if (bool_dist2(e1)) {
      std::cout << base64_decode(not_welcome) << std::endl << std::endl;
      for (;;) {
        for (int t = 0; t < 5; ++t) {
          std::cout << "0xD ";
          this_thread::sleep_for(chrono::milliseconds(200));
        }
        std::cout << "\t" << go_away[uniform_dist2(e1)] << "\t";
        for (int t = 0; t < 7; ++t) {
          std::cout << " 0xD";
          this_thread::sleep_for(chrono::milliseconds(200));
        }
        std::cout << endl;
        this_thread::sleep_for(chrono::milliseconds(2000));
      }
    } else if (bool_dist1(e1)) {
      std::cout << base64_decode(welcome) << std::endl;
      std::cout << "...Waiting for \"client\"..." << std::endl;
    } else {
      std::cout << "Waiting for client..." << std::endl;
    }
    IOService.run();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
  for (int i = 0; i < 5; ++i)
    cout << "FINAL SCORE " << score << endl;

  return 0;
}
