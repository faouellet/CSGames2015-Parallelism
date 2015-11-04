#include "base64.h"
#include "strings.h"

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <random>
#include <thread>

using namespace std;
using boost::asio::ip::tcp;

namespace
{
    template <class T>
    struct ContainerTraits
    {
        using iterator = typename T::iterator;

        static size_t size(const T& t) { return t.size(); }
        static iterator begin(const T& t) { return begin(t); }
        static iterator end(const T& t) { return end(t); }
    };

    template <class T1, class T2>
    struct ContainerTraits<std::pair<T1, T2>>
    {
        using iterator = typename T2::iterator;

        static size_t size(const pair<T1, T2>& p) { return p.second.size(); }
        static iterator begin(const pair<T1, T2>& p) { return begin(t); }
        static iterator end(const pair<T1, T2>& p) { return end(t); }
    };
}

class BaseProblem
{
public:
    BaseProblem() { }
    virtual ~BaseProblem() { }

public:
    virtual bool getAnswer() const = 0;
    virtual boost::optional<size_t> getExpectedValue() const = 0;
    virtual size_t size() const = 0;
};

template <class T>
class ConcreteProblem : BaseProblem
{
public:
    using iterator = typename ContainerTraits<T>::iterator;

public:
    ConcreteProblem(bool answer, T&& t) : mAnswer{ answer }, mData { t } { }
    virtual ~ConcreteProblem() { }

public:
    bool getAnswer() const override { return mAnswer; }
    boost::optional<size_t> getExpectedValue() const override { return mExpected; }

    size_t size() const override { return ContainerTraits<T>::size(data); }

    iterator begin() const { return ContainerTraits<T>::begin(data); }
    iterator end() const { return ContainerTraits<T>::end(data); }

private:
    bool mAnswer;
    boost::optional<size_t> mExpected;
    T mData;
};

enum ProblemType
{
    MAZE,
    SUDOKU,
    TREE,
    ARRAY,
    PASSWORD,
    RLE
};

class ProblemContainer {
public:
    template <class T>
    void addProblem(ProblemType type, bool answer, T&& data)
    {
        mProblems[type].emplace_back(answer, data);
        ++mGlobalSize;
    }
    
  BaseProblem* getProblem(ProblemType type, int index) { return mProblems[type][index].get(); }

  size_t getProblemSize(ProblemType type) {
      return mProblems[type].size();
  }

  size_t getGlobalSize() const { return mGlobalSize; }

private:
  vector<vector<unique_ptr<BaseProblem>>> mProblems;
  size_t mGlobalSize;
};

atomic<int> score;
ProblemContainer problems;
boost::array<bool, 4> answers;
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
      if (ReadMessage[i] != answers[i]) {
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
    } while (index >= problems.getProblemSize(static_cast<ProblemType>(next)));
    size_t dataSize;
    vector<boost::asio::const_buffer> bufs;

    bufs.push_back(boost::asio::buffer(&next, sizeof(next)));
    
    problemScore = next < 3 ? 2 : 1;

    for (int i = index; i < index + 4; ++i) {
    auto problem = problems.getProblem(static_cast<ProblemType>(next), i);

    boost::optional<size_t> expectedValue = problem->getExpectedValue();
    if (expectedValue)
    {
        dataSize = expectedValue.get();
        bufs.push_back(boost::asio::buffer(&dataSize, sizeof(dataSize)));
    }

    dataSize = problem->size();
    bufs.push_back(boost::asio::buffer(&dataSize, sizeof(dataSize)));
    bufs.push_back(
        boost::asio::buffer(reinterpret_cast<const char *>(&problem.second),
            dataSize * sizeof(int)));
    answers[i % 4] = problem->getAnswer();

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
