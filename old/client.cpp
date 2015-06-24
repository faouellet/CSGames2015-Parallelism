#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <boost/array.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;
using namespace std;

int main(int argc, char *argv[]) {
  try {
    if (argc != 2) {
      std::cerr << "Usage: client <host>" << std::endl;
      return 1;
    }

    std::random_device rd;
    std::default_random_engine e1(rd());
    std::bernoulli_distribution uniform_dist(0.5);

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

      // Read the data
      size_t len = socket.read_some(boost::asio::buffer(buf, 4), error);
      std::cout << "Received: " << len << std::endl;

      // Check for error
      if (error == boost::asio::error::eof) {
        break; // Connection closed cleanly by peer.
      } else if (error)
        throw boost::system::system_error(error); // Some other error.

      // Show the data
      std::cout << buf[0] << std::endl;

      // Generate random answer
      boost::array<bool, 4> send_buf;
      for (int i = 0; i < 4; ++i)
        send_buf[i] = uniform_dist(e1);
      // send it back
      socket.send(boost::asio::buffer(send_buf));
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
