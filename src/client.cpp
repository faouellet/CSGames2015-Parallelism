#include <chrono>
#include <iostream>
#include <random>
#include <thread>
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

boost::array<bool, 4> handleMazeProblem(tcp::socket& socket)
{
    boost::array<bool, 4> answer_buf;

    // Generate random answer
    for (int i = 0; i < 4; ++i)
        answer_buf[i] = uniform_dist(e1);

    return answer_buf;
}

boost::array<bool, 4> handleSudokuProblem(tcp::socket& socket)
{
    boost::array<bool, 4> answer_buf;

    // Generate random answer
    for (int i = 0; i < 4; ++i)
        answer_buf[i] = uniform_dist(e1);

    return answer_buf;
}

boost::array<bool, 4> handleTreeProblem(tcp::socket& socket)
{
    boost::array<bool, 4> answer_buf;

    // Generate random answer
    for (int i = 0; i < 4; ++i)
        answer_buf[i] = uniform_dist(e1);

    return answer_buf;
}

boost::array<bool, 4> handleArrayProblem(tcp::socket& socket)
{
    boost::array<bool, 4> answer_buf;

    // Generate random answer
    for (int i = 0; i < 4; ++i)
        answer_buf[i] = uniform_dist(e1);

    return answer_buf;
}

boost::array<bool, 4> handlePasswordProblem(tcp::socket& socket)
{
    boost::array<bool, 4> answer_buf;

    // Generate random answer
    for (int i = 0; i < 4; ++i)
        answer_buf[i] = uniform_dist(e1);

    return answer_buf;
}

boost::array<bool, 4> handleRLEProblem(tcp::socket& socket)
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
      // Single integer (32 bits) buffer to read a single byte from the server
      boost::array<int, 1> buf;
      boost::system::error_code error;

      // Read the problem type
      size_t problemType = socket.read_some(boost::asio::buffer(buf, 4), error);
      
      // Check for error
      if (error == boost::asio::error::eof)
          break; // Connection closed cleanly by peer.
      else if (error)
          throw boost::system::system_error(error); // Some other error.

      std::cout << "Received: " << problemType << std::endl;

      boost::array<bool, 4> answer_buf;
      switch (problemType)
      {
      case MAZE:
          answer_buf = handleMazeProblem(socket);
          break;
      case SUDOKU:
          answer_buf = handleSudokuProblem(socket);
          break;
      case TREE:
          answer_buf = handleTreeProblem(socket);
          break;
      case ARRAY:
          answer_buf = handleArrayProblem(socket);
          break;
      case PASSWORD:
          answer_buf = handlePasswordProblem(socket);
          break;
      case RLE:
          answer_buf = handleRLEProblem(socket);
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
