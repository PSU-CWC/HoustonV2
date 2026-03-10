// Ensure you have defined ASIO_STANDALONE if your build system doesn't do it automatically
// #define ASIO_STANDALONE 
#include "asio.hpp"
#include <thread>
#include <mutex>
#include <deque>
#include <vector>
#include <string>
#include <iostream>
#include "../Dispatcher.h"
#include "DataProducer.h"


class SerialProducer : public DataProducer {

public:
    SerialProducer() : DataProducer("SerialProducer"), io_context_(), serial_(io_context_),
                       work_guard_(asio::make_work_guard(io_context_)) {

    }

    void setPort(const std::string &port_name) {
        this->portName = port_name;
    }

    void init() override {
    }

    bool start() override {
        if (portName.empty()) {
            std::cerr << "SerialProducer: port name is not set." << std::endl;
            return false;
        }
        std::error_code ec;
        serial_.open(portName, ec);

        if (ec) {
            std::cerr << "SerialProducer: failed to open " << portName << ": " << ec.message() << std::endl;
            return false;
        }

        using namespace asio;
        serial_.set_option(serial_port_base::baud_rate(115200));
        serial_.set_option(serial_port_base::character_size(8));
        serial_.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::one));
        serial_.set_option(serial_port_base::parity(serial_port_base::parity::none));
        serial_.set_option(serial_port_base::flow_control(serial_port_base::flow_control::none));

        start_read();

        background_thread_ = std::thread([this]() {
            io_context_.run();
        });
        status = true;
        return true;
    }

    void stop() override {
        io_context_.stop();
        if (background_thread_.joinable()) {
            background_thread_.join();
        }
        serial_.close();
        status = false;
    }

    ~SerialProducer() override {
        stop();
    }

    void produce(Dispatcher *dispatcher) override {
        std::deque<std::vector<uint8_t>> batch;

        {
            std::lock_guard<std::mutex> lock(rx_mutex_);
            if (rx_queue_.empty()) return;
            batch.swap(rx_queue_);
        }

        for (auto &packet: batch) {
            dispatcher->dispatchData(std::move(packet));
        }
    }

    void send_data(Dispatcher *dispatcher) override {
        for (const auto &data: dispatcher->getTxData()) {
            asio::post(io_context_, [this, data]() {
                bool write_in_progress = !tx_queue_.empty();
                tx_queue_.push_back(data);

                if (!write_in_progress) {
                    start_write();
                }
            });
        }
        dispatcher->getTxData().clear();
    }

private:
    asio::io_context io_context_;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
    asio::serial_port serial_;
    std::thread background_thread_;
    asio::streambuf read_buffer_;

    std::deque<std::vector<uint8_t>> rx_queue_;
    std::mutex rx_mutex_;
    const size_t RX_CAPACITY = 1024;

    std::deque<std::vector<uint8_t>> tx_queue_;
    std::string portName;

    uint8_t raw_rx_buffer[8192]; // Fixed size raw storage
    std::vector<uint8_t> accumulator;

    void start_read() {
        serial_.async_read_some(asio::buffer(raw_rx_buffer, sizeof(raw_rx_buffer)),
            [this](const std::error_code& ec, std::size_t bytes_transferred) {
                if (!ec) {
                    for (size_t i = 0; i < bytes_transferred; ++i) {
                        uint8_t byte = raw_rx_buffer[i];
                        accumulator.push_back(byte);

                        // If we hit the COBS terminator
                        if (byte == 0x00) {
                            if (accumulator.size() > 1) { // Ignore empty/double zeros
                                std::lock_guard<std::mutex> lock(rx_mutex_);
                                if (rx_queue_.size() < RX_CAPACITY) {
                                    rx_queue_.push_back(accumulator);
                                }
                            }
                            accumulator.clear();
                        }
                    }
                    
                    // Safety: If accumulator grows too large without a 0x00, clear it
                    if (accumulator.size() > 8192) accumulator.clear();

                    start_read(); // Keep reading
                } else if (ec != asio::error::operation_aborted) {
                    std::cerr << "Serial Error: " << ec.message() << std::endl;
                }
            });
    }

    void start_write() {
        asio::async_write(serial_,
                          asio::buffer(tx_queue_.front()),
                          [this](const std::error_code &ec, std::size_t /*bytes*/) {
                              if (!ec) {
                                  tx_queue_.pop_front();
                                  if (!tx_queue_.empty()) {
                                      start_write();
                                  }
                              } else {
                                  std::cerr << "SerialProducer: write error: " << ec.message() << std::endl;
                              }
                          });
    }

};