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

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>


ABSL_FLAG(std::string, server, "localhost", "The server to connect to.");
ABSL_FLAG(int,
          port,
          5000,
          "The port on which the server is listening.");

class PeerDataChannelClient : public PeerClient,
                              public webrtc::PeerConnectionObserver {

public:
    PeerDataChannelClient(): PeerClient("DataChannelClient") {

    }
};

int main(int argc, char* argv[]) {
    auto logger = spdlog::stdout_color_mt("PeerDataChannel");
    absl::SetProgramUsageMessage(
      "Example usage: ./peer_datachanenl_client --server=localhost --port=5000\n");
    absl::ParseCommandLine(argc, argv);

    logger->info("Starting PeerDataChannelClient");
    // asio::ssl::context *ssl_ctx = new asio::ssl::context(asio::ssl::context::tls);

    // sio::client sio_client();

    PeerDataChannelClient client;

    client.init_signaling();

    logger->info("Connecting to {}:{}", absl::GetFlag(FLAGS_server), absl::GetFlag(FLAGS_port));
    client.connect_sync(absl::GetFlag(FLAGS_server), absl::GetFlag(FLAGS_port));

    rtc::InitializeSSL();

    try {
        while(true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } catch (std::exception &e) {
        logger->error("Terminated by Interrupt: {} ", e.what());
    }

    rtc::CleanupSSL();
    client.deinit_signaling();
    return 0;
}