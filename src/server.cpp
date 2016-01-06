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
  virtual ~BaseProblem() {}

public:
  template <class T> void init(bool answer, std::vector<T> &&data, const boost::optional<int>& expectedValue);

  virtual bool getAnswer() const = 0;
  virtual boost::optional<unsigned> getExpectedValue() const = 0;
  virtual size_t size() const = 0;

  template <class T> const std::vector<T> &getData() const;
};

template <class T> class ConcreteProblem : public BaseProblem {
public:
  virtual ~ConcreteProblem() {}

public:
    void init(bool answer, std::vector<T> &&data, const boost::optional<int>& expectedValue)
    {
        mAnswer = answer;
        mData = data;
        mExpected = expectedValue;
    }

    bool getAnswer() const override { return mAnswer; }
    boost::optional<unsigned> getExpectedValue() const override {
      return mExpected;
    }
    size_t size() const override { return mData.size(); }
    const std::vector<T> &getData() const { return mData; }

private:
  bool mAnswer;
  boost::optional<unsigned> mExpected;
  std::vector<T> mData;
};

template <class T> const std::vector<T> &BaseProblem::getData() const {
    return dynamic_cast<const ConcreteProblem<T>&>(*this).getData();
}

template <class T> void BaseProblem::init(bool answer, std::vector<T> &&data,
                                          const boost::optional<int>& expectedValue) {
    return dynamic_cast<const ConcreteProblem<T>&>(*this).init(answer, data, expectedValue);
}

class ProblemContainer {
public:
    ProblemContainer() : mProblems{ ProblemType::NB_ELEMS }, mGlobalSize{ 0 } { }

  template <class T>
  void addProblem(ProblemType type, bool answer, std::vector<T> &&data, const boost::optional<int>& expectedValue) {
    auto prob = std::make_unique<ConcreteProblem<T>>();
    prob->init(answer, std::move(data), expectedValue);
    mProblems[type].push_back(std::move(prob));
    ++mGlobalSize;
  }

  BaseProblem *getProblem(ProblemType type, int index) {
    return mProblems[type][index].get();
  }

  size_t getProblemSize(ProblemType type) { return mProblems[type].size(); }

  size_t getGlobalSize() const { return mGlobalSize; }

private:
  std::vector<std::vector<std::unique_ptr<BaseProblem>>> mProblems;
  size_t mGlobalSize;
};

std::atomic<int> score;
ProblemContainer problems;
boost::array<bool, 4> answers;
std::atomic<int> problemScore;
std::atomic<bool> expired;

std::random_device rd;
std::default_random_engine e1(rd());
std::uniform_int_distribution<int> uniform_dist(0, ProblemType::NB_ELEMS - 1);
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
        std::cout << "I f****** quit !!!" << std::endl
           << "Oh btw, network error, client crashed, connection reset, "
              "armaggeddon, 9/11 or somethin'...."
           << std::endl;
      stop();
      throw std::exception();
      return;
    }

    bool answersAllCorrect = true;

    for (int i = 0; i < 4; ++i) {
      if (mReadMessage[i] != answers[i]) {
        answersAllCorrect = false;
        break;
      }
    }

    if (answersAllCorrect) {
      score += problemScore * 2;
      std::cout << "well alright... +" << problemScore * 2 << std::endl;
    } else {
      score -= problemScore;
      std::cout << "YOU'RE WRONG !!!! -" << problemScore << std::endl;
    }
    
    mDataBuf.clear();

    sendData();
    readData();
  }

  void sendData() {
    int next = uniform_dist(e1);
    mDataBuf.push_back(next);
    problemScore = next < 3 ? 2 : 1;
    std::uniform_int_distribution<int> problemIdxDist(
        0, problems.getProblemSize(static_cast<ProblemType>(next)) - 1);

    std::cout << "ID: " << next << std::endl;

    // Prepare 4 problems to send to a client
    for (int i = 0; i < 4; ++i) {
      auto problem = problems.getProblem(static_cast<ProblemType>(next),
                                         problemIdxDist(e1));

      boost::optional<unsigned> expectedValue = problem->getExpectedValue();
      if (expectedValue)
        mDataBuf.push_back(expectedValue.get());

      mDataBuf.push_back(problem->size());

      if (next < PASSWORD) {
        auto& data = problem->getData<int>();
        mDataBuf.insert(mDataBuf.end(), data.begin(), data.end());
      } else {
        auto& data = problem->getData<char>();
        mDataBuf.insert(mDataBuf.end(), data.begin(), data.end());
      }
      answers[i % 4] = problem->getAnswer();
    }

    // Send the problems to a client
    boost::asio::async_write(
        mSocket, boost::asio::buffer(mDataBuf, mDataBuf.size() * sizeof(unsigned)),
        boost::bind(&TCPConnection::handleWrite, shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));

    // Wait 5 seconds at most for an answer, before sending the next batch of problems
    mTimer.expires_from_now(boost::posix_time::seconds(5));
    mTimer.async_wait(boost::bind(&TCPConnection::onDataTimerExpired,
        shared_from_this(),
        boost::asio::placeholders::error, &mTimer));
  }

  void onDataTimerExpired(const boost::system::error_code &ec,
                          boost::asio::deadline_timer * deadline) {
    if (!ec && (deadline->expires_at() <= boost::asio::deadline_timer::traits_type::now())) {
        std::cout << "Awww.... too slow -" << problemScore << std::endl;
        score -= problemScore;
        mDataBuf.clear();
        sendData();
    }
  }

  void handleWrite(const boost::system::error_code & /*error*/,
                   size_t /*bytes_transferred*/) {
      mDataBuf.clear();
      std::cout << "You have " << score << " points, new problem sent" << std::endl;
  }

private:
  tcp::socket mSocket;
  boost::array<bool, 4> mReadMessage;
  boost::asio::deadline_timer mTimer;
  std::vector<unsigned> mDataBuf;
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
void readProblem(const std::string &name, ProblemType type, ProblemContainer &problems) {
  std::ifstream file(name, std::ios::in | std::ios::binary);

  file.seekg(0, file.end);
  int length = file.tellg();
  file.seekg(0, file.beg);
  T tempDataType;
  int tempInt;
  int size;
  boost::optional<int> expectedValue = boost::none;

  // Read problem set
  while (length > 0) {
    // Read the flag
    unsigned char flag = file.get();
    --length;

    // Read the expected value if this is a RLE problem
    if (type == RLE) {
        file.read((char *)&tempInt, sizeof(tempInt));
        length -= sizeof(tempInt);
        expectedValue = tempInt;
    }

    // Read the 4 problems
    for (int i = 0; i < 4; ++i) {
      // Read single problem size
      file.read((char *)&tempInt, sizeof(tempInt));
      length -= sizeof(tempInt);
      size = tempInt;

      // Read the expected value if this is an array problem
      if (type == ARRAY) {
        file.read((char *)&tempInt, sizeof(tempInt));
        length -= sizeof(tempInt);
        expectedValue = tempInt;
        --size;
      }

      // Read the problem data
      std::vector<T> problemData;
      problemData.reserve(tempInt);
      for (int j = 0; j < size; ++j) {
        file.read((char *)&tempDataType, sizeof(tempDataType));
        length -= sizeof(tempDataType);
        problemData.push_back(tempDataType);
      }
      problems.addProblem(type, flag & (1 << i), std::move(problemData), expectedValue);
    }
  }
}

int main(int argc, char **argv) {
  readProblem<int>(argc > 1 ? argv[1]  : "maze_small.bin",      MAZE,       problems);
  readProblem<int>(argc > 2 ? argv[2]  : "sudoku_small.bin",    SUDOKU,     problems);
  readProblem<int>(argc > 3 ? argv[3]  : "array_small.bin",     ARRAY,      problems);
  readProblem<int>(argc > 5 ? argv[5]  : "tree_small.bin",      TREE,       problems);
  readProblem<char>(argc > 4 ? argv[4] : "password_small.bin",  PASSWORD,   problems);
  readProblem<char>(argc > 6 ? argv[6] : "RLE_small.bin",       RLE,        problems);

  std::cout << problems.getGlobalSize() << " problems loaded" << std::endl;

  try {
    boost::asio::io_service IOService;
    TCPServer Server(IOService);
    if (bool_dist2(e1)) {
      std::cout << base64_decode(not_welcome) << std::endl << std::endl;
      for (;;) {
        for (int t = 0; t < 5; ++t) {
          std::cout << "0xD ";
          std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        std::cout << "\t" << go_away[uniform_dist2(e1)] << "\t";
        for (int t = 0; t < 7; ++t) {
          std::cout << " 0xD";
          std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        std::cout << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
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
      std::cout << "FINAL SCORE " << score << std::endl;

  return 0;
}
