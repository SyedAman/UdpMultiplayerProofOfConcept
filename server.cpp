#include <boost/asio.hpp>
#include <iostream>
#include <unordered_map>
#include <string>

using boost::asio::ip::udp;

class Server {
public:
    Server(boost::asio::io_context& io_context) : socket_(io_context, udp::endpoint(udp::v4(), 12345)) {
        start_receive();
    }

private:
    udp::socket socket_;
    udp::endpoint remote_endpoint_;
    std::array<char, 1024> recv_buffer_;
    std::unordered_map<std::string, int> client_health_;

    void start_receive() {
        socket_.async_receive_from(
            boost::asio::buffer(recv_buffer_), remote_endpoint_,
            [this](boost::system::error_code ec, std::size_t /*bytes_transferred*/) {
               if (!ec) {
                   process_request();
               }
                start_receive(); // wait for the next packet
            });
    }

    void process_request() {
        // Simulating damage taken
        std::string client_id = remote_endpoint_.address().to_string() + ":" + std::to_string(remote_endpoint_.port());
        int& health = client_health_[client_id];
        health = std::max(0, health - 1); // Damage taken, reduce health by 1

        std::string message = "Client " + client_id + " Health: " + std::to_string(health);
        std::cout << message << std::endl;

        // Broadcast updated health to all clients
        for (const auto& pair : client_health_) {
            udp::resolver resolver(socket_.get_executor());
            auto endpoints = resolver.resolve(udp::v4(), pair.first.substr(0, pair.first.find(':')), pair.first.substr(pair.first.find(':') + 1));
            for (auto& endpoint : endpoints) {
                socket_.async_send_to(boost::asio::buffer(message), endpoint,
                    [](boost::system::error_code /*ec*/, std::size_t /*bytes_transferred*/) {});
            }
        }
    }
};

int main() {
    try {
        boost::asio::io_context io_context;
        Server server(io_context);
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
