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

enum ProblemType {
  MAZE,
  SUDOKU,
  TREE,
  ARRAY,
  PASSWORD,
  RLE,

  NB_ELEMS
};

class BaseProblem {
public:
  BaseProblem() {}
  virtual ~BaseProblem() {}

public:
  virtual bool getAnswer() const = 0;
  virtual boost::optional<size_t> getExpectedValue() const = 0;
  virtual size_t size() const = 0;

  template <class T> const vector<T> &getData() const {
    return dynamic_cast<const ConcreteProblem<T> &>(*this).getData();
  }
};

template <class T> class ConcreteProblem : BaseProblem {
public:
  ConcreteProblem(bool answer, vector<T> &&data, boost::optional<int> expectedValue)
      : mAnswer{ answer }, mData{ data }, mExpected{ expectedValue } {}
  virtual ~ConcreteProblem() {}

public:
  bool getAnswer() const override { return mAnswer; }
  boost::optional<size_t> getExpectedValue() const override {
    return mExpected;
  }
  size_t size() const override { return mDdata.size(); }
  template <class T> const vector<T> &getData() const { return mData; }

private:
  bool mAnswer;
  boost::optional<size_t> mExpected;
  vector<T> mData;
};

class ProblemContainer {
public:
  ProblemContainer() { mProblems.reserve(ProblemType::NB_ELEMS); }

  template <class T>
  void addProblem(ProblemType type, bool answer, vector<T> &&data) {
    mProblems[type].emplace_back(answer, data);
    ++mGlobalSize;
  }

  BaseProblem *getProblem(ProblemType type, int index) {
    return mProblems[type][index].get();
  }

  size_t getProblemSize(ProblemType type) { return mProblems[type].size(); }

  size_t getGlobalSize() const { return mGlobalSize; }

private:
  vector<vector<unique_ptr<BaseProblem>>> mProblems;
  size_t mGlobalSize;
};

atomic<int> score;
ProblemContainer problems;
boost::array<bool, 4> answers;
atomic<int> problemScore;
atomic<bool> expired;

std::random_device rd;
std::default_random_engine e1(rd());
std::uniform_int_distribution<int> uniform_dist(0, ProblemType::NB_ELEMS);
std::uniform_int_distribution<int> uniform_dist2(0, 13);
std::bernoulli_distribution bool_dist1(0.2);
std::bernoulli_distribution bool_dist2(0.01);

class TCPConnection : public boost::enable_shared_from_this<TCPConnection> {
public:
  typedef boost::shared_ptr<TCPConnection> pointer;

  static pointer create(boost::asio::io_service &IOService) {
    return pointer(new TCPConnection(IOService));
  }

  tcp::socket &socket() { return mSocket; }

  void start() {
    sendData();
    readData();
  }

private:
  TCPConnection(boost::asio::io_service &IOService)
      : mSocket(IOService), mTimer(IOService) {}

  void stop() {
    mSocket.close();
    mTimer.cancel();
  }

  void readData() {
    boost::asio::async_read(mSocket, boost::asio::buffer(mReadMessage),
                            boost::bind(&TCPConnection::onDataReceived,
                                        shared_from_this(),
                                        boost::asio::placeholders::error));
  }

  void onDataReceived(const boost::system::error_code &ec) {
    if (ec) {
      cout << "I f****** quit !!!" << endl
           << "Oh btw, network error, client crashed, connection reset, "
              "armagueddon, 9/11 or somethin'...."
           << endl;
      stop();
      // throw exception("bye bye !");
      throw exception();
      return;
    }

    mTimer.expires_from_now(boost::posix_time::seconds(5));
    mTimer.async_wait(boost::bind(&TCPConnection::onDataTimerExpired,
                                  shared_from_this(),
                                  boost::asio::placeholders::error, &mTimer));
    for (int i = 0; i < 4; ++i) {
      if (mReadMessage[i] != answers[i]) {
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
    size_t dataSize;
    vector<boost::asio::const_buffer> bufs;

    int next = uniform_dist(e1);
    bufs.push_back(boost::asio::buffer(&next, sizeof(next)));
    problemScore = next < 3 ? 2 : 1;
    std::uniform_int_distribution<int> problemIdxDist(
        0, problems.getProblemSize(static_cast<ProblemType>(next)));

    for (int i = 0; i < 4; ++i) {
      auto problem = problems.getProblem(static_cast<ProblemType>(next),
                                         problemIdxDist(e1));

      boost::optional<size_t> expectedValue = problem->getExpectedValue();
      if (expectedValue) {
        dataSize = expectedValue.get();
        bufs.push_back(boost::asio::buffer(&dataSize, sizeof(dataSize)));
      }

      dataSize = problem->size();
      bufs.push_back(boost::asio::buffer(&dataSize, sizeof(dataSize)));

      if (next < 3) {
        bufs.push_back(boost::asio::buffer(
            reinterpret_cast<const char *>(&problem->getData<int>()),
            dataSize * sizeof(int)));
      } else {
        bufs.push_back(boost::asio::buffer(
            reinterpret_cast<const char *>(&problem->getData<char>()),
            dataSize * sizeof(char)));
      }
      answers[i % 4] = problem->getAnswer();

      boost::asio::async_write(
          mSocket, bufs,
          boost::bind(&TCPConnection::handleWrite, shared_from_this(),
                      boost::asio::placeholders::error,
                      boost::asio::placeholders::bytes_transferred));

      mTimer.expires_from_now(boost::posix_time::seconds(5));
      mTimer.async_wait(boost::bind(&TCPConnection::onDataTimerExpired,
                                    shared_from_this(),
                                    boost::asio::placeholders::error, &mTimer));
    }
  }

  void onDataTimerExpired(const boost::system::error_code &ec,
                          boost::asio::deadline_timer *) {
    if (ec == boost::asio::error::operation_aborted) {
      cout << "Abort" << endl;
      return;
    }

    cout << "Awww.... too slow -" << problemScore << endl;
    score -= problemScore;
    sendData();
  }

  void handleWrite(const boost::system::error_code & /*error*/,
                   size_t /*bytes_transferred*/) {
    cout << "You have " << score << " points, sending new problem" << endl;
  }

private:
  tcp::socket mSocket;
  boost::array<bool, 4> mReadMessage;
  boost::asio::deadline_timer mTimer;
};

class TCPServer {
public:
  TCPServer(boost::asio::io_service &IOService)
      : mAcceptor(IOService, tcp::endpoint(tcp::v4(), 22022)) {
    startAccept();
  }

private:
  void startAccept() {
    TCPConnection::pointer NewConnection =
        TCPConnection::create(mAcceptor.get_io_service());

    mAcceptor.async_accept(NewConnection->socket(),
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

private:
  tcp::acceptor mAcceptor;
};

template <class T>
void readProblem(const string &name, ProblemContainer &problems) {
  ifstream file(name, ios::in | ios::binary);

  file.seekg(0, file.end);
  int length = file.tellg();
  file.seekg(0, file.beg);
  T tempDataType;
  int tempInt;
  boost::optional<int> expectedValue = boost::none;

  // Read problem set
  while (length > 0) {
    // Read the flag
    unsigned char flag = file.get();
    --length;
    // Read the 4 problems
    for (int i = 0; i < 4; ++i) {
      // Read single problem size
      file.read((char *)&tempInt, sizeof(tempInt));
      length -= sizeof(tempInt);

      // Read the expected value if the problem uses one
      if (flag > 3) {
        file.read((char *)&tempInt, sizeof(tempInt));
        length -= sizeof(tempInt);
        expectedValue = tempInt;
      }

      // Read the problem data
      vector<T> problemData;
      maze.reserve(tempInt);
      for (int j = 0; j < size; ++j) {
        file.read((char *)&tempDataType, sizeof(tempDataType));
        length -= sizeof(tempDataType);
        problemData.push_back(tempDataType);
      }
      problems.addProblem(flag, flag & (1 << i), problemData, expectedValue);
    }
  }
}

int main(int argc, char **argv) {
  cout << argv[1] << endl;
  readProblem<int>(argc > 1 ? argv[1] : "maze_small.bin", problems);
  readProblem<int>(argc > 2 ? argv[2] : "sudoku_small.bin", problems);
  readProblem<int>(argc > 3 ? argv[3] : "array_small.bin", problems);
  readProblem<char>(argc > 4 ? argv[4] : "password_small.bin", problems);
  readProblem<int>(argc > 5 ? argv[5] : "tree_small.bin", problems);
  readProblem<char>(argc > 6 ? argv[6] : "RLE_small.bin", problems);

  cout << problems.getGlobalSize() << " problems loaded" << endl;

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
