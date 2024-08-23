#include <chrono>

#include <csignal>
#include <cassert>

#include <thread>

#include <sio_client.h>
// #include <asio.hpp>
// #include <asio/ssl.hpp>

#include <api/media_stream_interface.h>
#include <api/peer_connection_interface.h>

#include "api/video/i420_buffer.h"
#include "api/video/video_frame_buffer.h"
#include "api/video/video_rotation.h"
#include "api/video/video_source_interface.h"

#include <api/audio_codecs/audio_decoder_factory.h>
#include <api/audio_codecs/audio_encoder_factory.h>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>

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

#include <cairo.h>
#include <gtk/gtk.h>

#include <libyuv/convert.h>
#include <libyuv/convert_from.h>

ABSL_FLAG(std::string, server, "localhost", "The server to connect to.");
ABSL_FLAG(int,
          port,
          5000,
          "The port on which the server is listening.");


#define TARGET_VIDEO_WIDTH      1920.0
#define TARGET_VIDEO_HEIGHT     1080.0

#define VIS_VIDEO_HEIGHT        480.0
#define VIS_VIDEO_WIDTH         (TARGET_VIDEO_WIDTH/TARGET_VIDEO_HEIGHT)*VIS_VIDEO_HEIGHT

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
    static rtc::scoped_refptr<CapturerTrackSource> Create() {
        auto logger = spdlog::stdout_color_mt("CapturerTrackSource");

        const size_t kWidth = TARGET_VIDEO_WIDTH;
        const size_t kHeight = TARGET_VIDEO_HEIGHT;
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
            // logger->info("\t[{}] Device Name: {}, Unique Name: {}, Product Name: {}", i, device_name, unique_name, product_name);
            logger->info("Try to create VideoTrackSource from Device[{}]({}, {})", i, device_name, unique_name, product_name);
            capturer = absl::WrapUnique(webrtc::test::VcmCapturer::Create(kWidth, kHeight, kFps, i));
            if (capturer) {
                return rtc::make_ref_counted<CapturerTrackSource>(std::move(capturer));
            }
        }
        return nullptr;
    }

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
                              public webrtc::CreateSessionDescriptionObserver
                            {

    class VideoRenderer : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
    public:
        VideoRenderer(const std::string name, PeerVideoClient *peer_client, webrtc::VideoTrackInterface* track_to_render, GtkWidget *&gtk3_drawing_area)
        :   name_(name),
            peer_client_(peer_client),
            rendered_track_(track_to_render),
            gtk3_drawing_area_(gtk3_drawing_area) {
            assert(gtk3_drawing_area_);
            rendered_track_->AddOrUpdateSink(this, rtc::VideoSinkWants());
        }

        virtual ~VideoRenderer() {

        }

        //
        // VideoSinkInterface implementation
        //
        void OnFrame(const webrtc::VideoFrame& video_frame) override {
            // peer_client_->logger_->info("VideoSinkInterface::OnFrame: {} {}x{}", name_, video_frame.width(), video_frame.height());

            // printf("peer_client_=(%p)\n", peer_client_);
            // printf("peer_client_->gtk3_drawing_area_local_=(%p)\n", peer_client_->gtk3_drawing_area_local_);
            // printf("peer_client_->gtk3_drawing_area_remote_=(%p)\n", peer_client_->gtk3_drawing_area_remote_);

            // gdk_threads_enter();

            rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(
                video_frame.video_frame_buffer()->ToI420());
            if (video_frame.rotation() != webrtc::kVideoRotation_0) {
                buffer = webrtc::I420Buffer::Rotate(*buffer, video_frame.rotation());
            }
            {
                std::unique_lock<std::mutex> lock(mutex_);


                SetSize(buffer->width(), buffer->height());

                // TODO(bugs.webrtc.org/6857): This conversion is correct for little-endian
                // only. Cairo ARGB32 treats pixels as 32-bit values in *native* byte order,
                // with B in the least significant byte of the 32-bit value. Which on
                // little-endian means that memory layout is BGRA, with the B byte stored at
                // lowest address. Libyuv's ARGB format (surprisingly?) uses the same
                // little-endian format, with B in the first byte in memory, regardless of
                // native endianness.
                libyuv::I420ToARGB(buffer->DataY(), buffer->StrideY(), buffer->DataU(),
                                    buffer->StrideU(), buffer->DataV(), buffer->StrideV(),
                                    image_.get(), width_ * 4, buffer->width(),
                                    buffer->height());

            }

            // gdk_threads_leave();

            if (gtk3_drawing_area_) {
                // gtk_widget_set_size_request (gtk3_drawing_area_, buffer->width(), buffer->height());
                gtk_widget_queue_draw(gtk3_drawing_area_);
            }

        }

        const uint8_t* image() const { return image_.get(); }

        int width() const { return width_; }

        int height() const { return height_; }

    // protected:
        void SetSize(int width, int height) {
            // gdk_threads_enter();

            if (width_ == width && height_ == height) {
                return;
            }

            width_ = width;
            height_ = height;
            image_.reset(new uint8_t[width * height * 4]);
            // gdk_threads_leave();
        }
        int width_      = 0;
        int height_     = 0;
        std::string                 name_;
        std::unique_ptr<uint8_t[]>  image_;
        PeerVideoClient             *peer_client_ = nullptr;
        GtkWidget                   *&gtk3_drawing_area_;
        std::mutex                  mutex_;
        rtc::scoped_refptr<webrtc::VideoTrackInterface> rendered_track_;
    };

    rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_ = nullptr;

    std::unique_ptr<rtc::Thread> network_thread_;
    std::unique_ptr<rtc::Thread> worker_thread_;
    std::unique_ptr<rtc::Thread> signaling_thread_;
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory_;
    webrtc::PeerConnectionInterface::RTCConfiguration configuration_;

    rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_;

    // std::unique_ptr<rtc::Thread> signaling_thread_;

    GtkWidget           *gtk3_window_ = nullptr;
    GtkWidget           *gtk3_grid_ = nullptr;
    GtkWidget           *gtk3_label_local_ = nullptr;
    GtkWidget           *gtk3_drawing_area_local_ = nullptr;
    GtkWidget           *gtk3_label_remote_ = nullptr;
    GtkWidget           *gtk3_drawing_area_remote_ = nullptr;

    std::unique_ptr<VideoRenderer> local_renderer_;
    std::unique_ptr<VideoRenderer> remote_renderer_;
    std::unique_ptr<uint8_t[]> local_buffer_;
    std::unique_ptr<uint8_t[]> remote_buffer_;

    void StartLocalRenderer(webrtc::VideoTrackInterface* local_video) {
        logger_->info("StartLocalRenderer()");
        local_renderer_.reset(new VideoRenderer("Local", this, local_video, gtk3_drawing_area_local_));
    }

    void StopLocalRenderer() {
        logger_->info("StopLocalRenderer()");
        local_renderer_.reset();
    }

    void StartRemoteRenderer(webrtc::VideoTrackInterface* remote_video) {
        logger_->info("StartRemoteRenderer()");
        remote_renderer_.reset(new VideoRenderer("Remote", this, remote_video, gtk3_drawing_area_remote_));
    }

    void StopRemoteRenderer() {
        logger_->info("StopRemoteRenderer()");
        remote_renderer_.reset();
    }

public:
    PeerVideoClient(): PeerClient("VideoClient") {

    }

    virtual ~PeerVideoClient() {

        StopLocalRenderer();
        StopRemoteRenderer();

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
        ;
        auto* track = reinterpret_cast<webrtc::MediaStreamTrackInterface*>(receiver->track().release());
        if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
            auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
            StartRemoteRenderer(video_track);
        }
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

        if (!peer_connection_->GetSenders().empty()) {
            return false;  // Already added tracks.
        }

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

        rtc::scoped_refptr<CapturerTrackSource> video_device = CapturerTrackSource::Create();
        if (!video_device) {
            logger_->error("Failed to create VideoTrackSource ");
            return false;
        }
        video_track_ = peer_connection_factory_->CreateVideoTrack(video_device, "video_label");
        // video_track_->AddOrUpdateSink(this, rtc::VideoSinkWants());
        StartLocalRenderer(video_track_.get());
        auto result_or_error = peer_connection_->AddTrack(video_track_, {"stream_id"});
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

    static gboolean draw_local_callback (GtkWidget *widget, cairo_t *cr, gpointer data) {
        return static_cast<PeerVideoClient *>(data)->draw_local_callback_handler(widget, cr);
    }

    gboolean draw_local_callback_handler (GtkWidget *widget, cairo_t *cr) {
        guint width = 0, height = 0;
        cairo_matrix_t matrix;

        if (local_renderer_) {

            std::unique_lock<std::mutex> lock(local_renderer_->mutex_);
            width = local_renderer_->width();
            height = local_renderer_->height();

            if (width == 0 || height == 0) {
                return FALSE;
            }
            // logger_->info("draw_local_callback_handler: {}x{}", width, height);
            cairo_get_matrix (cr, &matrix);
            cairo_format_t format = CAIRO_FORMAT_ARGB32;
            cairo_surface_t* surface = cairo_image_surface_create_for_data(
                (unsigned char *)local_renderer_->image(), format, width, height,
                cairo_format_stride_for_width(format, width));

            float ratio_x = static_cast<float>(VIS_VIDEO_WIDTH) / TARGET_VIDEO_WIDTH;
            float ratio_y = static_cast<float>(VIS_VIDEO_HEIGHT) / TARGET_VIDEO_HEIGHT;
            cairo_scale(cr, ratio_x, ratio_y);

            cairo_set_source_surface(cr, surface, 0, 0);
            // cairo_rectangle(cr, 0, 0, width, height);
            // cairo_fill(cr);
            cairo_paint(cr);

            cairo_surface_destroy(surface);

            cairo_set_matrix(cr, &matrix);
        }

        cairo_rectangle (cr, 0, 0, 200, 30);

        cairo_set_source_rgb (cr, 0, 0, 0);

        cairo_fill (cr);

        cairo_select_font_face (cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size (cr, 10);
        cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
        cairo_move_to (cr, 10, 20);

        char text[256];
        snprintf(text, sizeof(text), "Local Video: %dx%d", width, height);
        cairo_show_text (cr, text);

        return FALSE;
    }

    static gboolean draw_remote_callback (GtkWidget *widget, cairo_t *cr, gpointer data) {
        return static_cast<PeerVideoClient *>(data)->draw_remote_callback_handler(widget, cr);
    }

    gboolean draw_remote_callback_handler (GtkWidget *widget, cairo_t *cr) {
        guint width = 0, height = 0;
        cairo_matrix_t matrix;

        if (remote_renderer_) {
            std::unique_lock<std::mutex> lock(remote_renderer_->mutex_);
            width = remote_renderer_->width();
            height = remote_renderer_->height();

            if (width == 0 || height == 0) {
                return FALSE;
            }
            // logger_->info("draw_remote_callback_handler: {}x{}", width, height);
            cairo_get_matrix (cr, &matrix);
            cairo_format_t format = CAIRO_FORMAT_ARGB32;
            cairo_surface_t* surface = cairo_image_surface_create_for_data(
                (unsigned char *)remote_renderer_->image(), format, width, height,
                cairo_format_stride_for_width(format, width));

            float ratio_x = static_cast<float>(VIS_VIDEO_WIDTH) / TARGET_VIDEO_WIDTH;
            float ratio_y = static_cast<float>(VIS_VIDEO_HEIGHT) / TARGET_VIDEO_HEIGHT;
            cairo_scale(cr, ratio_x, ratio_y);

            cairo_set_source_surface(cr, surface, 0, 0);
            // cairo_rectangle(cr, 0, 0, width, height);
            // cairo_fill(cr);
            cairo_paint(cr);

            cairo_surface_destroy(surface);

            cairo_set_matrix(cr, &matrix);
        }

        cairo_rectangle (cr, 0, 0, 200, 30);

        cairo_set_source_rgb (cr, 0, 0, 0);

        cairo_fill (cr);

        cairo_select_font_face (cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size (cr, 10);
        cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
        cairo_move_to (cr, 10, 20);

        char text[256];
        snprintf(text, sizeof(text), "Remote Video: %dx%d", width, height);
        cairo_show_text (cr, text);

        return FALSE;
    }

    static void gtk3_window_activate_callback (GtkApplication* app, gpointer user_data) {
        static_cast<PeerVideoClient*>(user_data)->gtk3_window_activate_callback_handler(app);
    }

    void gtk3_window_activate_callback_handler (GtkApplication* app) {

        gtk3_window_ = gtk_application_window_new (app);

        gtk3_label_local_ = gtk_label_new("local video");
        gtk3_drawing_area_local_ = gtk_drawing_area_new ();
        gtk_widget_set_size_request (gtk3_drawing_area_local_, VIS_VIDEO_WIDTH, VIS_VIDEO_HEIGHT);
        g_signal_connect (G_OBJECT (gtk3_drawing_area_local_), "draw", G_CALLBACK (draw_local_callback), this);
        gtk3_label_remote_ = gtk_label_new("remote video");
        gtk3_drawing_area_remote_ = gtk_drawing_area_new ();
        gtk_widget_set_size_request (gtk3_drawing_area_remote_, VIS_VIDEO_WIDTH, VIS_VIDEO_HEIGHT);
        g_signal_connect (G_OBJECT (gtk3_drawing_area_remote_), "draw", G_CALLBACK (draw_remote_callback), this);
        gtk3_grid_ = gtk_grid_new();

        gtk_grid_set_row_spacing(GTK_GRID(gtk3_grid_), 2);
        gtk_grid_set_column_spacing(GTK_GRID(gtk3_grid_), 2);

        // gtk_grid_attach(GTK_GRID(gtk3_grid_), gtk3_label_local_, 0, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(gtk3_grid_), gtk3_drawing_area_local_, 0, 1, 1, 1);
        // gtk_grid_attach(GTK_GRID(gtk3_grid_), gtk3_label_remote_, 1, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(gtk3_grid_), gtk3_drawing_area_remote_, 1, 1, 1, 1);

        gtk_container_add(GTK_CONTAINER(gtk3_window_), gtk3_grid_);

        gtk_window_set_title (GTK_WINDOW (gtk3_window_), "PeerVideoClient: WebRTC Video Sample");
        gtk_window_set_default_size (GTK_WINDOW (gtk3_window_), 2*VIS_VIDEO_WIDTH, VIS_VIDEO_HEIGHT);
        gtk_widget_show_all (gtk3_window_);
    }

};

int main(int argc, char* argv[]) {

    auto logger = spdlog::stdout_color_mt("PeerVideo");
    absl::SetProgramUsageMessage(
      "Example usage: ./peer_video_client --server=localhost --port=5000\n");
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
    try {
        if (client->is_connected()) {
            GtkApplication *app;
            int status;

            app = gtk_application_new ("com.stereoboy.webrtc.example", G_APPLICATION_FLAGS_NONE);
            g_signal_connect (app, "activate", G_CALLBACK (PeerVideoClient::gtk3_window_activate_callback), client.get());
            status = g_application_run (G_APPLICATION (app), 0, nullptr);
            g_object_unref (app);
        }
    } catch (std::exception &e) {
        logger->error("Terminated by Interrupt: {} ", e.what());
    }

    rtc::CleanupSSL();
    client->deinit_signaling();

    logger->info("Stopped.");
    return 0;
}