#include "HybridOpenwakeword.hpp"
#include <cstring>
#include <chrono>

#ifdef __ANDROID__
  #include <android/log.h>
  #include <sys/system_properties.h>
  #define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "OpenWakeWord", __VA_ARGS__)
  #define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "OpenWakeWord", __VA_ARGS__)
#else
  #include <cstdio>
  #define LOGE(...) fprintf(stderr, __VA_ARGS__)
  #define LOGD(...) fprintf(stdout, __VA_ARGS__)
#endif

namespace margelo::nitro::openwakeword {

static uint64_t now_us() {
    using clock = std::chrono::steady_clock;
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            clock::now().time_since_epoch()).count());
}

static bool perf_logging_enabled() {
#ifdef __ANDROID__
    char value[PROP_VALUE_MAX] = {};
    int len = __system_property_get("debug.openwakeword.perf", value);
    return len > 0 &&
           (std::strcmp(value, "1") == 0 ||
            std::strcmp(value, "true") == 0 ||
            std::strcmp(value, "on") == 0);
#else
    return false;
#endif
}

// ── SIMD int16 → float32 ─────────────────────────────────────────────────
//
// On ARM: processes 8 samples per iteration using NEON widening + convert.
//   vld1q_s16:     load 8 × int16
//   vmovl_s16:     widen low/high halves to int32×4
//   vcvtq_f32_s32: convert to float32×4
//
// On other architectures: plain scalar loop; the compiler will auto-vectorize
// with SSE/AVX when -O3 -ffast-math are set (handled in CMakeLists / podspec).
// ─────────────────────────────────────────────────────────────────────────
#if defined(OWW_HAS_NEON)
static void s16_to_f32(const int16_t* __restrict__ src,
                        float*         __restrict__ dst,
                        int n) {
    int n8 = n & ~7;
    for (int i = 0; i < n8; i += 8) {
        int16x8_t v16 = vld1q_s16(src + i);
        int32x4_t lo  = vmovl_s16(vget_low_s16(v16));
        int32x4_t hi  = vmovl_s16(vget_high_s16(v16));
        vst1q_f32(dst + i,     vcvtq_f32_s32(lo));
        vst1q_f32(dst + i + 4, vcvtq_f32_s32(hi));
    }
    for (int i = n8; i < n; i++) dst[i] = static_cast<float>(src[i]);
}
#else
static void s16_to_f32(const int16_t* __restrict__ src,
                        float*         __restrict__ dst,
                        int n) {
    for (int i = 0; i < n; i++) dst[i] = static_cast<float>(src[i]);
}
#endif

// ── SIMD mel normalisation: (x / 10.0f) + 2.0f  ≡  x * 0.1f + 2.0f ────
//
// On ARM: vmlaq_f32 is a fused multiply-add in a single instruction:
//   result = offset + v * scale  → 4 floats per cycle.
// ─────────────────────────────────────────────────────────────────────────
#if defined(OWW_HAS_NEON)
static void normalize_mel(float* __restrict__ data, int n) {
    const float32x4_t scale  = vdupq_n_f32(0.1f);
    const float32x4_t offset = vdupq_n_f32(2.0f);
    int n4 = n & ~3;
    for (int i = 0; i < n4; i += 4) {
        float32x4_t v = vld1q_f32(data + i);
        v = vmlaq_f32(offset, v, scale); // offset + v * scale
        vst1q_f32(data + i, v);
    }
    for (int i = n4; i < n; i++) data[i] = data[i] * 0.1f + 2.0f;
}
#else
static void normalize_mel(float* __restrict__ data, int n) {
    for (int i = 0; i < n; i++) data[i] = data[i] * 0.1f + 2.0f;
}
#endif

// ── Thread count heuristic ────────────────────────────────────────────────
//
// XNNPACK has its own pthreadpool — TfLiteInterpreterOptionsSetNumThreads()
// does NOT affect it. Thread count must be set on xnn_opts.num_threads.
//
// big.LITTLE research: mixing big + little cores adds synchronisation
// barriers at layer boundaries. For small models, using only the 4 big
// cores (A715/A710 on SD8G2) is faster than 8 mixed cores.
// ─────────────────────────────────────────────────────────────────────────
int HybridOpenwakeword::optimal_threads() {
#if defined(__aarch64__)
    // ARM64: Snapdragon 8 Gen 2/3, Apple A-series — 4 perf cores is the
    // sweet spot for the small sequential pipeline.
    return 4;
#elif defined(__arm__)
    // armeabi-v7a: conservative; mixing big+little is unreliable.
    return 2;
#else
    // x86 / x86_64 (emulator or desktop CI).
    return 2;
#endif
}

// ── Destruction / cleanup ─────────────────────────────────────────────────
HybridOpenwakeword::~HybridOpenwakeword() { cleanupModels(); }

void HybridOpenwakeword::cleanupModels() {
    // Correct deletion order: interpreters → models → options → delegate.
    // The delegate must outlive every interpreter that uses it.
    if (melspec_interp_)   { TfLiteInterpreterDelete(melspec_interp_);   melspec_interp_   = nullptr; }
    if (melspec_model_)    { TfLiteModelDelete(melspec_model_);          melspec_model_    = nullptr; }
    if (embedding_interp_) { TfLiteInterpreterDelete(embedding_interp_); embedding_interp_ = nullptr; }
    if (embedding_model_)  { TfLiteModelDelete(embedding_model_);        embedding_model_  = nullptr; }
    if (wakeword_interp_)  { TfLiteInterpreterDelete(wakeword_interp_);  wakeword_interp_  = nullptr; }
    if (wakeword_model_)   { TfLiteModelDelete(wakeword_model_);         wakeword_model_   = nullptr; }
    if (options_)          { TfLiteInterpreterOptionsDelete(options_);   options_          = nullptr; }
#if defined(OWW_HAS_XNNPACK)
    // Delete after all interpreters that reference this delegate are gone.
    if (xnnpack_delegate_) { TfLiteXNNPackDelegateDelete(xnnpack_delegate_); xnnpack_delegate_ = nullptr; }
#endif
}

// ── loadModels ────────────────────────────────────────────────────────────
bool HybridOpenwakeword::loadModels(const std::string& melspecPath,
                                     const std::string& embeddingPath,
                                     const std::string& wakeWordPath) {
    cleanupModels();

    const int threads = optimal_threads();

    // Build interpreter options.
    options_ = TfLiteInterpreterOptionsCreate();

    // This covers non-XNNPACK fallback kernels (e.g. custom ops, unsupported ops).
    TfLiteInterpreterOptionsSetNumThreads(options_, threads);

#if defined(OWW_HAS_XNNPACK)
    // XNNPACK: hand-vectorised NEON kernels, shared pthreadpool across all
    // three interpreters, optional FP16 auto-downcast on ARMv8.2+ devices.
    // One delegate instance is shared — XNNPACK supports this and it
    // enables weight deduplication across the three model stages.
    TfLiteXNNPackDelegateOptions xnn = TfLiteXNNPackDelegateOptionsDefault();
    xnn.num_threads = threads; // CRITICAL: must be set here, not on interpreter options
    // FP16 auto-detection is on by default in TFLite 2.14+ — no force flag needed.
    xnnpack_delegate_ = TfLiteXNNPackDelegateCreate(&xnn);
    if (xnnpack_delegate_) {
        TfLiteInterpreterOptionsAddDelegate(options_, xnnpack_delegate_);
        LOGD("XNNPACK delegate enabled (%d threads)", threads);
    } else {
        LOGE("XNNPACK delegate creation failed; falling back to default CPU kernels");
    }
#endif

    // Helper: load a model file, create its interpreter, resize + allocate tensors.
    auto load_model = [&](const std::string& path,
                          TfLiteModel**       out_model,
                          TfLiteInterpreter** out_interp,
                          int dims[], int ndims) -> bool {
        *out_model = TfLiteModelCreateFromFile(path.c_str());
        if (!*out_model) { LOGE("Failed to load model: %s", path.c_str()); return false; }

        *out_interp = TfLiteInterpreterCreate(*out_model, options_);
        if (!*out_interp) { LOGE("Failed to create interpreter: %s", path.c_str()); return false; }

        TfLiteInterpreterResizeInputTensor(*out_interp, 0, dims, ndims);
        if (TfLiteInterpreterAllocateTensors(*out_interp) != kTfLiteOk) {
            LOGE("Failed to allocate tensors: %s", path.c_str());
            return false;
        }
        return true;
    };

    int melspec_dims[2]   = {1, 1280};       // [batch, pcm_samples]
    int embedding_dims[4] = {1, 76, 32, 1};  // [batch, frames, bins, channels]
    int wakeword_dims[3]  = {1, 16, 96};     // [batch, frames, features]

    if (!load_model(melspecPath,   &melspec_model_,   &melspec_interp_,   melspec_dims,   2)) return false;
    if (!load_model(embeddingPath, &embedding_model_, &embedding_interp_, embedding_dims, 4)) return false;
    if (!load_model(wakeWordPath,  &wakeword_model_,  &wakeword_interp_,  wakeword_dims,  3)) return false;

    LOGD("OpenWakeWord models loaded. Threads: %d", threads);
    return true;
}

// ── processFrame  (hot path) ──────────────────────────────────────────────
//
// Pipeline per 1280-sample window:
//   AudioRing (O(1) pop) → s16_to_f32 → melspec model
//   → normalize_mel → MelRing (O(1) push)
//   → [76 frames ready] → embedding model
//   → EmbRing (O(1) push)
//   → [16 frames ready] → wakeword model → score
//
// All tensor writes go directly to TFLite's backing buffers — zero extra copy.
// ─────────────────────────────────────────────────────────────────────────
DetectionResult HybridOpenwakeword::processFrame(const std::shared_ptr<ArrayBuffer>& buffer) {
    if (__builtin_expect(!melspec_interp_ || !embedding_interp_ || !wakeword_interp_, 0)) {
        return DetectionResult(0.0, false);
    }

    const bool perf_on = perf_logging_enabled();
    const uint64_t call_start_us = perf_on ? now_us() : 0;
    uint64_t call_mel_us = 0;
    uint64_t call_embedding_us = 0;
    uint64_t call_wakeword_us = 0;
    uint64_t call_windows = 0;

    // Enqueue incoming PCM into the ring.
    const auto* pcm    = reinterpret_cast<const int16_t*>(buffer->data());
    const int   n_samp = static_cast<int>(buffer->size() / sizeof(int16_t));
    audio_ring_.push(pcm, n_samp);

    float latest_prob = 0.0f;

    while (__builtin_expect(audio_ring_.count >= 1280, 1)) {
        call_windows++;
        // ── Stage 1: Melspec ──────────────────────────────────────────────
        // Peek 1280 int16 samples into a local stack buffer, then convert
        // directly into the TFLite input tensor memory. No intermediate heap.
        float* melspec_in = static_cast<float*>(
            TfLiteTensorData(TfLiteInterpreterGetInputTensor(melspec_interp_, 0)));

        {
            int16_t temp[1280];
            audio_ring_.peek(temp, 1280);
            audio_ring_.consume(1280);
            s16_to_f32(temp, melspec_in, 1280);
        }

        uint64_t stage_start_us = perf_on ? now_us() : 0;
        TfLiteStatus mel_status = TfLiteInterpreterInvoke(melspec_interp_);
        if (perf_on) call_mel_us += now_us() - stage_start_us;
        if (mel_status != kTfLiteOk) continue;

        // Normalize melspec output in-place, then push 8 new frames.
        float* mel_out = static_cast<float*>(
            TfLiteTensorData(TfLiteInterpreterGetOutputTensor(melspec_interp_, 0)));
        normalize_mel(mel_out, 8 * MelRing::BINS);
        mel_ring_.push(mel_out, 8);

        // ── Stage 2: Embedding ────────────────────────────────────────────
        // fill() writes the 76 most-recent mel frames directly into the
        // embedding interpreter's input tensor — one memcpy per frame, no
        // extra staging buffer needed.
        float* emb_in = static_cast<float*>(
            TfLiteTensorData(TfLiteInterpreterGetInputTensor(embedding_interp_, 0)));
        if (!mel_ring_.fill(emb_in, 76)) continue;

        stage_start_us = perf_on ? now_us() : 0;
        TfLiteStatus embedding_status = TfLiteInterpreterInvoke(embedding_interp_);
        if (perf_on) call_embedding_us += now_us() - stage_start_us;
        if (embedding_status != kTfLiteOk) continue;

        // Push the 96-dim embedding frame into the sliding window ring.
        const float* emb_out = static_cast<const float*>(
            TfLiteTensorData(TfLiteInterpreterGetOutputTensor(embedding_interp_, 0)));
        emb_ring_.push(emb_out);

        // ── Stage 3: Wake word head ───────────────────────────────────────
        float* ww_in = static_cast<float*>(
            TfLiteTensorData(TfLiteInterpreterGetInputTensor(wakeword_interp_, 0)));
        if (!emb_ring_.fill(ww_in, 16)) continue;

        stage_start_us = perf_on ? now_us() : 0;
        TfLiteStatus wakeword_status = TfLiteInterpreterInvoke(wakeword_interp_);
        if (perf_on) call_wakeword_us += now_us() - stage_start_us;
        if (wakeword_status != kTfLiteOk) continue;

        latest_prob = static_cast<const float*>(
            TfLiteTensorData(TfLiteInterpreterGetOutputTensor(wakeword_interp_, 0)))[0];

        if (__builtin_expect(latest_prob > threshold_, 0)) {
            LOGE("[!!!] WAKE WORD DETECTED! Probability: %.4f", latest_prob);
        }
    }

    if (perf_on) {
        const uint64_t total_us = now_us() - call_start_us;
        perf_.calls++;
        perf_.windows += call_windows;
        perf_.total_us += total_us;
        perf_.mel_us += call_mel_us;
        perf_.embedding_us += call_embedding_us;
        perf_.wakeword_us += call_wakeword_us;
        perf_.max_total_us = std::max(perf_.max_total_us, total_us);
        perf_.max_mel_us = std::max(perf_.max_mel_us, call_mel_us);
        perf_.max_embedding_us = std::max(perf_.max_embedding_us, call_embedding_us);
        perf_.max_wakeword_us = std::max(perf_.max_wakeword_us, call_wakeword_us);

        const uint64_t now = now_us();
        if (perf_.last_log_us == 0) perf_.last_log_us = now;
        if (now - perf_.last_log_us >= 5000000 && perf_.calls > 0) {
            const double calls = static_cast<double>(perf_.calls);
            const double windows = static_cast<double>(std::max<uint64_t>(perf_.windows, 1));
            LOGD("perf calls=%llu windows=%llu avg_total=%.3fms max_total=%.3fms "
                 "avg_mel=%.3fms max_mel=%.3fms avg_embedding=%.3fms max_embedding=%.3fms "
                 "avg_wakeword=%.3fms max_wakeword=%.3fms",
                 static_cast<unsigned long long>(perf_.calls),
                 static_cast<unsigned long long>(perf_.windows),
                 perf_.total_us / calls / 1000.0,
                 perf_.max_total_us / 1000.0,
                 perf_.mel_us / windows / 1000.0,
                 perf_.max_mel_us / 1000.0,
                 perf_.embedding_us / windows / 1000.0,
                 perf_.max_embedding_us / 1000.0,
                 perf_.wakeword_us / windows / 1000.0,
                 perf_.max_wakeword_us / 1000.0);
            perf_.clear();
            perf_.last_log_us = now;
        }
    }

    return DetectionResult(latest_prob, latest_prob >= threshold_);
}

// ── setThreshold / reset ──────────────────────────────────────────────────
void HybridOpenwakeword::setThreshold(double threshold) {
    threshold_ = threshold;
}

void HybridOpenwakeword::reset() {
    audio_ring_.clear();
    mel_ring_.clear();
    emb_ring_.clear();
    perf_.clear();
    // DO NOT call TfLiteInterpreterAllocateTensors here. Tensor shapes are fixed
    // at loadModels() time and never change. Re-allocating is expensive and wrong.
}

} // namespace margelo::nitro::openwakeword
