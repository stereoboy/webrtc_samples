#include <csignal>
#include <thread>

#include <sio_client.h>
// #include <asio.hpp>
// #include <asio/ssl.hpp>

// #include <api/create_peerconnection_factory.h>
#include <rtc_base/ssl_adapter.h>
// #include <rtc_base/thread.h>
// #include <system_wrappers/include/field_trial.h>

#include "peer_client.hpp"


class PeerDataChannelClient : public PeerClient {

public:
    PeerDataChannelClient(): PeerClient("DataChannelClient") {

    }
};

int main(int argc, char* argv[]) {
    auto logger = spdlog::stdout_color_mt("PeerDataChannel");
    // asio::ssl::context *ssl_ctx = new asio::ssl::context(asio::ssl::context::tls);

    // sio::client sio_client();

    PeerDataChannelClient client;

    client.init();

    client.connect_sync("localhost", 5000);

    rtc::InitializeSSL();

    try {
        while(true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } catch (std::exception &e) {
        logger->error("Terminated by Interrupt: {} ", e.what());
    }

    rtc::CleanupSSL();
    return 0;
}