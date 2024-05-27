#ifndef __PEER_CLIENT_HPP__
#define __PEER_CLIENT_HPP__

#include <mutex>
#include <condition_variable>

#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <sio_client.h>


class PeerClient {

    std::string                     _name;
    std::mutex                      _msg_mutex;
    std::condition_variable_any     _msg_cond;
    std::atomic_bool                _connected = {false};
    sio::client                     _sio_client;

    std::shared_ptr<spdlog::logger> _logger = nullptr;

public:
    PeerClient(std::string name) {

        _logger = spdlog::stdout_color_mt(fmt::format("{}", name));
        this->_name = name;
    }

    ~PeerClient() {

    }

    void connect_sync(const std::string& hostname, const int port) {
        _sio_client.connect(fmt::format("https://{}:{}", hostname, port));
        std::unique_lock<std::mutex> lock(_msg_mutex);
        _msg_cond.wait(_msg_mutex);
    }

    void init() {
        _logger->info("Initializing {}", _name);
        _sio_client.set_reconnect_attempts(0);
        _sio_client.set_open_listener(std::bind(&PeerClient::on_connected, this));
        _sio_client.set_close_listener(std::bind(&PeerClient::on_close, this,std::placeholders::_1));
        _sio_client.set_fail_listener(std::bind(&PeerClient::on_fail, this));
    }

    void deinit() {
        _msg_cond.wait(_msg_mutex);
        _sio_client.sync_close();
        _sio_client.clear_con_listeners();
    }

    void on_connected() {
        // std::unique_lock<std::mutex> lock(_msg_mutex);
        _logger->info("Connected to Signaling Server");
        _msg_cond.notify_all();
    }

    void on_close(sio::client::close_reason const& reason) {
        std::unique_lock<std::mutex> lock(_msg_mutex);
        _logger->info("Connection closed: {}", reason);
    }

    void on_fail() {
        std::unique_lock<std::mutex> lock(_msg_mutex);
        _logger->error("Connection failed: {}");
        _msg_cond.notify_all();
    }
};

#endif // __PEER_CLIENT_HPP__