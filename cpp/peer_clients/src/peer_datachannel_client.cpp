#include <csignal>
#include <thread>

#include <sio_client.h>
// #include <asio.hpp>
// #include <asio/ssl.hpp>

#include <api/peer_connection_interface.h>
#include <api/create_peerconnection_factory.h>
#include <rtc_base/ssl_adapter.h>
// #include <rtc_base/thread.h>
// #include <system_wrappers/include/field_trial.h>

#include "peer_client.hpp"

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>

struct ice {
    std::string candidate;
    std::string sdp_mid;
    int sdp_mline_index;
};

ABSL_FLAG(std::string, server, "localhost", "The server to connect to.");
ABSL_FLAG(int,
          port,
          5000,
          "The port on which the server is listening.");


class DummySetSessionDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
public:
    static rtc::scoped_refptr<DummySetSessionDescriptionObserver> Create() {
        return rtc::make_ref_counted<DummySetSessionDescriptionObserver>();
    }
    virtual void OnSuccess() {
        RTC_LOG(LS_INFO) << __FUNCTION__;
        spdlog::info("{}", __PRETTY_FUNCTION__);
    }
    virtual void OnFailure(webrtc::RTCError error) {
        RTC_LOG(LS_INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": " << error.message();
        spdlog::info("{}", __PRETTY_FUNCTION__);
    }
};

class PeerDataChannelClient : public PeerClient,
                              public webrtc::PeerConnectionObserver,
                              public webrtc::DataChannelObserver,
                              public webrtc::CreateSessionDescriptionObserver
                            {

    rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_ = nullptr;
    rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel_;

    std::list<struct ice> ice_list_;
    std::unique_ptr<rtc::Thread> network_thread_;
    std::unique_ptr<rtc::Thread> worker_thread_;
    std::unique_ptr<rtc::Thread> signaling_thread_;
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory_;
    webrtc::PeerConnectionInterface::RTCConfiguration configuration_;

    // std::unique_ptr<rtc::Thread> signaling_thread_;
public:
    PeerDataChannelClient(): PeerClient("DataChannelClient") {

    }

    virtual ~PeerDataChannelClient() {
        if (data_channel_) {
            data_channel_ = nullptr;
        }

        if (peer_connection_) {
            // peer_connection_->Close();
            peer_connection_ = nullptr;
        }

        if (peer_connection_factory_) {
            peer_connection_factory_ = nullptr;
        }

        if (network_thread_) {
            network_thread_->Stop();
            network_thread_ = nullptr;
        }

        if (worker_thread_) {
            worker_thread_->Stop();
            worker_thread_ = nullptr;
        }

        if (signaling_thread_) {
            signaling_thread_->Stop();
            signaling_thread_ = nullptr;
        }
        logger_->info("PeerDataChannelClient deleted");
    }
    //
    // PeerConnectionObserver implementation.
    //
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override {
        logger_->info("PeerConnectionInterface::OnSignalingChange: {}", new_state);
    }
    // void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
    //                 const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
    //                 streams) override {}
    // void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override {}

    void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {
        logger_->info("PeerConnectionInterface::OnAddStream: {}");
    };

    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {
        logger_->info("PeerConnectionInterface::OnDataChannel: {}", channel->label());
    }
    void OnRenegotiationNeeded() override {
        logger_->info("PeerConnectionInterface::OnRenegotiationNeeded");
    }
    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override {
        logger_->info("PeerConnectionInterface::OnIceConnectionChange: {}", new_state);
    }
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override {
        logger_->info("PeerConnectionInterface::OnIceGatheringChange: {}", new_state);
    }

    void OnIceCandidate(const webrtc::IceCandidateInterface *candidate) override {

        logger_->info("OnIceCandidate: {}", candidate->sdp_mline_index());
        std::string out;
        candidate->ToString(&out);
        ice_list_.push_back({out, candidate->sdp_mid(), candidate->sdp_mline_index()});
        logger_->info("\t{}", out);
    };

    //
    // CreateSessionDescriptionObserver implementation
    //
    void OnSuccess(webrtc::SessionDescriptionInterface *desc) override {
        logger_->info("CreateSessionDescriptionObserver::OnSuccess({})");
        peer_connection_->SetLocalDescription(DummySetSessionDescriptionObserver::Create().get(), desc);

        std::string sdp_str;
        desc->ToString(&sdp_str);
        logger_->info("SDP:\n{}", sdp_str);

        sio::message::ptr msg = sio::string_message::create(sdp_str);

        if (type_ == PeerClient::PeerType::Caller) {
            sio_client_.socket()->emit("offer", msg, [&](sio::message::list const& msg) {
                std::unique_lock<std::mutex> lock(msg_mutex_);
                bool ok = msg[0]->get_map()["ok"]->get_bool();
                std::string message = msg[0]->get_map()["message"]->get_string();
            if (ok) {
                    logger_->info("Offer SDP sent successfully");
                } else {
                    logger_->error("Offer SDP failed to send: {}", message);
                }
            });
        } else {
            sio_client_.socket()->emit("answer", msg, [&](sio::message::list const& msg) {
                std::unique_lock<std::mutex> lock(msg_mutex_);
                bool ok = msg[0]->get_map()["ok"]->get_bool();
                std::string message = msg[0]->get_map()["message"]->get_string();
                if (ok) {
                    logger_->info("Answer SDP sent successfully");
                } else {
                    logger_->error("Answer SDP failed to send: {}", message);
                }
            });
        }
    };

    void OnFailure(webrtc::RTCError error) override {
        logger_->info("CreateSessionDescriptionObserver::OnFailure({})", error.message());
    };

    //
    // DataChannelObserver implementation
    //
    void OnStateChange() override {
        logger_->info("DataChannelObserver::StateChange");
    };

    void OnMessage(const webrtc::DataBuffer &buffer) override {
        logger_->info("DataChannelObserver::Message: {}", buffer.data.data());
    };

    void OnBufferedAmountChange(uint64_t previous_amount) override {
        logger_->info("DataChannelObserver::BufferedAmountChange: {}", previous_amount);
    };

    //
    //
    //
    virtual void init_signaling(void) override {
        PeerClient::init_signaling();

        sio_client_.socket()->on("set-as-caller", sio::socket::event_listener_aux([&](std::string const& name, sio::message::ptr const& data, bool isAck, sio::message::list &ack_resp)
            {
                std::unique_lock<std::mutex> lock(msg_mutex_);
                logger_->info("event={}", name);
                type_ = PeerClient::PeerType::Caller;
                sio::message::ptr resp = sio::object_message::create();
                resp->get_map()["ok"] = sio::bool_message::create(true);
                resp->get_map()["message"] = sio::string_message::create("accepted");
                ack_resp.push(resp);
            }
        ));

        sio_client_.socket()->on("set-as-callee", sio::socket::event_listener_aux([&](std::string const& name, sio::message::ptr const& data, bool isAck, sio::message::list &ack_resp)
            {
                std::unique_lock<std::mutex> lock(msg_mutex_);
                logger_->info("event={}", name);
                type_ = PeerClient::PeerType::Callee;
                sio::message::ptr resp = sio::object_message::create();
                resp->get_map()["ok"] = sio::bool_message::create(true);
                resp->get_map()["message"] = sio::string_message::create("accepted");
                ack_resp.push(resp);
            }
        ));

        sio_client_.socket()->on("ready", sio::socket::event_listener_aux([&](std::string const& name, sio::message::ptr const& data, bool isAck, sio::message::list &ack_resp)
            {
                std::unique_lock<std::mutex> lock(msg_mutex_);
                logger_->info("event={}", name);
                if (type_ == PeerClient::PeerType::Caller) {
                    if (!create_offer()) {
                        logger_->error("Failed to create Offer");
                        sio::message::ptr resp = sio::object_message::create();
                        resp->get_map()["ok"] = sio::bool_message::create(false);
                        resp->get_map()["message"] = sio::string_message::create("Failed to create Offer");
                        ack_resp.push(resp);
                        return;
                    }
                }
                sio::message::ptr resp = sio::object_message::create();
                resp->get_map()["ok"] = sio::bool_message::create(true);
                resp->get_map()["message"] = sio::string_message::create("accepted");
                ack_resp.push(resp);
            }
        ));

        sio_client_.socket()->on("offer", sio::socket::event_listener_aux([&](std::string const& name, sio::message::ptr const& data, bool isAck, sio::message::list &ack_resp)
            {
                std::unique_lock<std::mutex> lock(msg_mutex_);
                logger_->info("event={}", name);
                if (type_ == PeerClient::PeerType::Callee) {
                    std::string sdp_str = data->get_string();
                    logger_->info("received Offer SDP:\n{}", sdp_str);
                    if (!receive_offer_create_answer(sdp_str)){
                        logger_->error("Failed to receive/create Answer");
                        sio::message::ptr resp = sio::object_message::create();
                        resp->get_map()["ok"] = sio::bool_message::create(false);
                        resp->get_map()["message"] = sio::string_message::create("Failed to receive/create Answer");
                        ack_resp.push(resp);
                        return;
                    }
                } else {
                    logger_->error("Unexpected Offer");
                }
                sio::message::ptr resp = sio::object_message::create();
                resp->get_map()["ok"] = sio::bool_message::create(true);
                resp->get_map()["message"] = sio::string_message::create("accepted");
                ack_resp.push(resp);
            }
        ));

        sio_client_.socket()->on("answer", sio::socket::event_listener_aux([&](std::string const& name, sio::message::ptr const& data, bool isAck, sio::message::list &ack_resp)
            {
                std::unique_lock<std::mutex> lock(msg_mutex_);
                logger_->info("event={}", name);
                if (type_ == PeerClient::PeerType::Caller) {
                    std::string sdp_str = data->get_string();
                    logger_->info("received Answer SDP:\n{}", sdp_str);
                    if (!receive_answer(sdp_str)) {
                        logger_->error("Failed to receive Answer");
                        sio::message::ptr resp = sio::object_message::create();
                        resp->get_map()["ok"] = sio::bool_message::create(false);
                        resp->get_map()["message"] = sio::string_message::create("Failed to receive Answer");
                        ack_resp.push(resp);
                        return;
                    }
                } else {
                    logger_->error("Unexpected Answer");
                }
                sio::message::ptr resp = sio::object_message::create();
                resp->get_map()["ok"] = sio::bool_message::create(true);
                resp->get_map()["message"] = sio::string_message::create("accepted");
                ack_resp.push(resp);
            }
        ));
    }

    void query_peer_type(void) {
        logger_->info(">>> query-peer-type");
        std::string msg = "dummy";
        sio_client_.socket()->emit("query-peer-type", msg, [&](sio::message::list const& msg) {
            // std::unique_lock<std::mutex> lock(msg_mutex_);
            std::string res = msg[0]->get_string();
            if (res.compare("caller") == 0) {
                type_ = PeerClient::PeerType::Caller;
                logger_->info("caller");
            } else if (res.compare("callee") == 0) {
                type_ = PeerClient::PeerType::Callee;
                logger_->info("callee");
            }
            msg_cond_.notify_all();
        });

        std::unique_lock<std::mutex> lock(msg_mutex_);
        msg_cond_.wait(msg_mutex_);
        logger_->info("<<< query-peer-type");
    }

    bool init_webrtc(void) {
        webrtc::PeerConnectionInterface::IceServer ice_server;
        ice_server.uri = "stun:stun.l.google.com:19302";
        configuration_.servers.push_back(ice_server);

#if 1
        network_thread_ = rtc::Thread::CreateWithSocketServer();
        network_thread_->Start();
        worker_thread_ = rtc::Thread::Create();
        worker_thread_->Start();
        signaling_thread_ = rtc::Thread::Create();
        signaling_thread_->Start();
        webrtc::PeerConnectionFactoryDependencies dependencies;
        dependencies.network_thread   = network_thread_.get();
        dependencies.worker_thread    = worker_thread_.get();
        dependencies.signaling_thread = signaling_thread_.get();
        peer_connection_factory_       = webrtc::CreateModularPeerConnectionFactory(std::move(dependencies));
#else
        if (!signaling_thread_.get()) {
            signaling_thread_ = rtc::Thread::CreateWithSocketServer();
            signaling_thread_->Start();
        }
        peer_connection_factory_ = webrtc::CreatePeerConnectionFactory(
            nullptr /* network_thread */, nullptr /* worker_thread */,
            signaling_thread_.get(), nullptr /* default_adm */,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr /* audio_mixer */, nullptr /* audio_processing */);
#endif

        webrtc::PeerConnectionDependencies pc_dependencies(this);
        auto error_or_peer_connection = peer_connection_factory_->CreatePeerConnectionOrError(configuration_, std::move(pc_dependencies));
        peer_connection_factory_->SetOptions(webrtc::PeerConnectionFactoryInterface::Options());
        if (error_or_peer_connection.ok()) {
            peer_connection_ = std::move(error_or_peer_connection.value());;
            logger_->info("peer_connection_ created successfully.");
        } else {
            logger_->error("Failed to create PeerConnection: {}", error_or_peer_connection.error().message());
            return false;
        }

        webrtc::DataChannelInit config;
        auto error_or_data_channel = peer_connection_->CreateDataChannelOrError("data_channel", &config);
        if (error_or_data_channel.ok()) {
            data_channel_ = std::move(error_or_data_channel.value());
            logger_->info("data_channel_ created successfully");
        } else {
            logger_->error("Failed to create DataChannel: {}", error_or_data_channel.error().message());
            return false;
        }
        data_channel_->RegisterObserver(this);

        logger_->info("current");
        return true;
    }

    bool create_offer(void) {
        peer_connection_->CreateOffer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
        return true;
    }

    bool receive_answer(const std::string &sdp_str) {
        webrtc::SdpParseError error;
            std::unique_ptr<webrtc::SessionDescriptionInterface> session_description = webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, sdp_str, &error);
        if (session_description == nullptr) {
            logger_->error("Failed to create session description: {}", error.description);
            return false;
        }
        peer_connection_->SetRemoteDescription(DummySetSessionDescriptionObserver::Create().get(), session_description.release());
        return true;
    }

    bool receive_offer_create_answer(const std::string &sdp_str) {
        webrtc::SdpParseError error;
        std::unique_ptr<webrtc::SessionDescriptionInterface> session_description = webrtc::CreateSessionDescription(webrtc::SdpType::kOffer, sdp_str, &error);
        if (session_description == nullptr) {
            logger_->error("Failed to create session description: {}", error.description);
            return false;
        }

        peer_connection_->SetRemoteDescription(DummySetSessionDescriptionObserver::Create().get(), session_description.release());
        peer_connection_->CreateAnswer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
        return true;
    }
};

int main(int argc, char* argv[]) {

    auto logger = spdlog::stdout_color_mt("PeerDataChannel");
    absl::SetProgramUsageMessage(
      "Example usage: ./peer_datachanenl_client --server=localhost --port=5000\n");
    absl::ParseCommandLine(argc, argv);

    logger->info("Starting PeerDataChannelClient");
    // asio::ssl::context *ssl_ctx = new asio::ssl::context(asio::ssl::context::tls);

    auto client = rtc::make_ref_counted<PeerDataChannelClient>();

    rtc::InitializeSSL();

    client->init_webrtc();

    client->init_signaling();

    logger->info("Connecting to {}:{}", absl::GetFlag(FLAGS_server), absl::GetFlag(FLAGS_port));
    client->connect_sync(absl::GetFlag(FLAGS_server), absl::GetFlag(FLAGS_port));

    // client->query_peer_type();

    try {
        while(client->is_connected()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } catch (std::exception &e) {
        logger->error("Terminated by Interrupt: {} ", e.what());
    }

    rtc::CleanupSSL();
    client->deinit_signaling();

    logger->info("Stopped.");
    return 0;
}