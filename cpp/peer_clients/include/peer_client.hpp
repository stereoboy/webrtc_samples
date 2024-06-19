#ifndef __PEER_CLIENT_HPP__
#define __PEER_CLIENT_HPP__

#include <mutex>
#include <condition_variable>

// #include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <sio_client.h>



class PeerClient {

public:
    enum class PeerType {
        Undefined = 0,
        Caller,
        Callee,
    };

protected:
    std::string                     name_;
    std::mutex                      msg_mutex_;
    std::condition_variable_any     msg_cond_;
    std::atomic_bool                connected_ = {false};

    sio::client                     sio_client_;

    PeerType                        type_ = PeerType::Undefined;

    std::shared_ptr<spdlog::logger> logger_ = nullptr;

public:
    PeerClient(std::string name) {

        logger_ = spdlog::stdout_color_mt(fmt::format("{}", name));
        this->name_ = name;
    }

    virtual ~PeerClient() {

    }

    PeerType getType() {
        return type_;
    }

    void connect_sync(const std::string& hostname, const int port) {
        sio_client_.connect(fmt::format("https://{}:{}", hostname, port));
        std::unique_lock<std::mutex> lock(msg_mutex_);
        msg_cond_.wait(msg_mutex_);
    }

    virtual void init_signaling() {
        logger_->info("Initializing {}", name_);
        sio_client_.set_reconnect_attempts(0);
        sio_client_.set_open_listener(std::bind(&PeerClient::on_connected, this));
        sio_client_.set_close_listener(std::bind(&PeerClient::on_close, this,std::placeholders::_1));
        sio_client_.set_fail_listener(std::bind(&PeerClient::on_fail, this));
    }

    virtual void deinit_signaling() {
        // msg_cond_.wait(msg_mutex_);
        sio_client_.sync_close();
        sio_client_.clear_con_listeners();
    }

    virtual void on_connected() {
        // std::unique_lock<std::mutex> lock(msg_mutex_);
        logger_->info("Connected to Signaling Server");
        connected_ = true;
        msg_cond_.notify_all();
    }

    virtual void on_close(sio::client::close_reason const& reason) {
        std::unique_lock<std::mutex> lock(msg_mutex_);
        logger_->info("Connection closed: {}", reason);
        connected_ = false;
    }

    virtual void on_fail() {
        std::unique_lock<std::mutex> lock(msg_mutex_);
        logger_->error("Connection failed: {}");
        msg_cond_.notify_all();
    }

    virtual bool is_connected() {
        return connected_;
    }
};

#endif // __PEER_CLIENT_HPP__