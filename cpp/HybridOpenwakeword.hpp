#pragma once

#include <string>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include "HybridOpenwakewordSpec.hpp"

// ── Platform TFLite headers ───────────────────────────────────────────────
#ifdef __APPLE__
  #include <TensorFlowLiteC/c_api.h>
  #if __has_include(<TensorFlowLiteC/xnnpack_delegate.h>)
    #include <TensorFlowLiteC/xnnpack_delegate.h>
    #define OWW_HAS_XNNPACK 1
  #endif
#else
  #include <tensorflow/lite/c/c_api.h>
  #if __has_include(<tensorflow/lite/delegates/xnnpack/xnnpack_delegate.h>)
    #include <tensorflow/lite/delegates/xnnpack/xnnpack_delegate.h>
    #define OWW_HAS_XNNPACK 1
  #endif
#endif

// ── NEON detection (ARM32 and ARM64) ─────────────────────────────────────
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
  #include <arm_neon.h>
  #define OWW_HAS_NEON 1
#endif

namespace margelo::nitro::openwakeword {

// ─────────────────────────────────────────────────────────────────────────
// AudioRing  —  lock-free O(1) SPSC ring for int16 PCM samples.
// Replaces std::vector::erase(begin()) which was O(n) on every pop.
// ─────────────────────────────────────────────────────────────────────────
struct AudioRing {
    static constexpr int CAP  = 4096; // power of 2, must be >= max frame size × 2
    static constexpr int MASK = CAP - 1;

    int16_t buf[CAP]{};
    int write = 0; // next write position [0, CAP)
    int count = 0; // live samples in the ring

    // Two-shot memcpy push — no element-wise loop.
    void push(const int16_t* src, int n) {
        int tail = CAP - write;
        if (n <= tail) {
            std::memcpy(buf + write, src, n * sizeof(int16_t));
        } else {
            std::memcpy(buf + write, src,        tail * sizeof(int16_t));
            std::memcpy(buf,         src + tail, (n - tail) * sizeof(int16_t));
        }
        write = (write + n) & MASK;
        count = std::min(count + n, CAP);
    }

    // Two-shot memcpy peek — reads n oldest samples without consuming them.
    void peek(int16_t* dst, int n) const {
        int read  = (write - count + CAP) & MASK;
        int first = std::min(n, CAP - read);
        std::memcpy(dst,         buf + read, first * sizeof(int16_t));
        if (first < n)
            std::memcpy(dst + first, buf, (n - first) * sizeof(int16_t));
    }

    // O(1): just move the logical read pointer forward.
    void consume(int n) { count -= n; }

    void clear() { write = 0; count = 0; }
};

// ─────────────────────────────────────────────────────────────────────────
// MelRing  —  O(1) sliding window for mel-spectrogram frames.
// Each frame is BINS floats. Replaces the O(n) memmove-based shiftLeft().
// ─────────────────────────────────────────────────────────────────────────
struct MelRing {
    static constexpr int FRAMES = 128; // power of 2, must be >= 76
    static constexpr int BINS   = 32;
    static constexpr int MASK   = FRAMES - 1;

    alignas(64) float buf[FRAMES][BINS]{}; // 64-byte aligned for cache-line fit
    int write = 0; // next write slot [0, FRAMES)
    int total = 0; // total frames ever pushed, capped at FRAMES

    // Push n_frames contiguous frames (each BINS floats) from src.
    void push(const float* src, int n_frames) {
        for (int f = 0; f < n_frames; f++) {
            std::memcpy(buf[write], src + f * BINS, BINS * sizeof(float));
            write = (write + 1) & MASK;
        }
        total = std::min(total + n_frames, FRAMES);
    }

    // Copy the n_frames most-recent frames (oldest→newest) into dst.
    // Returns false if fewer than n_frames have been pushed.
    bool fill(float* dst, int n_frames) const {
        if (total < n_frames) return false;
        int start = (write - n_frames + FRAMES) & MASK;
        for (int f = 0; f < n_frames; f++) {
            std::memcpy(dst + f * BINS, buf[(start + f) & MASK], BINS * sizeof(float));
        }
        return true;
    }

    void clear() { write = 0; total = 0; std::memset(buf, 0, sizeof(buf)); }
};

// ─────────────────────────────────────────────────────────────────────────
// EmbRing  —  O(1) sliding window for embedding frames (16 × 96 floats).
// Same design as MelRing, one frame pushed at a time.
// ─────────────────────────────────────────────────────────────────────────
struct EmbRing {
    static constexpr int FRAMES = 32; // power of 2, must be >= 16
    static constexpr int FEATS  = 96;
    static constexpr int MASK   = FRAMES - 1;

    alignas(64) float buf[FRAMES][FEATS]{};
    int write = 0;
    int total = 0;

    void push(const float* frame) {
        std::memcpy(buf[write], frame, FEATS * sizeof(float));
        write = (write + 1) & MASK;
        total = std::min(total + 1, FRAMES);
    }

    bool fill(float* dst, int n_frames) const {
        if (total < n_frames) return false;
        int start = (write - n_frames + FRAMES) & MASK;
        for (int f = 0; f < n_frames; f++) {
            std::memcpy(dst + f * FEATS, buf[(start + f) & MASK], FEATS * sizeof(float));
        }
        return true;
    }

    void clear() { write = 0; total = 0; std::memset(buf, 0, sizeof(buf)); }
};

// ─────────────────────────────────────────────────────────────────────────
// HybridOpenwakeword
// ─────────────────────────────────────────────────────────────────────────
class HybridOpenwakeword : public HybridOpenwakewordSpec {
public:
    static constexpr auto TAG = "Openwakeword";
    HybridOpenwakeword() : HybridObject(TAG), HybridOpenwakewordSpec() {}
    ~HybridOpenwakeword() override;

    bool loadModels(const std::string& melspecPath,
                    const std::string& embeddingPath,
                    const std::string& wakeWordPath) override;
    DetectionResult processFrame(const std::shared_ptr<ArrayBuffer>& buffer) override;
    void setThreshold(double threshold) override;
    void reset() override;

private:
    struct PerfStats {
        uint64_t calls = 0;
        uint64_t windows = 0;
        uint64_t total_us = 0;
        uint64_t mel_us = 0;
        uint64_t embedding_us = 0;
        uint64_t wakeword_us = 0;
        uint64_t max_total_us = 0;
        uint64_t max_mel_us = 0;
        uint64_t max_embedding_us = 0;
        uint64_t max_wakeword_us = 0;
        uint64_t last_log_us = 0;

        void clear() {
            calls = 0;
            windows = 0;
            total_us = 0;
            mel_us = 0;
            embedding_us = 0;
            wakeword_us = 0;
            max_total_us = 0;
            max_mel_us = 0;
            max_embedding_us = 0;
            max_wakeword_us = 0;
            last_log_us = 0;
        }
    };

    double threshold_ = 0.5;

    TfLiteModel*              melspec_model_    = nullptr;
    TfLiteInterpreter*        melspec_interp_   = nullptr;
    TfLiteModel*              embedding_model_  = nullptr;
    TfLiteInterpreter*        embedding_interp_ = nullptr;
    TfLiteModel*              wakeword_model_   = nullptr;
    TfLiteInterpreter*        wakeword_interp_  = nullptr;
    TfLiteInterpreterOptions* options_          = nullptr;
#if defined(OWW_HAS_XNNPACK)
    TfLiteDelegate*           xnnpack_delegate_ = nullptr;
#endif

    AudioRing audio_ring_;
    MelRing   mel_ring_;
    EmbRing   emb_ring_;
    PerfStats perf_;

    void cleanupModels();

    // Returns optimal XNNPACK/interpreter thread count for this CPU.
    static int optimal_threads();
};

} // namespace margelo::nitro::openwakeword
