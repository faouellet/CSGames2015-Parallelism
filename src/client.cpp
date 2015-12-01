#include <algorithm>
#include <iostream>
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
void getProblems(tcp::socket& socket, Problems<T>& data, boost::array<int, 4>& expectedValues)
{
    // Clean up
    std::fill(expectedValues.begin(), expectedValues.end(), 0);
    data.clear();

    size_t problemSize;
    for (size_t i = 0; i < 4; i++)
    {
        // Read a problem size
        socket.read_some(boost::asio::buffer(buf, 4), error);
        problemSize = buf.front();

        // Read an expected value
        if ((problemType == ARRAY) || (problemType == RLE))
        {
            socket.read_some(boost::asio::buffer(buf, 4), error);
            expectedValues[i] = buf.front();
        }

        // Read a problem data
        socket.read_some(boost::asio::buffer(buf, 4), error);
        data.reserve(problemSize);
        socket.read_some(data, error);
    }
}

boost::array<bool, 4> handleMazeProblem(const Problems<int>& mazes)
{
    boost::array<bool, 4> answer_buf;

    // Generate random answer
    for (int i = 0; i < 4; ++i)
        answer_buf[i] = uniform_dist(e1);

    return answer_buf;
}

boost::array<bool, 4> handleSudokuProblem(const Problems<int>& sudokus)
{
    boost::array<bool, 4> answer_buf;

    // Generate random answer
    for (int i = 0; i < 4; ++i)
        answer_buf[i] = uniform_dist(e1);

    return answer_buf;
}

boost::array<bool, 4> handleTreeProblem(const Problems<int>& trees)
{
    boost::array<bool, 4> answer_buf;

    // Generate random answer
    for (int i = 0; i < 4; ++i)
        answer_buf[i] = uniform_dist(e1);

    return answer_buf;
}

boost::array<bool, 4> handleArrayProblem(const Problems<int>& arrays, const boost::array<int, 4>& expectedValues)
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

boost::array<bool, 4> handleRLEProblem(const Problems<char>& rles, const boost::array<int, 4>& expectedValues)
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

    for (;;) {
      ProblemType problemType;
      boost::array<int, 4> expectedValues;
      Problems<int> iProblems;
      Problems<char> sProblems;

      boost::system::error_code error;
      boost::array<int, 1> buf;

      // Read the problem type
      socket.read_some(boost::asio::buffer(buf, 4), error);
      size_t problemType = buf.front();

      // Check for error
      if (error == boost::asio::error::eof)
          break; // Connection closed cleanly by peer.
      else if (error)
          throw boost::system::system_error(error); // Some other error.

      if (problemType < PASSWORD)
          getProblems<int>(socket, iProblems, expectedValues);
      else
          getProblems<char>(socket, sProblems, expectedValues);

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
