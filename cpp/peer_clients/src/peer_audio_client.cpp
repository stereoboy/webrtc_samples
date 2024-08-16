#include <chrono>

#include <csignal>
#include <thread>

#include <sio_client.h>
// #include <asio.hpp>
// #include <asio/ssl.hpp>

#include <pulse/pulseaudio.h>

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

#define _(String) (String)

static inline const char *pa_yes_no(bool b) {
    return b ? "yes" : "no";
}

static inline const char *pa_yes_no_localised(bool b) {
    return b ? _("yes") : _("no");
}

static inline const char *pa_strnull(const char *x) {
    return x ? x : "(null)";
}

static inline const char *pa_strempty(const char *x) {
    return x ? x : "";
}

static inline const char *pa_strna(const char *x) {
    return x ? x : "n/a";
}


ABSL_FLAG(bool, list_devices,       false, "List Audio Devices only, No additional Test");
ABSL_FLAG(int,  playout_device_id,      -1,      "Playout Device ID (Speaker) to play");
ABSL_FLAG(int,  recording_device_id,    -1,      "Recording Device ID (Microphone) to record");

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

struct PASourceInfo {
    unsigned int    index;
    std::string     name;
    std::string     driver;
    std::string     sample_spec;
    std::string     state;
};

struct PASinkInfo {
    unsigned int    index;
    std::string     name;
    std::string     driver;
    std::string     sample_spec;
    std::string     state;
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
    rtc::scoped_refptr<webrtc::AudioDeviceModule> audio_device_module_ = nullptr;
    // std::unique_ptr<rtc::Thread> signaling_thread_;

    std::atomic_bool                    pa_context_initialized_ = {false};
    std::atomic_bool                    pa_operation_done_ = {false};
    pa_context                          *pa_context_ = nullptr;
    pa_threaded_mainloop                *pa_mainloop_ = nullptr;
    pa_mainloop_api                     *pa_mainloop_api_ = nullptr;
    bool                                pa_short_list_format_ = true;
    std::vector<struct PASourceInfo>    pa_source_list_ = {};
    std::vector<struct PASinkInfo>      pa_sink_list_ = {};

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

    void print_pa_source_list(void) {
        logger_->info("-[Source Info List]-----------------------------------------------------------------");
        for (int i = 0; i < pa_source_list_.size(); i++) {
            // logger_->info("  [{}] {} {} {} {}", i,  pa_source_list_[i].name,
            //                                         pa_source_list_[i].driver,
            //                                         pa_source_list_[i].sample_spec,
            //                                         pa_source_list_[i].state);
            logger_->info("  [{}] {}", i,  pa_source_list_[i].name);
        }
        logger_->info("------------------------------------------------------------------------------------");
    }

    void print_pa_sink_list(void) {
        logger_->info("-[Sink Info List]------------------------------------------------------------------");
        for (int i = 0; i < pa_sink_list_.size(); i++) {
            // logger_->info("  [{}] {} {} {} {}", i,  pa_sink_list_[i].name,
            //                                         pa_sink_list_[i].driver,
            //                                         pa_sink_list_[i].sample_spec,
            //                                         pa_sink_list_[i].state);
            logger_->info("  [{}] {}", i,  pa_sink_list_[i].name);
        }
        logger_->info("-----------------------------------------------------------------------------------");
    }

    static void simple_callback(pa_context *c, int success, void *userdata) {
        static_cast<PeerAudioClient *>(userdata)->simple_callback_handler(c, success);
    }

    void simple_callback_handler(pa_context *c, int success) {
        if (!success) {
            logger_->error("Failure: {}", pa_strerror(pa_context_errno(c)));
            pa_mainloop_api_->quit(pa_mainloop_api_, 1);
            return;
        }
    }

    static void context_drain_complete(pa_context *c, void *userdata) {
        pa_context_disconnect(c);
    }

    void drain(pa_context *c) {
        pa_operation *o;

        if (!(o = pa_context_drain(c, context_drain_complete, NULL)))
            pa_context_disconnect(c);
        else
            pa_operation_unref(o);
    }

    void complete_action(pa_context *c) {
        drain(c);
    }

    const char* get_available_str(int available) {
        switch (available) {
            case PA_PORT_AVAILABLE_UNKNOWN: return _("availability unknown");
            case PA_PORT_AVAILABLE_YES: return _("available");
            case PA_PORT_AVAILABLE_NO: return _("not available");
        }

        // pa_assert_not_reached();
        abort();
    }

    const char* get_device_port_type(unsigned int type) {
        static char buf[32];
        switch (type) {
        case PA_DEVICE_PORT_TYPE_UNKNOWN: return _("Unknown");
        case PA_DEVICE_PORT_TYPE_AUX: return _("Aux");
        case PA_DEVICE_PORT_TYPE_SPEAKER: return _("Speaker");
        case PA_DEVICE_PORT_TYPE_HEADPHONES: return _("Headphones");
        case PA_DEVICE_PORT_TYPE_LINE: return _("Line");
        case PA_DEVICE_PORT_TYPE_MIC: return _("Mic");
        case PA_DEVICE_PORT_TYPE_HEADSET: return _("Headset");
        case PA_DEVICE_PORT_TYPE_HANDSET: return _("Handset");
        case PA_DEVICE_PORT_TYPE_EARPIECE: return _("Earpiece");
        case PA_DEVICE_PORT_TYPE_SPDIF: return _("SPDIF");
        case PA_DEVICE_PORT_TYPE_HDMI: return _("HDMI");
        case PA_DEVICE_PORT_TYPE_TV: return _("TV");
        case PA_DEVICE_PORT_TYPE_RADIO: return _("Radio");
        case PA_DEVICE_PORT_TYPE_VIDEO: return _("Video");
        case PA_DEVICE_PORT_TYPE_USB: return _("USB");
        case PA_DEVICE_PORT_TYPE_BLUETOOTH: return _("Bluetooth");
        case PA_DEVICE_PORT_TYPE_PORTABLE: return _("Portable");
        case PA_DEVICE_PORT_TYPE_HANDSFREE: return _("Handsfree");
        case PA_DEVICE_PORT_TYPE_CAR: return _("Car");
        case PA_DEVICE_PORT_TYPE_HIFI: return _("HiFi");
        case PA_DEVICE_PORT_TYPE_PHONE: return _("Phone");
        case PA_DEVICE_PORT_TYPE_NETWORK: return _("Network");
        case PA_DEVICE_PORT_TYPE_ANALOG: return _("Analog");
        }
        snprintf(buf, sizeof(buf), "%s-%u", _("Unknown"), type);
        return buf;
    }

    static void get_server_info_callback(pa_context *c, const pa_server_info *i, void *userdata) {
        static_cast<PeerAudioClient *>(userdata)->get_server_info_callback_handler(c, i);
    }

    void get_server_info_callback_handler(pa_context *c, const pa_server_info *i) {
        char ss[PA_SAMPLE_SPEC_SNPRINT_MAX], cm[PA_CHANNEL_MAP_SNPRINT_MAX];

        if (!i) {
            logger_->error("Failed to get server information: {}", pa_strerror(pa_context_errno(c)));
            pa_mainloop_api_->quit(pa_mainloop_api_, 1);
            return;
        }

        pa_sample_spec_snprint(ss, sizeof(ss), &i->sample_spec);
        pa_channel_map_snprint(cm, sizeof(cm), &i->channel_map);

        {
            printf(_("Server String: %s\n"
                "Library Protocol Version: %u\n"
                "Server Protocol Version: %u\n"
                "Is Local: %s\n"
                "Client Index: %u\n"
                "Tile Size: %zu\n"),
                pa_context_get_server(c),
                pa_context_get_protocol_version(c),
                pa_context_get_server_protocol_version(c),
                pa_yes_no_localised(pa_context_is_local(c)),
                pa_context_get_index(c),
                pa_context_get_tile_size(c, NULL));

            printf(_("User Name: %s\n"
                    "Host Name: %s\n"
                    "Server Name: %s\n"
                    "Server Version: %s\n"
                    "Default Sample Specification: %s\n"
                    "Default Channel Map: %s\n"
                    "Default Sink: %s\n"
                    "Default Source: %s\n"
                    "Cookie: %04x:%04x\n"),
                i->user_name,
                i->host_name,
                i->server_name,
                i->server_version,
                ss,
                cm,
                i->default_sink_name,
                i->default_source_name,
                i->cookie >> 16,
                i->cookie & 0xFFFFU);
        }
        return;
    }

    static void get_source_info_callback(pa_context *c, const pa_source_info *i, int is_last, void *userdata) {
        static_cast<PeerAudioClient *>(userdata)->get_source_info_callback_handler(c, i, is_last);
    }

    void get_source_info_callback_handler(pa_context *c, const pa_source_info *i, int is_last) {

        static const char *state_table[] = {
            [1+PA_SOURCE_INVALID_STATE] = "n/a",
            [1+PA_SOURCE_RUNNING] = "RUNNING",
            [1+PA_SOURCE_IDLE] = "IDLE",
            [1+PA_SOURCE_SUSPENDED] = "SUSPENDED"
        };

        char
            s[PA_SAMPLE_SPEC_SNPRINT_MAX],
            cv[PA_CVOLUME_SNPRINT_VERBOSE_MAX],
            v[PA_VOLUME_SNPRINT_VERBOSE_MAX],
            cm[PA_CHANNEL_MAP_SNPRINT_MAX],
            f[PA_FORMAT_INFO_SNPRINT_MAX];
        char *pl;

        if (is_last < 0) {
            logger_->error("Failed to get source information: {}"), pa_strerror(pa_context_errno(c));
            pa_mainloop_api_->quit(pa_mainloop_api_, 1);
            return;
        }

        if (is_last) {
            printf(_(" [ END ]\n"));
            // complete_action(pa_context_);
            pa_threaded_mainloop_signal(pa_mainloop_, 0);
            return;
        }

        assert(i);

        // if (nl && !short_list_format && format == TEXT)
        //     printf("\n");
        // nl = true;

        char *sample_spec = pa_sample_spec_snprint(s, sizeof(s), &i->sample_spec);
        pa_source_list_.push_back({i->index, i->name, pa_strnull(i->driver), sample_spec, state_table[1+i->state]});
        if (pa_short_list_format_) {
            {
                printf(" %3u %-80s\t%s\t%s\t%s\n",
                i->index,
                i->name,
                pa_strnull(i->driver),
                sample_spec,
                state_table[1+i->state]);
            }
            return;
        }

        char *channel_map = pa_channel_map_snprint(cm, sizeof(cm), &i->channel_map);
        float volume_balance = pa_cvolume_get_balance(&i->volume, &i->channel_map);

        {
            printf(_("Source #%u\n"
                    "\tState: %s\n"
                    "\tName: %s\n"
                    "\tDescription: %s\n"
                    "\tDriver: %s\n"
                    "\tSample Specification: %s\n"
                    "\tChannel Map: %s\n"
                    "\tOwner Module: %u\n"
                    "\tMute: %s\n"
                    "\tVolume: %s\n"
                    "\t        balance %0.2f\n"
                    "\tBase Volume: %s\n"
                    "\tMonitor of Sink: %s\n"
                    "\tLatency: %0.0f usec, configured %0.0f usec\n"
                    "\tFlags: %s%s%s%s%s%s\n"
                    "\tProperties:\n\t\t%s\n"),
                i->index,
                state_table[1+i->state],
                i->name,
                pa_strnull(i->description),
                pa_strnull(i->driver),
                sample_spec,
                channel_map,
                i->owner_module,
                pa_yes_no_localised(i->mute),
                pa_cvolume_snprint_verbose(cv, sizeof(cv), &i->volume, &i->channel_map, i->flags & PA_SOURCE_DECIBEL_VOLUME),
                volume_balance,
                pa_volume_snprint_verbose(v, sizeof(v), i->base_volume, i->flags & PA_SOURCE_DECIBEL_VOLUME),
                i->monitor_of_sink_name ? i->monitor_of_sink_name : _("n/a"),
                (double) i->latency, (double) i->configured_latency,
                i->flags & PA_SOURCE_HARDWARE ? "HARDWARE " : "",
                i->flags & PA_SOURCE_NETWORK ? "NETWORK " : "",
                i->flags & PA_SOURCE_HW_MUTE_CTRL ? "HW_MUTE_CTRL " : "",
                i->flags & PA_SOURCE_HW_VOLUME_CTRL ? "HW_VOLUME_CTRL " : "",
                i->flags & PA_SOURCE_DECIBEL_VOLUME ? "DECIBEL_VOLUME " : "",
                i->flags & PA_SOURCE_LATENCY ? "LATENCY " : "",
                pl = pa_proplist_to_string_sep(i->proplist, "\n\t\t"));

            if (i->ports) {
                pa_source_port_info **p;

                printf(_("\tPorts:\n"));
                for (p = i->ports; *p; p++)
                    printf(_("\t\t%s: %s (type: %s, priority: %u%s%s, %s)\n"),
                            (*p)->name, (*p)->description, get_device_port_type((*p)->type),
                            (*p)->priority, (*p)->availability_group ? _(", availability group: ") : "",
                            (*p)->availability_group ?: "", get_available_str((*p)->available));
            }

            if (i->active_port)
                printf(_("\tActive Port: %s\n"),
                    i->active_port->name);

            if (i->formats) {
                uint8_t j;

                printf(_("\tFormats:\n"));
                for (j = 0; j < i->n_formats; j++)
                    printf("\t\t%s\n", pa_format_info_snprint(f, sizeof(f), i->formats[j]));
            }
        }

        pa_xfree(pl);
    }

    static void get_sink_info_callback(pa_context *c, const pa_sink_info *i, int is_last, void *userdata) {
        static_cast<PeerAudioClient *>(userdata)->get_sink_info_callback_handler(c, i, is_last);
    }

    void get_sink_info_callback_handler(pa_context *c, const pa_sink_info *i, int is_last) {

        static const char *state_table[] = {
            [1+PA_SINK_INVALID_STATE] = "n/a",
            [1+PA_SINK_RUNNING] = "RUNNING",
            [1+PA_SINK_IDLE] = "IDLE",
            [1+PA_SINK_SUSPENDED] = "SUSPENDED"
        };

        char
            s[PA_SAMPLE_SPEC_SNPRINT_MAX],
            cv[PA_CVOLUME_SNPRINT_VERBOSE_MAX],
            v[PA_VOLUME_SNPRINT_VERBOSE_MAX],
            cm[PA_CHANNEL_MAP_SNPRINT_MAX],
            f[PA_FORMAT_INFO_SNPRINT_MAX];
        char *pl;

        if (is_last < 0) {
            logger_->error("Failed to get source information: {}"), pa_strerror(pa_context_errno(c));
            pa_mainloop_api_->quit(pa_mainloop_api_, 1);
            return;
        }

        if (is_last) {
            printf(_(" [ END ]\n"));
            // complete_action(pa_context_);
            pa_threaded_mainloop_signal(pa_mainloop_, 0);
            return;
        }

        assert(i);

        // if (nl && !short_list_format && format == TEXT)
        //     printf("\n");
        // nl = true;

        char *sample_spec = pa_sample_spec_snprint(s, sizeof(s), &i->sample_spec);
        pa_sink_list_.push_back({i->index, i->name, pa_strnull(i->driver), sample_spec, state_table[1+i->state]});
        if (pa_short_list_format_) {
            {
                printf(" %3u %-80s\t%s\t%s\t%s\n",
                i->index,
                i->name,
                pa_strnull(i->driver),
                sample_spec,
                state_table[1+i->state]);
            }
            return;
        }

        char *channel_map = pa_channel_map_snprint(cm, sizeof(cm), &i->channel_map);
        float volume_balance = pa_cvolume_get_balance(&i->volume, &i->channel_map);

        {
            printf(_("Sink #%u\n"
                "\tState: %s\n"
                "\tName: %s\n"
                "\tDescription: %s\n"
                "\tDriver: %s\n"
                "\tSample Specification: %s\n"
                "\tChannel Map: %s\n"
                "\tOwner Module: %u\n"
                "\tMute: %s\n"
                "\tVolume: %s\n"
                "\t        balance %0.2f\n"
                "\tBase Volume: %s\n"
                "\tMonitor Source: %s\n"
                "\tLatency: %0.0f usec, configured %0.0f usec\n"
                "\tFlags: %s%s%s%s%s%s%s\n"
                "\tProperties:\n\t\t%s\n"),
            i->index,
            state_table[1+i->state],
            i->name,
            pa_strnull(i->description),
            pa_strnull(i->driver),
            sample_spec,
            channel_map,
            i->owner_module,
            pa_yes_no_localised(i->mute),
            pa_cvolume_snprint_verbose(cv, sizeof(cv), &i->volume, &i->channel_map, i->flags & PA_SINK_DECIBEL_VOLUME),
            volume_balance,
            pa_volume_snprint_verbose(v, sizeof(v), i->base_volume, i->flags & PA_SINK_DECIBEL_VOLUME),
            pa_strnull(i->monitor_source_name),
            (double) i->latency, (double) i->configured_latency,
            i->flags & PA_SINK_HARDWARE ? "HARDWARE " : "",
            i->flags & PA_SINK_NETWORK ? "NETWORK " : "",
            i->flags & PA_SINK_HW_MUTE_CTRL ? "HW_MUTE_CTRL " : "",
            i->flags & PA_SINK_HW_VOLUME_CTRL ? "HW_VOLUME_CTRL " : "",
            i->flags & PA_SINK_DECIBEL_VOLUME ? "DECIBEL_VOLUME " : "",
            i->flags & PA_SINK_LATENCY ? "LATENCY " : "",
            i->flags & PA_SINK_SET_FORMATS ? "SET_FORMATS " : "",
            pl = pa_proplist_to_string_sep(i->proplist, "\n\t\t"));

            if (i->ports) {
                pa_sink_port_info **p;

                printf(_("\tPorts:\n"));
                for (p = i->ports; *p; p++)
                    printf(_("\t\t%s: %s (type: %s, priority: %u%s%s, %s)\n"),
                            (*p)->name, (*p)->description, get_device_port_type((*p)->type),
                            (*p)->priority, (*p)->availability_group ? _(", availability group: ") : "",
                            (*p)->availability_group ?: "", get_available_str((*p)->available));
            }

            if (i->active_port)
                printf(_("\tActive Port: %s\n"),
                    i->active_port->name);

            if (i->formats) {
                uint8_t j;

                printf(_("\tFormats:\n"));
                for (j = 0; j < i->n_formats; j++)
                    printf("\t\t%s\n", pa_format_info_snprint(f, sizeof(f), i->formats[j]));
            }
        }

        pa_xfree(pl);
    }

    static void context_state_callback(pa_context *c, void *userdata) {
        static_cast<PeerAudioClient *>(userdata)->context_state_callback_handler(c);
    }

    void context_state_callback_handler(pa_context *c) {
        pa_operation *o = NULL;

        assert(c);

        switch (pa_context_get_state(c)) {
            case PA_CONTEXT_CONNECTING:
            case PA_CONTEXT_AUTHORIZING:
            case PA_CONTEXT_SETTING_NAME:
                break;

            case PA_CONTEXT_READY:
                logger_->info("pa_context is ready");

                // o = pa_context_get_source_info_list(c, get_source_info_callback, this);
                // if (o) {
                //     pa_operation_unref(o);
                // }
                pa_context_initialized_ = true;
                break;

            case PA_CONTEXT_TERMINATED:
                logger_->info("pa_context is terminated");
                pa_context_initialized_ = true;
                pa_mainloop_api_->quit(pa_mainloop_api_, 0);
                break;

            case PA_CONTEXT_FAILED:
            default:
                pa_context_initialized_ = true;
                logger_->error("Connection failure: {}", pa_strerror(pa_context_errno(c)));
                pa_mainloop_api_->quit(pa_mainloop_api_, 1);
        }
    }

    void list_pa_audio_devices(void) {
#if 0
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
        if (audio_device_module->Terminate() < 0) {
            logger_->error("Failed to terminate the audio device module");
            return;
        }
#else

        logger_->info("Starting to list pulse audio devices.");
        pa_operation *o = NULL;
        static pa_proplist *proplist = NULL;
        int ret = 1, c;
        char *server = NULL;

        pa_source_list_.clear();
        pa_sink_list_.clear();

        proplist = pa_proplist_new();

        if (pa_proplist_sets(proplist, PA_PROP_APPLICATION_NAME, "peer_audio_client") < 0) {
            logger_->error("pa_proplist_sets() failed");
            goto quit;
        }

        if (!(pa_mainloop_ = pa_threaded_mainloop_new())) {
            logger_->error("pa_mainloop_new() failed");
            goto quit;
        }

        if (pa_threaded_mainloop_start(pa_mainloop_) != PA_OK) {
            logger_->error("pa_threaded_mainloop_start() failed");
            goto quit;
        }

        pa_mainloop_api_ = pa_threaded_mainloop_get_api(pa_mainloop_);

        if (pa_signal_init(pa_mainloop_api_) < 0) {
            logger_->error("pa_signal_init() failed");
            goto quit;
        }

        if (!(pa_context_ = pa_context_new_with_proplist(pa_mainloop_api_, NULL, proplist))) {
            logger_->error("pa_context_new() failed.");
            goto quit;
        }
        logger_->info("pa_constext created successfully.");

        pa_context_set_state_callback(pa_context_, PeerAudioClient::context_state_callback, this);
        if (pa_context_connect(pa_context_, server, PA_CONTEXT_NOFLAGS, NULL) < 0) {
            logger_->error("pa_context_connect() failed: %s"), pa_strerror(pa_context_errno(pa_context_));
            goto quit;
        }
        logger_->info("connnect to server successfully.");

        while (!pa_context_initialized_) {
            pa_threaded_mainloop_wait(pa_mainloop_);
        }

        logger_->info("-[Server Info List]-----------------------------------------------------------------");
        o = pa_context_get_server_info(pa_context_, get_server_info_callback, this);
        if (!o) {
            logger_->error("pa_context_get_server_info failed");
            goto quit;
        }
        while (pa_operation_get_state(o) == PA_OPERATION_RUNNING) {
            pa_threaded_mainloop_wait(pa_mainloop_);
        }
        pa_operation_unref(o);

        o = pa_context_get_source_info_list(pa_context_, get_source_info_callback, this);
        if (!o) {
            logger_->error("pa_context_get_source_info_list failed");
            goto quit;
        }
        while (pa_operation_get_state(o) == PA_OPERATION_RUNNING) {
            pa_threaded_mainloop_wait(pa_mainloop_);
        }
        pa_operation_unref(o);

        print_pa_source_list();

        o = pa_context_get_sink_info_list(pa_context_, get_sink_info_callback, this);
        if (!o) {
            logger_->error("pa_context_get_sink_info_list failed");
            goto quit;
        }
        while (pa_operation_get_state(o) == PA_OPERATION_RUNNING) {
            pa_threaded_mainloop_wait(pa_mainloop_);
        }
        pa_operation_unref(o);

        print_pa_sink_list();

        // if (pa_mainloop_run(pa_mainloop_, &ret) < 0) {
        //     logger_->error("pa_mainloop_run() failed.");
        //     goto quit;
        // }

        drain(pa_context_);

    quit:
        if (pa_context_)
            pa_context_unref(pa_context_);

        if (pa_mainloop_) {
            pa_signal_done();
            pa_threaded_mainloop_free(pa_mainloop_);
        }

        pa_xfree(server);

        if (proplist)
            pa_proplist_free(proplist);
#endif
    }

    int init_pa_audio_devices(int playout_device_id, int recording_device_id) {
        logger_->info("init_pa_audio_devices({}, {})", playout_device_id, recording_device_id);

#if 0
        audio_device_module_ =
            webrtc::AudioDeviceModule::Create(webrtc::AudioDeviceModule::kPlatformDefaultAudio, webrtc::CreateDefaultTaskQueueFactory(nullptr).get());

        if (audio_device_module_->Init() < 0) {
            logger_->error("Failed to initialize the audio device module");
            return -1;
        }


        {
            if (audio_device_module_->SetPlayoutDevice(playout_device_id) != 0) {
                logger_->error("Unable to set playout device.");
                return -1;
            }
            if (audio_device_module_->InitSpeaker() != 0) {
                logger_->error("Unable to access speaker.");
            }

            // Set number of channels
            bool available = false;
            if (audio_device_module_->StereoPlayoutIsAvailable(&available) != 0) {
                logger_->error("Failed to query stereo playout.");
            }
            if (audio_device_module_->SetStereoPlayout(available) != 0) {
                logger_->error("Failed to set stereo playout mode.");
            }
            int16_t num_devices = audio_device_module_->PlayoutDevices();
            logger_->info("Playout Devices: {}", num_devices);

            for (int16_t i = 0; i < num_devices; ++i) {
                char name[webrtc::kAdmMaxDeviceNameSize];
                char guid[webrtc::kAdmMaxGuidSize];
                audio_device_module_->PlayoutDeviceName(i, name, guid);
                logger_->info("\t[{}] {} (guid={})", i, name, guid);
            }
        }
        {
            if (audio_device_module_->SetRecordingDevice(recording_device_id) != 0) {
                logger_->error("Unable to set recording device.");
                return -1;
            }
            if (audio_device_module_->InitMicrophone() != 0) {
                logger_->error("Unable to access microphone.");
            }

            // Set number of channels
            bool available = false;
            if (audio_device_module_->StereoRecordingIsAvailable(&available) != 0) {
                logger_->error("Failed to query stereo recording.");
            }
            if (audio_device_module_->SetStereoRecording(available) != 0) {
                logger_->error("Failed to set stereo recording mode.");
            }
            int16_t num_devices = audio_device_module_->RecordingDevices();
            logger_->info("Recording Devices: {}", num_devices);

            for (int16_t i = 0; i < num_devices; ++i) {
                char name[webrtc::kAdmMaxDeviceNameSize];
                char guid[webrtc::kAdmMaxGuidSize];
                audio_device_module_->RecordingDeviceName(i, name, guid);
                logger_->info("\t[{}] {} (guid={})", i, name, guid);
            }
        }
        // if (audio_device_module_->Terminate() < 0) {
        //     logger_->error("Failed to terminate the audio device module");
        //     return -1;
        // }

        return 0;
#else
        logger_->info("Starting to init pulse audio devices.");
        pa_operation *o = NULL;
        static pa_proplist *proplist = NULL;
        int ret = 1, c;
        char *server = NULL;

        pa_source_list_.clear();
        pa_sink_list_.clear();

        proplist = pa_proplist_new();

        if (pa_proplist_sets(proplist, PA_PROP_APPLICATION_NAME, "peer_audio_client") < 0) {
            logger_->error("pa_proplist_sets() failed");
            goto quit;
        }

        if (!(pa_mainloop_ = pa_threaded_mainloop_new())) {
            logger_->error("pa_mainloop_new() failed");
            goto quit;
        }

        if (pa_threaded_mainloop_start(pa_mainloop_) != PA_OK) {
            logger_->error("pa_threaded_mainloop_start() failed");
            goto quit;
        }

        pa_mainloop_api_ = pa_threaded_mainloop_get_api(pa_mainloop_);

        if (pa_signal_init(pa_mainloop_api_) < 0) {
            logger_->error("pa_signal_init() failed");
            goto quit;
        }

        if (!(pa_context_ = pa_context_new_with_proplist(pa_mainloop_api_, NULL, proplist))) {
            logger_->error("pa_context_new() failed.");
            goto quit;
        }
        logger_->info("pa_constext created successfully.");

        pa_context_set_state_callback(pa_context_, PeerAudioClient::context_state_callback, this);
        if (pa_context_connect(pa_context_, server, PA_CONTEXT_NOFLAGS, NULL) < 0) {
            logger_->error("pa_context_connect() failed: %s"), pa_strerror(pa_context_errno(pa_context_));
            goto quit;
        }
        logger_->info("connnect to server successfully.");

        while (!pa_context_initialized_) {
            pa_threaded_mainloop_wait(pa_mainloop_);
        }

        if (recording_device_id != -1) {
            o = pa_context_get_source_info_list(pa_context_, get_source_info_callback, this);
            if (!o) {
                logger_->error("pa_context_get_source_info_list failed");
                goto quit;
            }
            while (pa_operation_get_state(o) == PA_OPERATION_RUNNING) {
                pa_threaded_mainloop_wait(pa_mainloop_);
            }
            pa_operation_unref(o);

            print_pa_source_list();

            if (recording_device_id >= pa_source_list_.size()) {
                logger_->error("Invalid recording device id: {}", recording_device_id);
                goto quit;
            }

            logger_->info("Set default source as <{}>", pa_source_list_[recording_device_id].name);
            o = pa_context_set_default_source(pa_context_, pa_source_list_[recording_device_id].name.c_str(), simple_callback, this);
            if (!o) {
                logger_->error("pa_context_get_source_info_list failed");
                goto quit;
            }
            while (pa_operation_get_state(o) == PA_OPERATION_RUNNING) {
                pa_threaded_mainloop_wait(pa_mainloop_);
            }
            pa_operation_unref(o);
        }

        if (playout_device_id != -1) {
            o = pa_context_get_sink_info_list(pa_context_, get_sink_info_callback, this);
            if (!o) {
                logger_->error("pa_context_get_source_info_list failed");
                goto quit;
            }
            while (pa_operation_get_state(o) == PA_OPERATION_RUNNING) {
                pa_threaded_mainloop_wait(pa_mainloop_);
            }
            pa_operation_unref(o);

            print_pa_sink_list();

            if (playout_device_id >= pa_sink_list_.size()) {
                logger_->error("Invalid playout device id: {}", playout_device_id);
                goto quit;
            }

            logger_->info("Set default sink as <{}>", pa_sink_list_[playout_device_id].name);
            o = pa_context_set_default_sink(pa_context_, pa_sink_list_[playout_device_id].name.c_str(), simple_callback, this);
            if (!o) {
                logger_->error("pa_context_set_default_sink failed");
                goto quit;
            }
            while (pa_operation_get_state(o) == PA_OPERATION_RUNNING) {
                pa_threaded_mainloop_wait(pa_mainloop_);
            }
            pa_operation_unref(o);
        }



        // if (pa_mainloop_run(pa_mainloop_, &ret) < 0) {
        //     logger_->error("pa_mainloop_run() failed.");
        //     goto quit;
        // }

        drain(pa_context_);

    quit:
        if (pa_context_)
            pa_context_unref(pa_context_);

        if (pa_mainloop_) {
            pa_signal_done();
            pa_threaded_mainloop_free(pa_mainloop_);
        }

        pa_xfree(server);

        if (proplist)
            pa_proplist_free(proplist);

        return 0;
#endif
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
            signaling_thread_.get(), audio_device_module_ /* default_adm */,
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
        client->list_pa_audio_devices();
        return 0;
    }

    if (absl::GetFlag(FLAGS_playout_device_id) != -1 || absl::GetFlag(FLAGS_recording_device_id) != -1) {
        if (client->init_pa_audio_devices(absl::GetFlag(FLAGS_playout_device_id), absl::GetFlag(FLAGS_recording_device_id)) < 0)
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