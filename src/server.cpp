#include "base64.h"
#include "strings.h"

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

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
           << "Oh btw, network error, client crashed, connection reset, armagueddon, 9/11 or somethin'...." << endl;
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
                          boost::asio::deadline_timer *) {
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

int main(int argc, char** argv) {
  cout << argv[1] << endl;
  readMaze(argc > 1 ? argv[1] : "maze_small.bin", problems);
  readSudoku(argc > 2 ? argv[2] : "sudoku_small.bin", problems);
  readArray(argc > 3 ? argv[3] : "array_small.bin", problems);
  readPassword(argc > 4 ? argv[4] : "password_small.bin", problems);
  readTree(argc > 5 ? argv[5] : "tree_small.bin", problems);
  readRLE(argc > 6 ? argv[6] : "RLE_small.bin", problems);

  cout << problems.getGlobalSize() << " problems loaded" << endl;

  problemIndex.resize(6);

  try {
    boost::asio::io_service IOService;
    TCPServer Server(IOService);
    if (bool_dist2(e1)) {
      std::cout << base64_decode(not_welcome) << std::endl
                << std::endl;
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
