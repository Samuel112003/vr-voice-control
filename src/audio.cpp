#include "voicecontrol/audio.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#endif

namespace vc {

// ---------------------------------------------------------------------------
// WAV
// ---------------------------------------------------------------------------
namespace {

std::uint32_t le32(const char* p) {
    auto u = reinterpret_cast<const unsigned char*>(p);
    return u[0] | (u[1] << 8) | (u[2] << 16) | (std::uint32_t(u[3]) << 24);
}

std::uint16_t le16(const char* p) {
    auto u = reinterpret_cast<const unsigned char*>(p);
    return static_cast<std::uint16_t>(u[0] | (u[1] << 8));
}

} // namespace

AudioClip readWav(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("impossible d'ouvrir " + path);
    std::vector<char> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (data.size() < 12 || std::memcmp(data.data(), "RIFF", 4) || std::memcmp(data.data() + 8, "WAVE", 4))
        throw std::runtime_error(path + " n'est pas un fichier WAV");

    AudioClip clip;
    bool fmtOk = false;
    for (std::size_t pos = 12; pos + 8 <= data.size();) {
        const char* chunk = data.data() + pos;
        std::size_t size = le32(chunk + 4);
        const char* body = chunk + 8;
        if (size > data.size() - pos - 8) size = data.size() - pos - 8;
        if (!std::memcmp(chunk, "fmt ", 4) && size >= 16) {
            if (le16(body) != 1 || le16(body + 2) != 1 || le16(body + 14) != 16)
                throw std::runtime_error(path + " : le WAV doit etre en PCM 16 bits mono");
            clip.sampleRate = le32(body + 4);
            fmtOk = true;
        } else if (!std::memcmp(chunk, "data", 4)) {
            clip.samples.resize(size / 2);
            std::memcpy(clip.samples.data(), body, clip.samples.size() * 2);
        }
        pos += 8 + size + (size & 1);
    }
    if (!fmtOk) throw std::runtime_error(path + " : en-tete WAV invalide");
    return clip;
}

// ---------------------------------------------------------------------------
// Microphone
// ---------------------------------------------------------------------------
#ifdef _WIN32

struct Microphone::Impl {
    static constexpr int kBuffers = 4;
    HWAVEIN device = nullptr;
    HANDLE event = nullptr;
    std::vector<std::vector<std::int16_t>> buffers;
    WAVEHDR headers[kBuffers]{};
    int next = 0;

    ~Impl() {
        if (device) {
            waveInStop(device);
            waveInReset(device);
            for (auto& h : headers)
                if (h.lpData) waveInUnprepareHeader(device, &h, sizeof(WAVEHDR));
            waveInClose(device);
        }
        if (event) CloseHandle(event);
    }
};

Microphone::Microphone(std::uint32_t sampleRate, unsigned blockMs)
    : impl_(std::make_unique<Impl>()), sampleRate_(sampleRate) {
    WAVEFORMATEX fmt{};
    fmt.wFormatTag = WAVE_FORMAT_PCM;
    fmt.nChannels = 1;
    fmt.nSamplesPerSec = sampleRate;
    fmt.wBitsPerSample = 16;
    fmt.nBlockAlign = 2;
    fmt.nAvgBytesPerSec = sampleRate * 2;

    impl_->event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!impl_->event ||
        waveInOpen(&impl_->device, WAVE_MAPPER, &fmt, reinterpret_cast<DWORD_PTR>(impl_->event), 0,
                   CALLBACK_EVENT) != MMSYSERR_NOERROR) {
        impl_->device = nullptr;
        throw std::runtime_error("impossible d'ouvrir le micro (verifiez qu'il est branche et autorise)");
    }

    const std::size_t frames = std::max<std::size_t>(1, sampleRate * blockMs / 1000);
    impl_->buffers.assign(Impl::kBuffers, std::vector<std::int16_t>(frames));
    for (int i = 0; i < Impl::kBuffers; ++i) {
        WAVEHDR& h = impl_->headers[i];
        h.lpData = reinterpret_cast<LPSTR>(impl_->buffers[i].data());
        h.dwBufferLength = static_cast<DWORD>(frames * 2);
        waveInPrepareHeader(impl_->device, &h, sizeof(WAVEHDR));
        waveInAddBuffer(impl_->device, &h, sizeof(WAVEHDR));
    }
    if (waveInStart(impl_->device) != MMSYSERR_NOERROR)
        throw std::runtime_error("impossible de demarrer la capture micro");
}

Microphone::~Microphone() = default;

bool Microphone::read(std::vector<std::int16_t>& block, unsigned timeoutMs) {
    WAVEHDR& h = impl_->headers[impl_->next];
    if (!(h.dwFlags & WHDR_DONE)) {
        WaitForSingleObject(impl_->event, timeoutMs);
        if (!(h.dwFlags & WHDR_DONE)) return false;
    }
    auto samples = reinterpret_cast<const std::int16_t*>(h.lpData);
    block.assign(samples, samples + h.dwBytesRecorded / 2);
    waveInAddBuffer(impl_->device, &h, sizeof(WAVEHDR));   // remet le tampon en file
    impl_->next = (impl_->next + 1) % Impl::kBuffers;
    return true;
}

bool Microphone::available() { return true; }

#else

struct Microphone::Impl {};

Microphone::Microphone(std::uint32_t sampleRate, unsigned)
    : sampleRate_(sampleRate) {
    throw std::runtime_error("capture micro non disponible sur cette plateforme");
}

Microphone::~Microphone() = default;

bool Microphone::read(std::vector<std::int16_t>&, unsigned) { return false; }

bool Microphone::available() { return false; }

#endif

} // namespace vc
