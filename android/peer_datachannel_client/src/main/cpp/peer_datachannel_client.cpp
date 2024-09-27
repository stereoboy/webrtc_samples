//
// Created by wom on 24. 9. 27..
//

#include <cstring>
#include <jni.h>
#include <cinttypes>
#include <android/log.h>

#include <chrono>

#include <csignal>
#include <thread>

#include <sio_client.h>
#include <memory>
// #include <asio.hpp>
// #include <asio/ssl.hpp>

//#include <api/peer_connection_interface.h>
//#include <api/create_peerconnection_factory.h>
//#include <rtc_base/ssl_adapter.h>
// #include <rtc_base/thread.h>
// #include <system_wrappers/include/field_trial.h>

#include "logging.h"
#include "peer_client.h"

#define SERVER_HOSTNAME "192.168.0.2"
#define SERVER_PORT     5000




//ABSL_FLAG(std::string, server, "localhost", "The server to connect to.");
//ABSL_FLAG(int,
//            port,
//            5000,
//"The port on which the server is listening.");


#if 0
class DummySetSessionDescriptionObserver
        : public webrtc::SetSessionDescriptionObserver {
public:
    static rtc::scoped_refptr<DummySetSessionDescriptionObserver> Create() {
        return rtc::make_ref_counted<DummySetSessionDescriptionObserver>();
    }
    virtual void OnSuccess() {
        RTC_LOG(LS_INFO) << __FUNCTION__;
        LOGI("%s", __PRETTY_FUNCTION__);
    }
    virtual void OnFailure(webrtc::RTCError error) {
        RTC_LOG(LS_INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": " << error.message();
        LOGI("%s", __PRETTY_FUNCTION__);
    }
};
#endif

class PeerDataChannelClient : public PeerClient
//                              public webrtc::PeerConnectionObserver,
//                              public webrtc::DataChannelObserver,
//                              public webrtc::CreateSessionDescriptionObserver
{

//    rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_ = nullptr;
//    rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel_;
//
//    std::unique_ptr<rtc::Thread> network_thread_;
//    std::unique_ptr<rtc::Thread> worker_thread_;
//    std::unique_ptr<rtc::Thread> signaling_thread_;
//    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory_;
//    webrtc::PeerConnectionInterface::RTCConfiguration configuration_;

    std::mutex                      data_channel_mutex_;
    std::condition_variable_any     data_channel_cond_;
    // std::unique_ptr<rtc::Thread> signaling_thread_;
public:
    PeerDataChannelClient(): PeerClient("DataChannelClient") {

    }

    virtual ~PeerDataChannelClient() {
//        if (data_channel_) {
//            data_channel_->Close();
//            data_channel_ = nullptr;
//        }
//
//        if (peer_connection_) {
//            peer_connection_->Close();
//            peer_connection_ = nullptr;
//        }
//
//        if (peer_connection_factory_) {
//            peer_connection_factory_ = nullptr;
//        }
//
//        if (network_thread_) {
//            network_thread_->Stop();
//            network_thread_ = nullptr;
//        }
//
//        if (worker_thread_) {
//            worker_thread_->Stop();
//            worker_thread_ = nullptr;
//        }
//
//        if (signaling_thread_) {
//            signaling_thread_->Stop();
//            signaling_thread_ = nullptr;
//        }
        LOGI("PeerDataChannelClient", "PeerDataChannelClient deleted");
    }

#if 0
    //
    // PeerConnectionObserver implementation.
    //
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override {
        LOGI("PeerDataChannelClient", "PeerConnectionInterface::OnSignalingChange: {}", webrtc::PeerConnectionInterface::AsString(new_state));
    }
    // void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
    //                 const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
    //                 streams) override {}
    // void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override {}

    void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {
        LOGI("PeerDataChannelClient", "PeerConnectionInterface::OnAddStream: {}", stream.get()->id());
    };

    void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {
        LOGI("PeerDataChannelClient", "PeerConnectionObserver::RemoveStream: {}", stream.get()->id());
    };

    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {
        LOGI("PeerDataChannelClient", "PeerConnectionInterface::OnDataChannel: {}", channel->label());
        std::unique_lock<std::mutex> lock(data_channel_mutex_);

        data_channel_ = channel;
        data_channel_->RegisterObserver(this);
        data_channel_cond_.notify_all();
    }
    void OnRenegotiationNeeded() override {
        LOGI("PeerDataChannelClient", "PeerConnectionInterface::OnRenegotiationNeeded");
    }

    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override {
        LOGI("PeerDataChannelClient", "PeerConnectionInterface::OnIceConnectionChange: {}", webrtc::PeerConnectionInterface::AsString(new_state));
    }

    virtual void OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState new_state) override {
        LOGI("PeerDataChannelClient", "PeerConnectionInterface::OnConnectionChange: {}", webrtc::PeerConnectionInterface::AsString(new_state));
    }

    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override {
        LOGI("PeerDataChannelClient", "PeerConnectionInterface::OnIceGatheringChange: {}", webrtc::PeerConnectionInterface::AsString(new_state));
    }

    void OnIceCandidate(const webrtc::IceCandidateInterface *candidate) override {

        LOGI("PeerDataChannelClient", "PeerConnectionInterface::OnIceCandidate: {}, {}", candidate->sdp_mid(), candidate->sdp_mline_index());
        std::string candidate_str;
        candidate->ToString(&candidate_str);
        LOGI("PeerDataChannelClient", "\t- {}", candidate_str);

        sio::message::ptr msg = sio::object_message::create();
        msg->get_map()["sdp_mid"] = sio::string_message::create(candidate->sdp_mid());
        msg->get_map()["sdp_mline_index"] = sio::int_message::create(candidate->sdp_mline_index());
        msg->get_map()["candidate"] = sio::string_message::create(candidate_str);
        sio_client_.socket()->emit("ice", msg, [&](sio::message::list const& msg) {
            std::unique_lock<std::mutex> lock(msg_mutex_);
            bool ok = msg[0]->get_map()["ok"]->get_bool();
            std::string message = msg[0]->get_map()["message"]->get_string();
            if (ok) {
                LOGI("PeerDataChannelClient", "ICE Candidate sent successfully");
            } else {
                LOGE("PeerDataChannelClient", "ICE Candidate sent failed to send: {}", message);
            }
        });

    };

    //
    // CreateSessionDescriptionObserver implementation
    //
    void OnSuccess(webrtc::SessionDescriptionInterface *desc) override {
        LOGI("PeerDataChannelClient", "CreateSessionDescriptionObserver::OnSuccess({})");
        peer_connection_->SetLocalDescription(DummySetSessionDescriptionObserver::Create().get(), desc);

        std::string sdp_str;
        desc->ToString(&sdp_str);
        LOGI("PeerDataChannelClient", "SDP:\n{}", sdp_str);

        sio::message::ptr msg = sio::string_message::create(sdp_str);

        if (type_ == PeerClient::PeerType::Caller) {
            sio_client_.socket()->emit("offer", msg, [&](sio::message::list const& msg) {
                std::unique_lock<std::mutex> lock(msg_mutex_);
                bool ok = msg[0]->get_map()["ok"]->get_bool();
                std::string message = msg[0]->get_map()["message"]->get_string();
                if (ok) {
                    LOGI("PeerDataChannelClient", "Offer SDP sent successfully");
                } else {
                    LOGE("PeerDataChannelClient", "Offer SDP failed to send: {}", message);
                }
            });
        } else {
            sio_client_.socket()->emit("answer", msg, [&](sio::message::list const& msg) {
                std::unique_lock<std::mutex> lock(msg_mutex_);
                bool ok = msg[0]->get_map()["ok"]->get_bool();
                std::string message = msg[0]->get_map()["message"]->get_string();
                if (ok) {
                    LOGI("PeerDataChannelClient", "Answer SDP sent successfully");
                } else {
                    LOGE("PeerDataChannelClient", "Answer SDP failed to send: {}", message);
                }
            });
        }
    };

    void OnFailure(webrtc::RTCError error) override {
        LOGI("PeerDataChannelClient", "CreateSessionDescriptionObserver::OnFailure({})", error.message());
    };

    //
    // DataChannelObserver implementation
    //
    void OnStateChange() override {
        LOGI("PeerDataChannelClient", "DataChannelObserver::StateChange");
    };

    void OnMessage(const webrtc::DataBuffer &buffer) override {
        std::unique_lock<std::mutex> lock(data_channel_mutex_);
        std::string message(buffer.data.data<char>(), buffer.data.size());
        // LOGI("PeerDataChannelClient", "DataChannelObserver::OnMessage: {}", message);

        if (type_ == PeerClient::PeerType::Callee) {
            std::string response = "response";
            webrtc::DataBuffer buffer(rtc::CopyOnWriteBuffer(response.c_str(), response.size() + 1), true);
            data_channel_->Send(buffer);
        }
        data_channel_cond_.notify_all();
    };

    void OnBufferedAmountChange(uint64_t sent_data_size) override {
        // LOGI("PeerDataChannelClient", "DataChannelObserver::BufferedAmountChange: {}", sent_data_size);
    };
#endif
    //
    //
    //
    virtual void init_signaling(void) override {
        PeerClient::init_signaling();

        sio_client_.socket()->on("set-as-caller", sio::socket::event_listener_aux([&](std::string const& name, sio::message::ptr const& data, bool isAck, sio::message::list &ack_resp)
                                                                                  {
                                                                                      std::unique_lock<std::mutex> lock(msg_mutex_);
                                                                                      LOGI("PeerDataChannelClient", "event=%s", name.c_str());
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
                                                                                      LOGI("PeerDataChannelClient", "event=%s", name.c_str());
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
                                                                              LOGI("PeerDataChannelClient", "event=%s", name.c_str());
                                                                              if (type_ == PeerClient::PeerType::Caller) {
                                                                                  if (!create_offer()) {
                                                                                      LOGE("PeerDataChannelClient", "Failed to create Offer");
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
                                                                              LOGI("PeerDataChannelClient", "event=%s", name.c_str());
                                                                              if (type_ == PeerClient::PeerType::Callee) {
                                                                                  std::string sdp_str = data->get_string();
                                                                                  LOGI("PeerDataChannelClient", "received Offer SDP:\n%s", sdp_str.c_str());
                                                                                  if (!receive_offer_create_answer(sdp_str)){
                                                                                      LOGE("PeerDataChannelClient", "Failed to receive/create Answer");
                                                                                      sio::message::ptr resp = sio::object_message::create();
                                                                                      resp->get_map()["ok"] = sio::bool_message::create(false);
                                                                                      resp->get_map()["message"] = sio::string_message::create("Failed to receive/create Answer");
                                                                                      ack_resp.push(resp);
                                                                                      return;
                                                                                  }
                                                                              } else {
                                                                                  LOGE("PeerDataChannelClient", "Unexpected Offer");
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
                                                                               LOGI("PeerDataChannelClient", "event=%s", name.c_str());
                                                                               if (type_ == PeerClient::PeerType::Caller) {
                                                                                   std::string sdp_str = data->get_string();
                                                                                   LOGI("PeerDataChannelClient", "received Answer SDP:\n%s", sdp_str.c_str());
                                                                                   if (!receive_answer(sdp_str)) {
                                                                                       LOGE("PeerDataChannelClient", "Failed to receive Answer");
                                                                                       sio::message::ptr resp = sio::object_message::create();
                                                                                       resp->get_map()["ok"] = sio::bool_message::create(false);
                                                                                       resp->get_map()["message"] = sio::string_message::create("Failed to receive Answer");
                                                                                       ack_resp.push(resp);
                                                                                       return;
                                                                                   }
                                                                               } else {
                                                                                   LOGE("PeerDataChannelClient", "Unexpected Answer");
                                                                               }
                                                                               sio::message::ptr resp = sio::object_message::create();
                                                                               resp->get_map()["ok"] = sio::bool_message::create(true);
                                                                               resp->get_map()["message"] = sio::string_message::create("accepted");
                                                                               ack_resp.push(resp);
                                                                           }
        ));

        sio_client_.socket()->on("ice", sio::socket::event_listener_aux([&](std::string const& name, sio::message::ptr const& data, bool isAck, sio::message::list &ack_resp)
                                                                        {
                                                                            std::unique_lock<std::mutex> lock(msg_mutex_);
                                                                            LOGI("PeerDataChannelClient", "event=%s", name.c_str());
                                                                            std::string sdp_mid = data->get_map()["sdp_mid"]->get_string();
                                                                            int sdp_mline_index = data->get_map()["sdp_mline_index"]->get_int();
                                                                            std::string candidate = data->get_map()["candidate"]->get_string();

                                                                            std::string err_message;
                                                                            if (add_ice_candidate(sdp_mid, sdp_mline_index, candidate, err_message)) {
                                                                                LOGI("PeerDataChannelClient", "ICE Candidate added successfully");
                                                                                sio::message::ptr resp = sio::object_message::create();
                                                                                resp->get_map()["ok"] = sio::bool_message::create(true);
                                                                                resp->get_map()["message"] = sio::string_message::create("accepted");
                                                                                ack_resp.push(resp);
                                                                            } else {
                                                                                LOGE("PeerDataChannelClient", "Failed to add ICE Candidate");
                                                                                sio::message::ptr resp = sio::object_message::create();
                                                                                resp->get_map()["ok"] = sio::bool_message::create(false);
                                                                                resp->get_map()["message"] = sio::string_message::create(err_message);
                                                                                ack_resp.push(resp);
                                                                            }
                                                                        }
        ));
    }

    void query_peer_type(void) {
        LOGI("PeerDataChannelClient", ">>> query-peer-type");
        std::string msg = "dummy";
        sio_client_.socket()->emit("query-peer-type", msg, [&](sio::message::list const& msg) {
            // std::unique_lock<std::mutex> lock(msg_mutex_);
            std::string res = msg[0]->get_string();
            if (res.compare("caller") == 0) {
                type_ = PeerClient::PeerType::Caller;
                LOGI("PeerDataChannelClient", "caller");
            } else if (res.compare("callee") == 0) {
                type_ = PeerClient::PeerType::Callee;
                LOGI("PeerDataChannelClient", "callee");
            }
            msg_cond_.notify_all();
        });

        std::unique_lock<std::mutex> lock(msg_mutex_);
        msg_cond_.wait(msg_mutex_);
        LOGI("PeerDataChannelClient", "<<< query-peer-type");
    }

    bool init_webrtc(void) {
#if 0
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
            LOGI("PeerDataChannelClient", "peer_connection_ created successfully.");
        } else {
            LOGE("PeerDataChannelClient", "Failed to create PeerConnection: {}", error_or_peer_connection.error().message());
            return false;
        }

        webrtc::DataChannelInit config;
        auto error_or_data_channel = peer_connection_->CreateDataChannelOrError("data_channel", &config);
        if (error_or_data_channel.ok()) {
            data_channel_ = std::move(error_or_data_channel.value());
            LOGI("PeerDataChannelClient", "data_channel_ created successfully");
        } else {
            LOGE("PeerDataChannelClient", "Failed to create DataChannel: {}", error_or_data_channel.error().message());
            return false;
        }

        data_channel_->RegisterObserver(this);
#endif
        return true;
    }

    bool create_offer(void) {
#if 0
        peer_connection_->CreateOffer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
#endif
        return true;
    }

    bool receive_answer(const std::string &sdp_str) {
#if 0
        webrtc::SdpParseError error;
        std::unique_ptr<webrtc::SessionDescriptionInterface> session_description = webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, sdp_str, &error);
        if (session_description == nullptr) {
            LOGE("PeerDataChannelClient", "Failed to create session description: {}", error.description);
            return false;
        }
        peer_connection_->SetRemoteDescription(DummySetSessionDescriptionObserver::Create().get(), session_description.release());
#endif
        return true;
    }

    bool receive_offer_create_answer(const std::string &sdp_str) {
#if 0
        webrtc::SdpParseError error;
        std::unique_ptr<webrtc::SessionDescriptionInterface> session_description = webrtc::CreateSessionDescription(webrtc::SdpType::kOffer, sdp_str, &error);
        if (session_description == nullptr) {
            LOGE("PeerDataChannelClient", "Failed to create session description: {}", error.description);
            return false;
        }

        peer_connection_->SetRemoteDescription(DummySetSessionDescriptionObserver::Create().get(), session_description.release());
        peer_connection_->CreateAnswer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
#endif
        return true;
    }

    bool add_ice_candidate(const std::string &sdp_mid, int sdp_mline_index, const std::string &candidate, std::string &err_message) {
#if 0
        webrtc::SdpParseError error;
        std::unique_ptr<webrtc::IceCandidateInterface> ice_candidate(webrtc::CreateIceCandidate(sdp_mid, sdp_mline_index, candidate, &error));
        if (!ice_candidate.get()) {
            LOGE("PeerDataChannelClient", "Failed to create ICE Candidate: {}", error.description);
            err_message = error.description;
            return false;
        }
        if (!peer_connection_->AddIceCandidate(ice_candidate.get())) {
            LOGE("PeerDataChannelClient", "Failed to add ICE candidate");
            err_message = "Failed to add ICE candidate";
            return false;
        }
        LOGI("PeerDataChannelClient", "Added ICE Candidate: {}", candidate);
#endif
        return true;
    }

    void wait_for_data_channel_connection(void) {
        std::unique_lock<std::mutex> lock(data_channel_mutex_);
        if (!connected_) {
            return;
        }
        LOGI("PeerDataChannelClient", "Waiting for data channel connection");
        data_channel_cond_.wait(data_channel_mutex_);
    }

    void send_message_sync(const std::string &message) {
#if 0
        std::unique_lock<std::mutex> lock(data_channel_mutex_);
        // LOGI("PeerDataChannelClient", "Sending message: {}", message);
        webrtc::DataBuffer buffer(rtc::CopyOnWriteBuffer(message.c_str(), message.size() + 1), true);
        data_channel_->Send(buffer);
        data_channel_cond_.wait(data_channel_mutex_);
#endif
    }

    void wait_for_message(void) {
#if 0
        std::unique_lock<std::mutex> lock(data_channel_mutex_);
        data_channel_cond_.wait(data_channel_mutex_);
#endif
    }
};
//
//int main(int argc, char* argv[]) {
//
//    auto logger = spdlog::stdout_color_mt("PeerDataChannel");
//    absl::SetProgramUsageMessage(
//            "Example usage: ./peer_datachanenl_client --server=localhost --port=5000\n");
//    absl::ParseCommandLine(argc, argv);
//
//    logger->info("Starting PeerDataChannelClient");
//    // asio::ssl::context *ssl_ctx = new asio::ssl::context(asio::ssl::context::tls);
//
//    auto client = rtc::make_ref_counted<PeerDataChannelClient>();
//
//    rtc::InitializeSSL();
//
//    client->init_webrtc();
//
//    client->init_signaling();
//
//    logger->info("Connecting to {}:{}", absl::GetFlag(FLAGS_server), absl::GetFlag(FLAGS_port));
//    client->connect_sync(absl::GetFlag(FLAGS_server), absl::GetFlag(FLAGS_port));
//
//    // client->query_peer_type();
//
//    client->wait_for_data_channel_connection();
//    try {
//        int count = 0;
//        auto b = std::chrono::high_resolution_clock::now();
//        while(client->is_connected()) {
//            // for (int i = 0; i < 10; i++) {
//            // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//            if (client->getType() == PeerClient::PeerType::Caller) {
//                std::string message = "hello world";
//                client->send_message_sync(message);
//                // client->wait_for_message();
//
//                auto e = std::chrono::high_resolution_clock::now();
//                double elapsed = std::chrono::duration<double, std::milli>(e - b).count();
//                count++;
//                if (elapsed > 10000) {
//                    const float hz = count*1000/elapsed;
//                    logger->info("Sent {} messages in {} ms ({} Hz)", count, elapsed, hz);
//                    count = 0;
//                    b = std::chrono::high_resolution_clock::now();
//                }
//            } else {
//                logger->info("Callee main thread is doing nothing.");
//                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//            }
//        }
//    } catch (std::exception &e) {
//        logger->error("Terminated by Interrupt: {} ", e.what());
//    }
//
//    rtc::CleanupSSL();
//    client->deinit_signaling();
//
//    logger->info("Stopped.");
//    return 0;
//}

static jfieldID custom_data_field_id;

struct NativeData {
    std::shared_ptr<PeerDataChannelClient> client = nullptr;
};

static std::unique_ptr<NativeData> data = nullptr;

extern "C" JNIEXPORT void JNICALL
Java_com_stereoboy_peer_1datachannel_1client_MainActivity_initNative(JNIEnv *env, jobject thiz) {

//    auto logger = spdlog::stdout_color_mt("PeerDataChannel");
//    absl::SetProgramUsageMessage(
//            "Example usage: ./peer_datachanenl_client --server=localhost --port=5000\n");
//    absl::ParseCommandLine(argc, argv);

    LOGI("PeerDataChannel", "Starting PeerDataChannelClient");
    // asio::ssl::context *ssl_ctx = new asio::ssl::context(asio::ssl::context::tls);

    auto client = std::make_shared<PeerDataChannelClient>();
//    auto client = rtc::make_ref_counted<PeerDataChannelClient>();
//
//    rtc::InitializeSSL();

    client->init_webrtc();

    client->init_signaling();

    LOGI("PeerDataChannel", "Connecting to %s:%d", SERVER_HOSTNAME, SERVER_PORT);
    client->connect_sync(SERVER_HOSTNAME, SERVER_PORT);

    // client->query_peer_type();

//    client->wait_for_data_channel_connection();
//    try {
//        int count = 0;
//        auto b = std::chrono::high_resolution_clock::now();
//        while(client->is_connected()) {
//            // for (int i = 0; i < 10; i++) {
//            // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//            if (client->getType() == PeerClient::PeerType::Caller) {
//                std::string message = "hello world";
//                client->send_message_sync(message);
//                // client->wait_for_message();
//
//                auto e = std::chrono::high_resolution_clock::now();
//                double elapsed = std::chrono::duration<double, std::milli>(e - b).count();
//                count++;
//                if (elapsed > 10000) {
//                    const float hz = count*1000/elapsed;
//                    LOGI("PeerDataChannel", "Sent {} messages in {} ms ({} Hz)", count, elapsed, hz);
//                    count = 0;
//                    b = std::chrono::high_resolution_clock::now();
//                }
//            } else {
//                LOGI("PeerDataChannel", "Callee main thread is doing nothing.");
//                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//            }
//        }
//    } catch (std::exception &e) {
//        LOGE("PeerDataChannel", "Terminated by Interrupt: {} ", e.what());
//    }
//
////    rtc::CleanupSSL();
//    client->deinit_signaling();
//
//    LOGI("PeerDataChannel", "Stopped.");


    jclass klass = env->GetObjectClass(thiz);
    custom_data_field_id = env->GetFieldID (klass, "nativeData", "J");

#if 0 // FIXME: shared_ptr in memory allocated by std::malloc is not working well
    struct NativeData *data = (struct NativeData *)std::malloc(sizeof(struct NativeData));
    if (!data) {
        LOGE("PeerDataChannel", "failed to call std::malloc(sizeof(struct NativeData))");
        return;
    }
#else
    data = std::make_unique<NativeData>();
#endif
    data->client = client;
#if 0
    env->SetLongField (thiz, custom_data_field_id, (jlong)data);
#endif
    return;
}

extern "C" JNIEXPORT void JNICALL
Java_com_stereoboy_peer_1datachannel_1client_MainActivity_deinitNative(JNIEnv *env, jobject thiz) {
#if 0
    struct NativeData *data = (struct NativeData *)env->GetLongField (thiz, custom_data_field_id);
#endif
    auto client = data->client;
//    try {
//        int count = 0;
//        auto b = std::chrono::high_resolution_clock::now();
//        while(client->is_connected()) {
//            // for (int i = 0; i < 10; i++) {
//            // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//            if (client->getType() == PeerClient::PeerType::Caller) {
//                std::string message = "hello world";
//                client->send_message_sync(message);
//                // client->wait_for_message();
//
//                auto e = std::chrono::high_resolution_clock::now();
//                double elapsed = std::chrono::duration<double, std::milli>(e - b).count();
//                count++;
//                if (elapsed > 10000) {
//                    const float hz = count*1000/elapsed;
//                    LOGI("PeerDataChannel", "Sent {} messages in {} ms ({} Hz)", count, elapsed, hz);
//                    count = 0;
//                    b = std::chrono::high_resolution_clock::now();
//                }
//            } else {
//                LOGI("PeerDataChannel", "Callee main thread is doing nothing.");
//                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//            }
//        }
//    } catch (std::exception &e) {
//        LOGE("PeerDataChannel", "Terminated by Interrupt: {} ", e.what());
//    }
//
//    rtc::CleanupSSL();

    client->deinit_signaling();
    data->client = nullptr;
    client = nullptr;
#if 0 // FIXME: shared_ptr in memory allocated by std::malloc is not working well
    std::free(data);
    env->SetLongField (thiz, custom_data_field_id, (jlong) nullptr);
#endif
    LOGI("PeerDataChannel", "Stopped.");
    return;
}
