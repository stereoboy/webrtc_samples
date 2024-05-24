
#include <sio_client.h>
#include <asio.hpp>
#include <asio/ssl.hpp>

// #include <api/create_peerconnection_factory.h>
#include <rtc_base/ssl_adapter.h>
// #include <rtc_base/thread.h>
// #include <system_wrappers/include/field_trial.h>


int main(int argc, char* argv[]) {
    asio::ssl::context *ssl_ctx = new asio::ssl::context(asio::ssl::context::tls);

    sio::client sio_client();

    rtc::InitializeSSL();

    rtc::CleanupSSL();
    return 0;
}