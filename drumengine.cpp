/*
 * Copyright 2026 Rodolfo González González
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "drumengine.h"

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/utils/result.h>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <array>
#include <mutex>
#include <algorithm>

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
static constexpr int    SR       = 44100;   // Hz
static constexpr int    CHANNELS = 1;
static constexpr float  GAIN     = 0.90f;

// ---------------------------------------------------------------------------
// Synthesis helpers
// ---------------------------------------------------------------------------
template<typename F>
static std::vector<float> synth(double durationSec, F &&gen)
{
    const int n = static_cast<int>(SR * durationSec);
    std::vector<float> buf(n);
    for (int i = 0; i < n; ++i) {
        double v = std::clamp(gen(i), -1.0, 1.0);
        buf[i] = static_cast<float>(v);
    }
    return buf;
}

static std::vector<float> makeBombo()
{
    return synth(0.55, [](int i) -> double {
        const double t = double(i) / SR;
        // Frequency sweep 100 Hz → 38 Hz
        const double phase = 2.0 * M_PI * (6.888 * (1.0 - std::exp(-9.0 * t)) + 38.0 * t);
        const double body  = std::sin(phase) * std::exp(-4.5 * t);
        const double click = std::exp(-150.0 * t) * 0.55;
        return body * 0.90 + click;
    });
}

static std::vector<float> makeTarola()
{
    const int n = int(SR * 0.35);
    std::srand(1234);
    std::vector<double> noise(n);
    for (auto &v : noise) v = double(std::rand()) / RAND_MAX * 2.0 - 1.0;
    return synth(0.35, [&noise](int i) -> double {
        const double t    = double(i) / SR;
        const double tone = std::sin(2.0 * M_PI * 210.0 * t) * 0.25;
        const double snap = std::exp(-55.0 * t) * 0.45;
        return std::exp(-11.0 * t) * (tone + noise[i] * 0.80) + snap;
    });
}

static std::vector<float> makeHihat()
{
    const int n = int(SR * 0.11);
    std::srand(5678);
    std::vector<double> noise(n);
    for (auto &v : noise) v = double(std::rand()) / RAND_MAX * 2.0 - 1.0;
    return synth(0.11, [&noise](int i) -> double {
        const double t  = double(i) / SR;
        const double hp = noise[i] * std::sin(2.0 * M_PI * 9500.0 * double(i) / SR);
        return hp * std::exp(-38.0 * t);
    });
}

static std::vector<float> makePlatillo()
{
    const int n = int(SR * 2.2);
    std::srand(9012);
    std::vector<double> noise(n);
    for (auto &v : noise) v = double(std::rand()) / RAND_MAX * 2.0 - 1.0;
    return synth(2.2, [&noise](int i) -> double {
        const double t = double(i) / SR;
        const double metal =
            std::sin(2.0 * M_PI * 2800.0 * t) * 0.22 +
            std::sin(2.0 * M_PI * 4350.0 * t) * 0.18 +
            std::sin(2.0 * M_PI * 6200.0 * t) * 0.14 +
            std::sin(2.0 * M_PI * 8750.0 * t) * 0.09;
        const double amp    = std::exp(-2.2 * t);
        const double attack = (t < 0.004) ? t / 0.004 : 1.0;
        return amp * attack * (metal + noise[i] * 0.38) * 0.92;
    });
}

static std::vector<float> makeCencerro()
{
    return synth(0.90, [](int i) -> double {
        const double t = double(i) / SR;
        const double body =
            std::sin(2.0 * M_PI * 562.0  * t) * 0.50 +
            std::sin(2.0 * M_PI * 845.0  * t) * 0.38 +
            std::sin(2.0 * M_PI * 1124.0 * t) * 0.18;
        return body * std::exp(-3.8 * t) + std::exp(-60.0 * t) * 0.35;
    });
}

static std::vector<float> makeClave()
{
    return synth(0.18, [](int i) -> double {
        const double t = double(i) / SR;
        const double body =
            std::sin(2.0 * M_PI * 2500.0 * t) * 0.60 +
            std::sin(2.0 * M_PI * 5000.0 * t) * 0.28 +
            std::sin(2.0 * M_PI * 7500.0 * t) * 0.10;
        return body * std::exp(-30.0 * t);
    });
}

static std::vector<float> makeQuijada()
{
    const int n = int(SR * 0.50);
    std::srand(3456);
    std::vector<double> noise(n);
    for (auto &v : noise) v = double(std::rand()) / RAND_MAX * 2.0 - 1.0;
    return synth(0.50, [&noise](int i) -> double {
        const double t   = double(i) / SR;
        const double mod = 0.5 + 0.5 * std::sin(2.0 * M_PI * 22.0 * t);
        return noise[i] * (std::exp(-14.0 * t) + std::exp(-3.5 * t) * 0.35) * mod;
    });
}

// ---------------------------------------------------------------------------
// ActiveSound: a sound being currently played (read by PW thread)
// ---------------------------------------------------------------------------
struct ActiveSound {
    const float *data   = nullptr;
    int          length = 0;
    int          pos    = 0;     // current read position (in samples)
};

// ---------------------------------------------------------------------------
// DrumEnginePrivate: holds PipeWire state + sound buffers
// ---------------------------------------------------------------------------
struct DrumEnginePrivate {
    // Sound buffers (generated once at init)
    std::array<std::vector<float>, 7> sounds;   // indexed by (int)DrumType

    // PipeWire objects
    struct pw_thread_loop *loop   = nullptr;
    struct pw_stream      *stream = nullptr;

    // Active voices (mixed by PW callback)
    std::mutex              voicesMtx;
    std::vector<ActiveSound> voices;

    // PipeWire stream event callbacks (static, forwarded to instance)
    static const pw_stream_events streamEvents;
    static void onProcess(void *userdata);
};

// ---------------------------------------------------------------------------
// PipeWire audio callback: called from the PW real-time thread
// ---------------------------------------------------------------------------
void DrumEnginePrivate::onProcess(void *userdata)
{
    auto *d = static_cast<DrumEnginePrivate *>(userdata);

    struct pw_buffer *pwbuf = pw_stream_dequeue_buffer(d->stream);
    if (!pwbuf) return;

    auto  *spa_buf = pwbuf->buffer;
    float *out     = static_cast<float *>(spa_buf->datas[0].data);
    if (!out) { pw_stream_queue_buffer(d->stream, pwbuf); return; }

    const uint32_t maxFrames = spa_buf->datas[0].maxsize / sizeof(float);
    const uint32_t nFrames   = pwbuf->requested ? std::min((uint32_t)pwbuf->requested, maxFrames)
                                                 : maxFrames;

    std::memset(out, 0, nFrames * sizeof(float));

    {
        std::lock_guard<std::mutex> lk(d->voicesMtx);

        for (auto &v : d->voices) {
            const int remaining = v.length - v.pos;
            const int toWrite   = std::min((int)nFrames, remaining);
            for (int i = 0; i < toWrite; ++i)
                out[i] += v.data[v.pos + i] * GAIN;
            v.pos += toWrite;
        }

        // Remove voices that have finished
        d->voices.erase(
            std::remove_if(d->voices.begin(), d->voices.end(),
                           [](const ActiveSound &v){ return v.pos >= v.length; }),
            d->voices.end());
    }

    // Soft clip
    for (uint32_t i = 0; i < nFrames; ++i)
        out[i] = std::clamp(out[i], -1.0f, 1.0f);

    spa_buf->datas[0].chunk->offset = 0;
    spa_buf->datas[0].chunk->stride = sizeof(float);
    spa_buf->datas[0].chunk->size   = nFrames * sizeof(float);

    pw_stream_queue_buffer(d->stream, pwbuf);
}

const pw_stream_events DrumEnginePrivate::streamEvents = {
    .version = PW_VERSION_STREAM_EVENTS,
    .process = DrumEnginePrivate::onProcess,
};

// ---------------------------------------------------------------------------
// DrumEngine
// ---------------------------------------------------------------------------
DrumEngine::DrumEngine(QObject *parent)
    : QObject(parent)
    , d(new DrumEnginePrivate)
{
    // Pre-generate all sounds
    d->sounds[int(DrumType::BOMBO)]    = makeBombo();
    d->sounds[int(DrumType::TAROLA)]   = makeTarola();
    d->sounds[int(DrumType::HIHAT)]    = makeHihat();
    d->sounds[int(DrumType::PLATILLO)] = makePlatillo();
    d->sounds[int(DrumType::CENCERRO)] = makeCencerro();
    d->sounds[int(DrumType::CLAVE)]    = makeClave();
    d->sounds[int(DrumType::QUIJADA)]  = makeQuijada();

    // Initialize PipeWire
    pw_init(nullptr, nullptr);

    d->loop = pw_thread_loop_new("drumbox-audio", nullptr);
    if (!d->loop) return;

    // Build audio format parameter
    uint8_t paramBuf[1024];
    struct spa_pod_builder  b   = SPA_POD_BUILDER_INIT(paramBuf, sizeof(paramBuf));
    struct spa_audio_info_raw ai = {};
    ai.format   = SPA_AUDIO_FORMAT_F32;
    ai.rate     = SR;
    ai.channels = CHANNELS;
    ai.position[0] = SPA_AUDIO_CHANNEL_MONO;
    const struct spa_pod *params[1];
    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &ai);

    d->stream = pw_stream_new_simple(
        pw_thread_loop_get_loop(d->loop),
        "DrumBox",
        pw_properties_new(
            PW_KEY_MEDIA_TYPE,     "Audio",
            PW_KEY_MEDIA_CATEGORY, "Playback",
            PW_KEY_MEDIA_ROLE,     "Music",
            nullptr),
        &DrumEnginePrivate::streamEvents,
        d);

    if (!d->stream) return;

    pw_stream_connect(
        d->stream,
        PW_DIRECTION_OUTPUT,
        PW_ID_ANY,
        static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT |
                                     PW_STREAM_FLAG_MAP_BUFFERS  |
                                     PW_STREAM_FLAG_RT_PROCESS),
        params, 1);

    pw_thread_loop_start(d->loop);
}

DrumEngine::~DrumEngine()
{
    if (d->loop)   pw_thread_loop_stop(d->loop);
    if (d->stream) pw_stream_destroy(d->stream);
    if (d->loop)   pw_thread_loop_destroy(d->loop);
    pw_deinit();
    delete d;
}

void DrumEngine::play(DrumType drum)
{
    const auto &snd = d->sounds[int(drum)];
    if (snd.empty()) return;

    std::lock_guard<std::mutex> lk(d->voicesMtx);
    d->voices.push_back({ snd.data(), int(snd.size()), 0 });
}
