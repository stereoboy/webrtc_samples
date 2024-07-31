#include <chrono>

#include <csignal>
#include <thread>

#include <sio_client.h>
// #include <asio.hpp>
// #include <asio/ssl.hpp>

#include <api/media_stream_interface.h>
#include <api/peer_connection_interface.h>

#include <api/peer_connection_interface.h>
#include <api/audio/audio_mixer.h>
#include <api/audio_codecs/audio_decoder_factory.h>
#include <api/audio_codecs/audio_encoder_factory.h>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/audio_options.h>

#include <api/video_codecs/video_decoder_factory.h>
#include <api/video_codecs/video_decoder_factory_template.h>
#include <api/video_codecs/video_decoder_factory_template_dav1d_adapter.h>
#include <api/video_codecs/video_decoder_factory_template_libvpx_vp8_adapter.h>
#include <api/video_codecs/video_decoder_factory_template_libvpx_vp9_adapter.h>
#include <api/video_codecs/video_decoder_factory_template_open_h264_adapter.h>
#include <api/video_codecs/video_encoder_factory.h>
#include <api/video_codecs/video_encoder_factory_template.h>
#include <api/video_codecs/video_encoder_factory_template_libaom_av1_adapter.h>
#include <api/video_codecs/video_encoder_factory_template_libvpx_vp8_adapter.h>
#include <api/video_codecs/video_encoder_factory_template_libvpx_vp9_adapter.h>
#include <api/video_codecs/video_encoder_factory_template_open_h264_adapter.h>

#include <modules/audio_device/include/audio_device.h>
#include <modules/audio_processing/include/audio_processing.h>
#include <modules/video_capture/video_capture.h>
#include <modules/video_capture/video_capture_factory.h>

#include <p2p/base/port_allocator.h>
#include <pc/video_track_source.h>

#include <api/create_peerconnection_factory.h>
#include <rtc_base/ssl_adapter.h>
// #include <rtc_base/thread.h>
// #include <system_wrappers/include/field_trial.h>

#include <test/vcm_capturer.h>


#include "peer_client.hpp"

#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>

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

class CapturerTrackSource : public webrtc::VideoTrackSource {

public:
    // static rtc::scoped_refptr<CapturerTrackSource> Create() {
    //     const size_t kWidth = 640;
    //     const size_t kHeight = 480;
    //     const size_t kFps = 30;
    //     std::unique_ptr<webrtc::test::VcmCapturer> capturer;
    //     std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
    //         webrtc::VideoCaptureFactory::CreateDeviceInfo());
    //     if (!info) {
    //         return nullptr;
    //     }
    //     int num_devices = info->NumberOfDevices();
    //     for (int i = 0; i < num_devices; ++i) {
    //         capturer = absl::WrapUnique(webrtc::test::VcmCapturer::Create(kWidth, kHeight, kFps, i));
    //         if (capturer) {
    //             return rtc::make_ref_counted<CapturerTrackSource>(std::move(capturer));
    //         }
    //     }
    //     return nullptr;
    // }

protected:
    explicit CapturerTrackSource(std::unique_ptr<webrtc::test::VcmCapturer> capturer)
        : VideoTrackSource(/*remote=*/false), capturer_(std::move(capturer)) {}

protected:
    rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
        return capturer_.get();
    }
    std::unique_ptr<webrtc::test::VcmCapturer> capturer_;
};

class PeerVideoClient : public PeerClient,
                              public webrtc::PeerConnectionObserver,
                              public webrtc::DataChannelObserver,
                              public webrtc::CreateSessionDescriptionObserver
                            {

    rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_ = nullptr;
    rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel_;

    std::unique_ptr<rtc::Thread> network_thread_;
    std::unique_ptr<rtc::Thread> worker_thread_;
    std::unique_ptr<rtc::Thread> signaling_thread_;
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory_;
    webrtc::PeerConnectionInterface::RTCConfiguration configuration_;

    std::mutex                      data_channel_mutex_;
    std::condition_variable_any     data_channel_cond_;
    // std::unique_ptr<rtc::Thread> signaling_thread_;
public:
    PeerVideoClient(): PeerClient("VideoClient") {

    }

    virtual ~PeerVideoClient() {
        if (data_channel_) {
            data_channel_->Close();
            data_channel_ = nullptr;
        }

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
        logger_->info("PeerVideoClient deleted");
    }

    rtc::scoped_refptr<CapturerTrackSource> CreateCapturerTrackSource() {
        const size_t kWidth = 640;
        const size_t kHeight = 480;
        const size_t kFps = 30;
        std::unique_ptr<webrtc::test::VcmCapturer> capturer;
        std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(webrtc::VideoCaptureFactory::CreateDeviceInfo());
        if (!info) {
            return nullptr;
        }
        int num_devices = info->NumberOfDevices();
        for (int i = 0; i < num_devices; ++i) {
            char device_name[256];
            char unique_name[256];
            char product_name[256];
            info->GetDeviceName(i, device_name, sizeof(device_name), unique_name, sizeof(unique_name), product_name, sizeof(product_name));
            // logger_->info("\t[{}] Device Name: {}, Unique Name: {}, Product Name: {}", i, device_name, unique_name, product_name);
            logger_->info("Try to create VideoTrackSource from Device[{}]({}, {})", i, device_name, unique_name, product_name);

            capturer = absl::WrapUnique(webrtc::test::VcmCapturer::Create(kWidth, kHeight, kFps, i));
            if (capturer) {
                return rtc::make_ref_counted<CapturerTrackSource>(std::move(capturer));
            }
        }
        return nullptr;
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
        // TODO
        // Start to render Remote Video Track, if stream is video
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
        std::unique_lock<std::mutex> lock(data_channel_mutex_);

        data_channel_ = channel;
        data_channel_->RegisterObserver(this);
        data_channel_cond_.notify_all();
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
    // DataChannelObserver implementation
    //
    void OnStateChange() override {
        logger_->info("DataChannelObserver::StateChange");
    };

    void OnMessage(const webrtc::DataBuffer &buffer) override {
        std::unique_lock<std::mutex> lock(data_channel_mutex_);
        std::string message(buffer.data.data<char>(), buffer.data.size());
        // logger_->info("DataChannelObserver::OnMessage: {}", message);

        if (type_ == PeerClient::PeerType::Callee) {
            std::string response = "response";
            webrtc::DataBuffer buffer(rtc::CopyOnWriteBuffer(response.c_str(), response.size() + 1), true);
            data_channel_->Send(buffer);
        }
        data_channel_cond_.notify_all();
    };

    void OnBufferedAmountChange(uint64_t sent_data_size) override {
        // logger_->info("DataChannelObserver::BufferedAmountChange: {}", sent_data_size);
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
            std::make_unique<webrtc::VideoEncoderFactoryTemplate<
                webrtc::LibvpxVp8EncoderTemplateAdapter,
                webrtc::LibvpxVp9EncoderTemplateAdapter,
                webrtc::OpenH264EncoderTemplateAdapter,
                webrtc::LibaomAv1EncoderTemplateAdapter>>(),
            std::make_unique<webrtc::VideoDecoderFactoryTemplate<
                webrtc::LibvpxVp8DecoderTemplateAdapter,
                webrtc::LibvpxVp9DecoderTemplateAdapter,
                webrtc::OpenH264DecoderTemplateAdapter,
                webrtc::Dav1dDecoderTemplateAdapter>>(),
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

        std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(webrtc::VideoCaptureFactory::CreateDeviceInfo());
        if (!info) {
            logger_->error("Failed to create VideoCaptureFactory::DeviceInfo");
            return false;
        }
        int num_devices = info->NumberOfDevices();
        logger_->info("[Video Capture Device List]");
        for (int i = 0; i < num_devices; ++i) {
            char device_name[256];
            char unique_name[256];
            char product_name[256];
            info->GetDeviceName(i, device_name, sizeof(device_name), unique_name, sizeof(unique_name), product_name, sizeof(product_name));
            // logger_->info("\t[{}] Device Name: {}, Unique Name: {}, Product Name: {}", i, device_name, unique_name, product_name);
            logger_->info("\t[{}] Device Name: {}, Unique Name: {}", i, device_name, unique_name, product_name);
        }

        rtc::scoped_refptr<CapturerTrackSource> video_device = CreateCapturerTrackSource();
        if (!video_device) {
            logger_->error("Failed to create VideoTrackSource ");
            return false;
        }
        rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track(peer_connection_factory_->CreateVideoTrack(video_device, "video_label"));
        result_or_error = peer_connection_->AddTrack(video_track, {"stream_id"});
        if (!result_or_error.ok()) {
            logger_->error("Failed to add video track to PeerConnection: {}", result_or_error.error().message());
        }
        logger_->info("Add video track successfully.");

        // TODO
        // Start to render Local Video Track
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

    void wait_for_data_channel_connection(void) {
        std::unique_lock<std::mutex> lock(data_channel_mutex_);
        if (!connected_) {
            return;
        }
        logger_->info("Waiting for data channel connection");
        data_channel_cond_.wait(data_channel_mutex_);
    }

    void send_message_sync(const std::string &message) {
        std::unique_lock<std::mutex> lock(data_channel_mutex_);
        // logger_->info("Sending message: {}", message);
        webrtc::DataBuffer buffer(rtc::CopyOnWriteBuffer(message.c_str(), message.size() + 1), true);
        data_channel_->Send(buffer);
        data_channel_cond_.wait(data_channel_mutex_);
    }

    void wait_for_message(void) {
        std::unique_lock<std::mutex> lock(data_channel_mutex_);
        data_channel_cond_.wait(data_channel_mutex_);
    }
};

int main(int argc, char* argv[]) {

    auto logger = spdlog::stdout_color_mt("PeerVideo");
    absl::SetProgramUsageMessage(
      "Example usage: ./peer_datachanenl_client --server=localhost --port=5000\n");
    absl::ParseCommandLine(argc, argv);

    logger->info("Starting PeerVideoClient");
    // asio::ssl::context *ssl_ctx = new asio::ssl::context(asio::ssl::context::tls);

    auto client = rtc::make_ref_counted<PeerVideoClient>();

    rtc::InitializeSSL();

    client->init_webrtc();

    client->init_signaling();

    logger->info("Connecting to {}:{}", absl::GetFlag(FLAGS_server), absl::GetFlag(FLAGS_port));
    client->connect_sync(absl::GetFlag(FLAGS_server), absl::GetFlag(FLAGS_port));

    // client->query_peer_type();

    client->wait_for_data_channel_connection();
    try {
        int count = 0;
        auto b = std::chrono::high_resolution_clock::now();
        while(client->is_connected()) {
        // for (int i = 0; i < 10; i++) {
            // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            if (client->getType() == PeerClient::PeerType::Caller) {
                std::string message = "hello world";
                client->send_message_sync(message);
                // client->wait_for_message();

                auto e = std::chrono::high_resolution_clock::now();
                double elapsed = std::chrono::duration<double, std::milli>(e - b).count();
                count++;
                if (elapsed > 10000) {
                    const float hz = count*1000/elapsed;
                    logger->info("Sent {} messages in {} ms ({} Hz)", count, elapsed, hz);
                    count = 0;
                    b = std::chrono::high_resolution_clock::now();
                }
            } else {
                logger->info("Callee main thread is doing nothing.");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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