#pragma once

#include <atomic>
#include "hu_aap.h"

#include "command_server.h"
#include "audio.h"
#include <dbus-c++/dbus.h>
#include <asoundlib.h>

#include "dbus/generated_cmu.h"

class VideoOutput;
class AudioOutput;

//This is kind of hacky, but they are only for the query HTTP request
struct GlobalState
{
    static std::atomic<bool> connected;
    static std::atomic<bool> videoFocus;
    static std::atomic<bool> audioFocus;
};

class MazdaEventCallbacks;

class AudioManagerClient : public com::xsembedded::ServiceProvider_proxy,
                     public DBus::ObjectProxy
{
    std::map<std::string, int> streamToSessionIds;
    //"USB" as far as the audio manager cares is the normal ALSA sound output
    int usbSessionID = -1;
    int previousSessionID = -1;
    bool waitingForFocusLostEvent = false;
    MazdaEventCallbacks& callbacks;

    //These IDs are usually the same, but they depend on the startup order of the services on the car so we can't assume them 100% reliably
    void populateStreamTable();
public:
    AudioManagerClient(MazdaEventCallbacks& callbacks, DBus::Connection &connection);

    bool canSwitchAudio();

    //calling requestAudioFocus directly doesn't work on the audio mgr
    void audioMgrRequestAudioFocus();
    void audioMgrReleaseAudioFocus();

    virtual void Notify(const std::string& signalName, const std::string& payload) override;
};

class MazdaEventCallbacks : public IHUConnectionThreadEventCallbacks {
        std::unique_ptr<VideoOutput> videoOutput;
        std::unique_ptr<AudioOutput> audioOutput;

        MicInput micInput;
        DBus::Connection& serviceBus;
        DBus::Connection& hmiBus;

        std::unique_ptr<AudioManagerClient> audioMgrClient;
public:
        MazdaEventCallbacks(DBus::Connection& serviceBus, DBus::Connection& hmiBus);
        ~MazdaEventCallbacks();

        virtual int MediaPacket(int chan, uint64_t timestamp, const byte * buf, int len) override;
        virtual int MediaStart(int chan) override;
        virtual int MediaStop(int chan) override;
        virtual void MediaSetupComplete(int chan) override;
        virtual void DisconnectionOrError() override;
        virtual void CustomizeOutputChannel(int chan, HU::ChannelDescriptor::OutputStreamChannel& streamChannel) override;
        virtual void AudioFocusRequest(int chan, const HU::AudioFocusRequest& request) override;
        virtual void VideoFocusRequest(int chan, const HU::VideoFocusRequest& request) override;

        void VideoFocusHappened(bool hasFocus, bool unrequested);
        void AudioFocusHappend(bool hasFocus);
};

class MazdaCommandServerCallbacks : public ICommandServerCallbacks
{
    MazdaEventCallbacks& eventCallbacks;
public:
    MazdaCommandServerCallbacks(MazdaEventCallbacks& eventCallbacks);

    virtual bool IsConnected() const override;
    virtual bool HasAudioFocus() const override;
    virtual bool HasVideoFocus() const override;
    virtual void TakeVideoFocus() override;
};
