//
// Created by wom on 24. 10. 14..
//

#include <cstring>
#include <jni.h>
#include <cinttypes>
#include <android/log.h>

#include <chrono>

//#include <csignal>
#include <memory>
#include <thread>

#include <sio_client.h>
//#include <memory>
// #include <asio.hpp>
// #include <asio/ssl.hpp>

#include "logging.h"
#include "peer_client.h"

#include <modules/utility/include/jvm_android.h>
#include <sdk/android/native_api/base/init.h>

#include <sdk/android/native_api/audio_device_module/audio_device_android.h>
#include <sdk/android/native_api/jni/java_types.h>
#include <sdk/android/native_api/jni/jvm.h>
#include <sdk/android/native_api/jni/scoped_java_ref.h>


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

#include "logging.h"
#include "peer_client.h"

#define TAG             "PeerAudio"

#define SERVER_HOSTNAME "192.168.0.2"
#define SERVER_PORT     5000

#define _(String) (String)

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

    bool                                pa_short_list_format_ = true;

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
        LOGI(name_.c_str(), "PeerAudioClient deleted");
    }

    //
    // PeerConnectionObserver implementation.
    //
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override {
        LOGI(name_.c_str(), "PeerConnectionInterface::OnSignalingChange: %s", webrtc::PeerConnectionInterface::AsString(new_state));
    }

    void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                    const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> &streams) override {
        LOGI(name_.c_str(), "PeerConnectionInterface::OnAddTrack: %s", receiver->id().c_str());
    }

    void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override {
        LOGI(name_.c_str(), "PeerConnectionInterface::OnRemoveTrack: %s", receiver->id().c_str());
    }

    void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {
        LOGI(name_.c_str(), "PeerConnectionInterface::OnAddStream: %s", stream.get()->id().c_str());
    };

    void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {
        LOGI(name_.c_str(), "PeerConnectionObserver::RemoveStream: %s", stream.get()->id().c_str());
    };

    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {
        LOGI(name_.c_str(), "PeerConnectionInterface::OnDataChannel: %s", channel->label().c_str());
    }
    void OnRenegotiationNeeded() override {
        LOGI(name_.c_str(), "PeerConnectionInterface::OnRenegotiationNeeded");
    }

    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override {
        LOGI(name_.c_str(), "PeerConnectionInterface::OnIceConnectionChange: %s", webrtc::PeerConnectionInterface::AsString(new_state));
    }

    virtual void OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState new_state) override {
        LOGI(name_.c_str(), "PeerConnectionInterface::OnConnectionChange: %s", webrtc::PeerConnectionInterface::AsString(new_state));
    }

    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override {
        LOGI(name_.c_str(), "PeerConnectionInterface::OnIceGatheringChange: %s", webrtc::PeerConnectionInterface::AsString(new_state));
    }

    void OnIceCandidate(const webrtc::IceCandidateInterface *candidate) override {

        LOGI(name_.c_str(), "PeerConnectionInterface::OnIceCandidate: %s, %d", candidate->sdp_mid().c_str(), candidate->sdp_mline_index());
        std::string candidate_str;
        candidate->ToString(&candidate_str);
        LOGI(name_.c_str(), "\t- %s", candidate_str.c_str());

        sio::message::ptr msg = sio::object_message::create();
        msg->get_map()["sdp_mid"] = sio::string_message::create(candidate->sdp_mid());
        msg->get_map()["sdp_mline_index"] = sio::int_message::create(candidate->sdp_mline_index());
        msg->get_map()["candidate"] = sio::string_message::create(candidate_str);
        sio_client_.socket()->emit("ice", msg, [&](sio::message::list const& msg) {
            std::unique_lock<std::mutex> lock(msg_mutex_);
            bool ok = msg[0]->get_map()["ok"]->get_bool();
            std::string message = msg[0]->get_map()["message"]->get_string();
            if (ok) {
                LOGI(name_.c_str(), "ICE Candidate sent successfully");
            } else {
                LOGE(name_.c_str(), "ICE Candidate sent failed to send: %s", message.c_str());
            }
        });

    };

    //
    // CreateSessionDescriptionObserver implementation
    //
    void OnSuccess(webrtc::SessionDescriptionInterface *desc) override {
        LOGI(name_.c_str(), "CreateSessionDescriptionObserver::OnSuccess({})");
        peer_connection_->SetLocalDescription(DummySetSessionDescriptionObserver::Create().get(), desc);

        std::string sdp_str;
        desc->ToString(&sdp_str);
        LOGI(name_.c_str(), "SDP:\n%s", sdp_str.c_str());

        sio::message::ptr msg = sio::string_message::create(sdp_str);

        if (type_ == PeerClient::PeerType::Caller) {
            sio_client_.socket()->emit("offer", msg, [&](sio::message::list const& msg) {
                std::unique_lock<std::mutex> lock(msg_mutex_);
                bool ok = msg[0]->get_map()["ok"]->get_bool();
                std::string message = msg[0]->get_map()["message"]->get_string();
                if (ok) {
                    LOGI(name_.c_str(), "Offer SDP sent successfully");
                } else {
                    LOGE(name_.c_str(), "Offer SDP failed to send: %s", message.c_str());
                }
            });
        } else {
            sio_client_.socket()->emit("answer", msg, [&](sio::message::list const& msg) {
                std::unique_lock<std::mutex> lock(msg_mutex_);
                bool ok = msg[0]->get_map()["ok"]->get_bool();
                std::string message = msg[0]->get_map()["message"]->get_string();
                if (ok) {
                    LOGI(name_.c_str(), "Answer SDP sent successfully");
                } else {
                    LOGE(name_.c_str(), "Answer SDP failed to send: %s", message.c_str());
                }
            });
        }
    };

    void OnFailure(webrtc::RTCError error) override {
        LOGI(name_.c_str(), "CreateSessionDescriptionObserver::OnFailure(%s)", error.message());
    };

    //
    //
    //
    virtual void init_signaling(void) override {
        PeerClient::init_signaling();

        sio_client_.socket()->on("set-as-caller", sio::socket::event_listener_aux([&](std::string const& name, sio::message::ptr const& data, bool isAck, sio::message::list &ack_resp)
                                                                                  {
                                                                                      std::unique_lock<std::mutex> lock(msg_mutex_);
                                                                                      LOGI(name_.c_str(), "event=%s", name.c_str());
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
                                                                                      LOGI(name_.c_str(), "event=%s", name.c_str());
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
                                                                              LOGI(name_.c_str(), "event=%s", name.c_str());
                                                                              if (type_ == PeerClient::PeerType::Caller) {
                                                                                  if (!create_offer()) {
                                                                                      LOGE(name_.c_str(), "Failed to create Offer");
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
                                                                              LOGI(name_.c_str(), "event=%s", name.c_str());
                                                                              if (type_ == PeerClient::PeerType::Callee) {
                                                                                  std::string sdp_str = data->get_string();
                                                                                  LOGI(name_.c_str(), "received Offer SDP:\n%s", sdp_str.c_str());
                                                                                  if (!receive_offer_create_answer(sdp_str)){
                                                                                      LOGE(name_.c_str(), "Failed to receive/create Answer");
                                                                                      sio::message::ptr resp = sio::object_message::create();
                                                                                      resp->get_map()["ok"] = sio::bool_message::create(false);
                                                                                      resp->get_map()["message"] = sio::string_message::create("Failed to receive/create Answer");
                                                                                      ack_resp.push(resp);
                                                                                      return;
                                                                                  }
                                                                              } else {
                                                                                  LOGE(name_.c_str(), "Unexpected Offer");
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
                                                                               LOGI(name_.c_str(), "event=%s", name.c_str());
                                                                               if (type_ == PeerClient::PeerType::Caller) {
                                                                                   std::string sdp_str = data->get_string();
                                                                                   LOGI(name_.c_str(), "received Answer SDP:\n%s", sdp_str.c_str());
                                                                                   if (!receive_answer(sdp_str)) {
                                                                                       LOGE(name_.c_str(), "Failed to receive Answer");
                                                                                       sio::message::ptr resp = sio::object_message::create();
                                                                                       resp->get_map()["ok"] = sio::bool_message::create(false);
                                                                                       resp->get_map()["message"] = sio::string_message::create("Failed to receive Answer");
                                                                                       ack_resp.push(resp);
                                                                                       return;
                                                                                   }
                                                                               } else {
                                                                                   LOGE(name_.c_str(), "Unexpected Answer");
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
                                                                            LOGI(name_.c_str(), "event=%s", name.c_str());
                                                                            std::string sdp_mid = data->get_map()["sdp_mid"]->get_string();
                                                                            int sdp_mline_index = data->get_map()["sdp_mline_index"]->get_int();
                                                                            std::string candidate = data->get_map()["candidate"]->get_string();

                                                                            std::string err_message;
                                                                            if (add_ice_candidate(sdp_mid, sdp_mline_index, candidate, err_message)) {
                                                                                LOGI(name_.c_str(), "ICE Candidate added successfully");
                                                                                sio::message::ptr resp = sio::object_message::create();
                                                                                resp->get_map()["ok"] = sio::bool_message::create(true);
                                                                                resp->get_map()["message"] = sio::string_message::create("accepted");
                                                                                ack_resp.push(resp);
                                                                            } else {
                                                                                LOGE(name_.c_str(), "Failed to add ICE Candidate");
                                                                                sio::message::ptr resp = sio::object_message::create();
                                                                                resp->get_map()["ok"] = sio::bool_message::create(false);
                                                                                resp->get_map()["message"] = sio::string_message::create(err_message);
                                                                                ack_resp.push(resp);
                                                                            }
                                                                        }
        ));
    }

    void query_peer_type(void) {
        LOGI(name_.c_str(), ">>> query-peer-type");
        std::string msg = "dummy";
        sio_client_.socket()->emit("query-peer-type", msg, [&](sio::message::list const& msg) {
            // std::unique_lock<std::mutex> lock(msg_mutex_);
            std::string res = msg[0]->get_string();
            if (res.compare("caller") == 0) {
                type_ = PeerClient::PeerType::Caller;
                LOGI(name_.c_str(), "caller");
            } else if (res.compare("callee") == 0) {
                type_ = PeerClient::PeerType::Callee;
                LOGI(name_.c_str(), "callee");
            }
            msg_cond_.notify_all();
        });

        std::unique_lock<std::mutex> lock(msg_mutex_);
        msg_cond_.wait(msg_mutex_);
        LOGI(name_.c_str(), "<<< query-peer-type");
    }

    bool init_webrtc(JNIEnv* env, jobject application_context) {
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

        //
        // references
        //  - https://groups.google.com/g/discuss-webrtc/c/O_Mbam2GVHU
        //
        audio_device_module_ = webrtc::CreateJavaAudioDeviceModule(env, application_context);

        peer_connection_factory_ = webrtc::CreatePeerConnectionFactory(
                nullptr /* network_thread */, nullptr /* worker_thread */,
                signaling_thread_.get(), audio_device_module_ /* default_adm */,
                webrtc::CreateBuiltinAudioEncoderFactory(),
                webrtc::CreateBuiltinAudioDecoderFactory(),
                nullptr /* video_encoder_factory */, nullptr /* video_decoder_factory */,
                nullptr /* audio_mixer */, nullptr /* audio_processing */);
        LOGI(name_.c_str(), "PeerConnectionFactory created successfully.");
#endif

        webrtc::PeerConnectionDependencies pc_dependencies(this);
        auto error_or_peer_connection = peer_connection_factory_->CreatePeerConnectionOrError(configuration_, std::move(pc_dependencies));
        peer_connection_factory_->SetOptions(webrtc::PeerConnectionFactoryInterface::Options());
        if (error_or_peer_connection.ok()) {
            peer_connection_ = std::move(error_or_peer_connection.value());;
            LOGI(name_.c_str(), "peer_connection_ created successfully.");
        } else {
            LOGE(name_.c_str(), "Failed to create PeerConnection: %s", error_or_peer_connection.error().message());
            return false;
        }

        if (!peer_connection_->GetSenders().empty()) {
            return false;  // Already added tracks.
        }

        cricket::AudioOptions audio_options;
        LOGI(name_.c_str(), "audio_options=%s", audio_options.ToString().c_str());
        rtc::scoped_refptr<webrtc::AudioSourceInterface> audio_source = peer_connection_factory_->CreateAudioSource(audio_options);
        rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
                peer_connection_factory_->CreateAudioTrack("audio_label", audio_source.get())
        );
        auto result_or_error = peer_connection_->AddTrack(audio_track, {"stream_id"});
        if (!result_or_error.ok()) {
            LOGE(name_.c_str(), "Failed to add audio track to PeerConnection: %s", result_or_error.error().message());
            return false;
        }

        LOGI(name_.c_str(), "Add audio track successfully.");
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
            LOGE(name_.c_str(), "Failed to create session description: %s", error.description.c_str());
            return false;
        }
        peer_connection_->SetRemoteDescription(DummySetSessionDescriptionObserver::Create().get(), session_description.release());
        return true;
    }

    bool receive_offer_create_answer(const std::string &sdp_str) {
        webrtc::SdpParseError error;
        std::unique_ptr<webrtc::SessionDescriptionInterface> session_description = webrtc::CreateSessionDescription(webrtc::SdpType::kOffer, sdp_str, &error);
        if (session_description == nullptr) {
            LOGE(name_.c_str(), "Failed to create session description: %s", error.description.c_str());
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
            LOGE(name_.c_str(), "Failed to create ICE Candidate: %s", error.description.c_str());
            err_message = error.description;
            return false;
        }
        if (!peer_connection_->AddIceCandidate(ice_candidate.get())) {
            LOGE(name_.c_str(), "Failed to add ICE candidate");
            err_message = "Failed to add ICE candidate";
            return false;
        }
        LOGI(name_.c_str(), "Added ICE Candidate: %s", candidate.c_str());
        return true;
    }
};

static void *app_thread_func(void *userdata);
struct AppContext {
    JavaVM              *jvm;
    jobject             application_context = nullptr;
    pthread_t           app_thread;
    std::atomic_bool    system_on = {false};
};

static AppContext g_ctx;

extern "C" JNIEXPORT void JNICALL
Java_com_stereoboy_peer_1audio_1client_MainActivity_initNative(JNIEnv *env, jobject thiz, jobject application_context) {
    LOGI(TAG, "%s", __PRETTY_FUNCTION__ );

    g_ctx.system_on = true;
    g_ctx.application_context = env->NewGlobalRef(application_context);

    pthread_create(&g_ctx.app_thread, nullptr, app_thread_func, nullptr);
    return;
}

extern "C" JNIEXPORT void JNICALL
Java_com_stereoboy_peer_1audio_1client_MainActivity_deinitNative(JNIEnv *env, jobject thiz) {
    LOGI(TAG, "%s", __PRETTY_FUNCTION__ );
    g_ctx.system_on = false;
    void *ret;
    pthread_join(g_ctx.app_thread, &ret);

    LOGI(TAG, "PeerAudio Stopped with ret = %d", *(int *)ret);

    env->DeleteGlobalRef(g_ctx.application_context);
    g_ctx.application_context = nullptr;
    return;
}

extern "C" jint JNIEXPORT JNICALL JNI_OnLoad(JavaVM* jvm, void* reserved) {
    LOGI(TAG, "JNI_OnLoad()");
    webrtc::InitAndroid(jvm);
    LOGI(TAG, "webrtc::InitAndroid() completed");
    webrtc::JVM::Initialize(jvm);
    LOGI(TAG, "webrtc::JVM::Initialize() completed");
    g_ctx.jvm = jvm;
    return JNI_VERSION_1_6;
}

extern "C" void JNIEXPORT JNICALL JNI_OnUnLoad(JavaVM* jvm, void* reserved) {
    LOGI(TAG, "JNI_OnUnLoad()");
}


static void *app_thread_func(void *userdata) {
    int ret = 0;
    LOGI(TAG, "Starting PeerAudioClient");

    JNIEnv * env;
    // double check it's all ok
    int getEnvStat = g_ctx.jvm->GetEnv((void **)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        LOGI(TAG, "GetEnv: not attached.");
        if (g_ctx.jvm->AttachCurrentThread(&env, nullptr) != 0) {
            LOGE(TAG, "Failed to attach env to Current Thread");
            ret = -1;
            pthread_exit(&ret);
        } else {
            LOGI(TAG, "JNIEnv attached to Current Thread");
        }
    } else if (getEnvStat == JNI_OK) {
        LOGI(TAG, "JNIEnv already attached to Current Thread");
    } else if (getEnvStat == JNI_EVERSION) {
        LOGE(TAG, "GetEnv: version not supported");
        ret = -1;
        pthread_exit(&ret);
    }

//    webrtc::InitAndroid(g_ctx.jvm);
//    LOGI(TAG, "webrtc::InitAndroid() completed");
//    webrtc::JVM::Initialize(g_ctx.jvm);
//    LOGI(TAG, "webrtc::JVM::Initialize() completed");



    auto client = rtc::make_ref_counted<PeerAudioClient>();

    rtc::InitializeSSL();

    client->init_webrtc(env, g_ctx.application_context);

    client->init_signaling();

    LOGI(TAG, "Connecting to %s:%d", SERVER_HOSTNAME, SERVER_PORT);
    client->connect_sync(SERVER_HOSTNAME, SERVER_PORT);

    // client->query_peer_type();
    try {
        int count = 0;
        auto b = std::chrono::high_resolution_clock::now();
        while(g_ctx.system_on && client->is_connected()) {
            // for (int i = 0; i < 10; i++) {
            // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            if (client->getType() == PeerClient::PeerType::Caller) {
                LOGI(TAG, "Callee main thread is doing nothing.");
                std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            } else {
                LOGI(TAG, "Callee main thread is doing nothing.");
                std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            }
        }
    } catch (std::exception &e) {
        LOGE(TAG, "Terminated by Interrupt: {} ", e.what());
        ret = -1;
    }

    rtc::CleanupSSL();
    client->deinit_signaling();

    LOGI(TAG, "Stopped.");
    pthread_exit(&ret);
}
