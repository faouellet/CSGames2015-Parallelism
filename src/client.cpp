#include <algorithm>
#include <iostream>
#include <numeric>
#include <random>
#include <thread>
#include <vector>

#include <boost/array.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;
using namespace std;

enum ProblemType {
    MAZE,
    SUDOKU,
    TREE,
    ARRAY,
    PASSWORD,
    RLE,
};

std::random_device rd;
std::default_random_engine e1(rd());
std::bernoulli_distribution uniform_dist(0.5);

template <class T>
using Problems = boost::array<std::vector<T>, 4>;

template <class T>
void getProblems(tcp::socket& socket, unsigned pType, Problems<T>& data, boost::array<unsigned, 4>& expectedValues)
{
    // Clean up
    std::fill(expectedValues.begin(), expectedValues.end(), 0);
    std::for_each(data.begin(), data.end(), [](std::vector<T>& vec) { vec.clear(); });

    unsigned problemSize;
    boost::array<unsigned, 1> buf;

    for (size_t i = 0; i < 4; i++)
    {
        buf[0] = 0;

        // Read an expected value
        if ((pType == ARRAY) || (pType == RLE))
        {
            boost::asio::read(socket, boost::asio::buffer(buf, sizeof(unsigned)));
            expectedValues[i] = buf.front();
            buf[0] = 0;
        }

        // Read a problem size
        boost::asio::read(socket, boost::asio::buffer(buf, sizeof(unsigned)));
        problemSize = buf.front();
        std::vector<unsigned> dataBuf(problemSize);

        // Read a problem data
        // NOTE: Everything is transmitted as an unsigned integer from the server side
        boost::asio::read(socket, boost::asio::buffer(dataBuf, problemSize * sizeof(unsigned)));

        std::copy(dataBuf.begin(), dataBuf.end(), std::back_inserter(data[i]));
    }
}

boost::array<bool, 4> handleMazeProblem(const Problems<unsigned>& mazes)
{
    boost::array<bool, 4> answer_buf;

    // Generate random answer
    for (int i = 0; i < 4; ++i)
        answer_buf[i] = uniform_dist(e1);

    return answer_buf;
}

boost::array<bool, 4> handleSudokuProblem(const Problems<unsigned>& sudokus)
{
    boost::array<bool, 4> answer_buf;

    // Generate random answer
    for (int i = 0; i < 4; ++i)
        answer_buf[i] = uniform_dist(e1);

    return answer_buf;
}

boost::array<bool, 4> handleTreeProblem(const Problems<unsigned>& trees)
{
    boost::array<bool, 4> answer_buf;

    // Generate random answer
    for (int i = 0; i < 4; ++i)
        answer_buf[i] = uniform_dist(e1);

    return answer_buf;
}

boost::array<bool, 4> handleArrayProblem(const Problems<unsigned>& arrays, const boost::array<unsigned, 4>& expectedValues)
{
    boost::array<bool, 4> answer_buf;

    // Generate random answer
    for (int i = 0; i < 4; ++i)
        answer_buf[i] = uniform_dist(e1);

    return answer_buf;
}

boost::array<bool, 4> handlePasswordProblem(const Problems<char>& passwords)
{
    boost::array<bool, 4> answer_buf;

    // Generate random answer
    for (int i = 0; i < 4; ++i)
        answer_buf[i] = uniform_dist(e1);

    return answer_buf;
}

boost::array<bool, 4> handleRLEProblem(const Problems<char>& rles, const boost::array<unsigned, 4>& expectedValues)
{
    boost::array<bool, 4> answer_buf;

    // Generate random answer
    for (int i = 0; i < 4; ++i)
        answer_buf[i] = uniform_dist(e1);

    return answer_buf;
}

int main(int argc, char *argv[]) {
  try {
    if (argc != 2) {
      std::cerr << "Usage: client <host>" << std::endl;
      return 1;
    }

    boost::asio::io_service io_service;

    tcp::resolver resolver(io_service);
    tcp::resolver::query query(argv[1], "22022");
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::socket socket(io_service);

    // Connect Boost TCP Socket
    boost::asio::connect(socket, endpoint_iterator);

    //boost::asio::async_read();

    for (;;) {
      unsigned problemType;
      boost::array<unsigned, 4> expectedValues;
      Problems<unsigned > iProblems;
      Problems<char> sProblems;

      boost::system::error_code error;
      boost::array<unsigned, 1> buf;

      // Read the problem type
      boost::asio::read(socket, boost::asio::buffer(buf, sizeof(unsigned)), error);

      // Check for error
      if (error == boost::asio::error::eof)
          break; // Connection closed cleanly by peer.
      else if (error)
          throw boost::system::system_error(error); // Some other error.

      problemType = buf.front();
      
      if (problemType < PASSWORD)
          getProblems<unsigned>(socket, problemType, iProblems, expectedValues);
      else
          getProblems<char>(socket, problemType, sProblems, expectedValues);

      boost::array<bool, 4> answer_buf;
      switch (problemType)
      {
      case MAZE:
          answer_buf = handleMazeProblem(iProblems);
          break;
      case SUDOKU:
          answer_buf = handleSudokuProblem(iProblems);
          break;
      case TREE:
          answer_buf = handleTreeProblem(iProblems);
          break;
      case ARRAY:
          answer_buf = handleArrayProblem(iProblems, expectedValues);
          break;
      case PASSWORD:
          answer_buf = handlePasswordProblem(sProblems);
          break;
      case RLE:
          answer_buf = handleRLEProblem(sProblems, expectedValues);
          break;
      }
      
      // send it back
      std::cout << "Sending answers" << std::endl;
      socket.send(boost::asio::buffer(answer_buf));
      if (uniform_dist(e1))
        this_thread::sleep_for(chrono::milliseconds(100));
      else
        this_thread::sleep_for(chrono::milliseconds(6000));
      // and here we go again !
    }
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
