#include <iomanip>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include <iostream>

using tcp = boost::asio::ip::tcp;

void Log(boost::system::error_code const& ec) {
    std::cerr << "[" << std::setw(14) << std::this_thread::get_id() << "] "
              << (ec ? "Error: " : "OK")
              << (ec ? ec.message() : "<-_->")
              << std::endl;
}

int main() {
    std::cout << "[ Main Thread : " << std::setw(14) << std::this_thread::get_id() << " ]" << std::endl;

    boost::asio::io_context io_context {};
    size_t const num_threads {4};

    tcp::socket socket {boost::asio::make_strand(io_context)};

    boost::system::error_code ec {};

    tcp::resolver resolver {io_context};
    const auto endpoints {resolver.resolve("google.com", "80", ec)};

    if (ec) {
        Log(ec);
        return -1;
    }

    for (size_t idx = 0; idx < num_threads; ++idx) {
        socket.async_connect(*endpoints.begin(), [](boost::system::error_code const& err_code) {
            Log(err_code);
        });
    }

    // We must call io_context::run for asynchronous callbacks to run.
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        threads.emplace_back([&io_context]() {
            io_context.run();
        });
    }

    // Threads
    for (auto& thread : threads) {
        thread.join();
    }


    return 0;
}