#include <chrono>

#include <csignal>
#include <thread>

#include <sio_client.h>
// #include <asio.hpp>
// #include <asio/ssl.hpp>

#include <api/media_stream_interface.h>
#include <api/peer_connection_interface.h>

#include "api/task_queue/default_task_queue_factory.h"

#include <api/peer_connection_interface.h>
#include <api/audio/audio_mixer.h>
#include <api/audio_codecs/audio_decoder_factory.h>
#include <api/audio_codecs/audio_encoder_factory.h>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/audio_options.h>

#include <modules/audio_device/include/audio_device.h>
#include <modules/audio_processing/include/audio_processing.h>

#include <p2p/base/port_allocator.h>
#include <pc/video_track_source.h>

#include <api/create_peerconnection_factory.h>
#include <rtc_base/ssl_adapter.h>
// #include <rtc_base/thread.h>
// #include <system_wrappers/include/field_trial.h>

#include "peer_client.hpp"

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>

ABSL_FLAG(bool, list_devices,       false, "List Audio Devices only, No additional Test");

ABSL_FLAG(std::string, server, "localhost",     "The server to connect to.");
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

class PeerAudioClient : public PeerClient,
                              public webrtc::PeerConnectionObserver,
                              public webrtc::CreateSessionDescriptionObserver
                            {

    rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_ = nullptr;

    std::unique_ptr<rtc::Thread> network_thread_;
    std::unique_ptr<rtc::Thread> worker_thread_;
    std::unique_ptr<rtc::Thread> signaling_thread_;
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory_;
    webrtc::PeerConnectionInterface::RTCConfiguration configuration_;

    // std::unique_ptr<rtc::Thread> signaling_thread_;
public:
    PeerAudioClient(): PeerClient("AudioClient") {

    }

    virtual ~PeerAudioClient() {
        if (peer_connection_) {
            peer_connection_->Close();
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
        logger_->info("PeerAudioClient deleted");
    }

    //
    // PeerConnectionObserver implementation.
    //
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override {
        logger_->info("PeerConnectionInterface::OnSignalingChange: {}", webrtc::PeerConnectionInterface::AsString(new_state));
    }

    void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                    const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> &streams) override {
        logger_->info("PeerConnectionInterface::OnAddTrack: {}", receiver->id());
    }

    void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override {
        logger_->info("PeerConnectionInterface::OnRemoveTrack: {}", receiver->id());
    }

    void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {
        logger_->info("PeerConnectionInterface::OnAddStream: {}", stream.get()->id());
    };

    void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {
        logger_->info("PeerConnectionObserver::RemoveStream: {}", stream.get()->id());
    };

    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {
        logger_->info("PeerConnectionInterface::OnDataChannel: {}", channel->label());
    }
    void OnRenegotiationNeeded() override {
        logger_->info("PeerConnectionInterface::OnRenegotiationNeeded");
    }

    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override {
        logger_->info("PeerConnectionInterface::OnIceConnectionChange: {}", webrtc::PeerConnectionInterface::AsString(new_state));
    }

     virtual void OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState new_state) override {
        logger_->info("PeerConnectionInterface::OnConnectionChange: {}", webrtc::PeerConnectionInterface::AsString(new_state));
     }

    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override {
        logger_->info("PeerConnectionInterface::OnIceGatheringChange: {}", webrtc::PeerConnectionInterface::AsString(new_state));
    }

    void OnIceCandidate(const webrtc::IceCandidateInterface *candidate) override {

        logger_->info("PeerConnectionInterface::OnIceCandidate: {}, {}", candidate->sdp_mid(), candidate->sdp_mline_index());
        std::string candidate_str;
        candidate->ToString(&candidate_str);
        logger_->info("\t- {}", candidate_str);

        sio::message::ptr msg = sio::object_message::create();
        msg->get_map()["sdp_mid"] = sio::string_message::create(candidate->sdp_mid());
        msg->get_map()["sdp_mline_index"] = sio::int_message::create(candidate->sdp_mline_index());
        msg->get_map()["candidate"] = sio::string_message::create(candidate_str);
        sio_client_.socket()->emit("ice", msg, [&](sio::message::list const& msg) {
            std::unique_lock<std::mutex> lock(msg_mutex_);
            bool ok = msg[0]->get_map()["ok"]->get_bool();
            std::string message = msg[0]->get_map()["message"]->get_string();
            if (ok) {
                logger_->info("ICE Candidate sent successfully");
            } else {
                logger_->error("ICE Candidate sent failed to send: {}", message);
            }
        });

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

        sio_client_.socket()->on("ice", sio::socket::event_listener_aux([&](std::string const& name, sio::message::ptr const& data, bool isAck, sio::message::list &ack_resp)
            {
                std::unique_lock<std::mutex> lock(msg_mutex_);
                logger_->info("event={}", name);
                std::string sdp_mid = data->get_map()["sdp_mid"]->get_string();
                int sdp_mline_index = data->get_map()["sdp_mline_index"]->get_int();
                std::string candidate = data->get_map()["candidate"]->get_string();

                std::string err_message;
                if (add_ice_candidate(sdp_mid, sdp_mline_index, candidate, err_message)) {
                    logger_->info("ICE Candidate added successfully");
                    sio::message::ptr resp = sio::object_message::create();
                    resp->get_map()["ok"] = sio::bool_message::create(true);
                    resp->get_map()["message"] = sio::string_message::create("accepted");
                ack_resp.push(resp);
                } else {
                    logger_->error("Failed to add ICE Candidate");
                    sio::message::ptr resp = sio::object_message::create();
                    resp->get_map()["ok"] = sio::bool_message::create(false);
                    resp->get_map()["message"] = sio::string_message::create(err_message);
                    ack_resp.push(resp);
                }
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

    void list_audio_devices(void) {
        rtc::scoped_refptr<webrtc::AudioDeviceModule> audio_device_module =
            webrtc::AudioDeviceModule::Create(webrtc::AudioDeviceModule::kPlatformDefaultAudio, webrtc::CreateDefaultTaskQueueFactory(nullptr).get());

        if (audio_device_module->Init() < 0) {
            logger_->error("Failed to initialize the audio device module");
            return;
        }
        {
            int16_t num_devices = audio_device_module->PlayoutDevices();
            logger_->info("Playout Devices: {}", num_devices);

            for (int16_t i = 0; i < num_devices; ++i) {
                char name[webrtc::kAdmMaxDeviceNameSize];
                char guid[webrtc::kAdmMaxGuidSize];
                audio_device_module->PlayoutDeviceName(i, name, guid);
                logger_->info("\t[{}] {} (guid={})", i, name, guid);
            }
        }
        {
            int16_t num_devices = audio_device_module->RecordingDevices();
            logger_->info("Recording Devices: {}", num_devices);

            for (int16_t i = 0; i < num_devices; ++i) {
                char name[webrtc::kAdmMaxDeviceNameSize];
                char guid[webrtc::kAdmMaxGuidSize];
                audio_device_module->RecordingDeviceName(i, name, guid);
                logger_->info("\t[{}] {} (guid={})", i, name, guid);
            }
        }
    }

    bool init_webrtc(void) {
        webrtc::PeerConnectionInterface::IceServer ice_server;
        ice_server.uri = "stun:stun.l.google.com:19302";
        configuration_.servers.push_back(ice_server);

#if 0
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
            webrtc::CreateBuiltinAudioEncoderFactory(),
            webrtc::CreateBuiltinAudioDecoderFactory(),
            nullptr /* video_encoder_factory */, nullptr /* video_decoder_factory */,
            nullptr /* audio_mixer */, nullptr /* audio_processing */);
        logger_->info("PeerConnectionFactory created successfully.");
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

        if (!peer_connection_->GetSenders().empty()) {
            return false;  // Already added tracks.
        }

        cricket::AudioOptions audio_options;
        logger_->info("audio_options={}", audio_options.ToString());
        rtc::scoped_refptr<webrtc::AudioSourceInterface> audio_source = peer_connection_factory_->CreateAudioSource(audio_options);
        rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
            peer_connection_factory_->CreateAudioTrack("audio_label", audio_source.get())
        );
        auto result_or_error = peer_connection_->AddTrack(audio_track, {"stream_id"});
        if (!result_or_error.ok()) {
            logger_->error("Failed to add audio track to PeerConnection: {}", result_or_error.error().message());
            return false;
        }

        logger_->info("Add audio track successfully.");
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

    bool add_ice_candidate(const std::string &sdp_mid, int sdp_mline_index, const std::string &candidate, std::string &err_message) {
        webrtc::SdpParseError error;
        std::unique_ptr<webrtc::IceCandidateInterface> ice_candidate(webrtc::CreateIceCandidate(sdp_mid, sdp_mline_index, candidate, &error));
        if (!ice_candidate.get()) {
            logger_->error("Failed to create ICE Candidate: {}", error.description);
            err_message = error.description;
            return false;
        }
        if (!peer_connection_->AddIceCandidate(ice_candidate.get())) {
            logger_->error("Failed to add ICE candidate");
            err_message = "Failed to add ICE candidate";
            return false;
        }
        logger_->info("Added ICE Candidate: {}", candidate);
        return true;
    }
};

int main(int argc, char* argv[]) {

    auto logger = spdlog::stdout_color_mt("PeerAudio");
    absl::SetProgramUsageMessage(
      "Example usage: ./peer_audio_client --server=localhost --port=5000\n");
    absl::ParseCommandLine(argc, argv);

    logger->info("Starting PeerAudioClient");
    // asio::ssl::context *ssl_ctx = new asio::ssl::context(asio::ssl::context::tls);

    auto client = rtc::make_ref_counted<PeerAudioClient>();

    if (absl::GetFlag(FLAGS_list_devices)) {
        client->list_audio_devices();
        return 0;
    }

    rtc::InitializeSSL();

    client->init_webrtc();

    client->init_signaling();

    logger->info("Connecting to {}:{}", absl::GetFlag(FLAGS_server), absl::GetFlag(FLAGS_port));
    client->connect_sync(absl::GetFlag(FLAGS_server), absl::GetFlag(FLAGS_port));

    // client->query_peer_type();
    try {
        int count = 0;
        auto b = std::chrono::high_resolution_clock::now();
        while(client->is_connected()) {
        // for (int i = 0; i < 10; i++) {
            // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            if (client->getType() == PeerClient::PeerType::Caller) {
                logger->info("Callee main thread is doing nothing.");
                std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            } else {
                logger->info("Callee main thread is doing nothing.");
                std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            }
        }
    } catch (std::exception &e) {
        logger->error("Terminated by Interrupt: {} ", e.what());
    }

    rtc::CleanupSSL();
    client->deinit_signaling();

    logger->info("Stopped.");
    return 0;
}