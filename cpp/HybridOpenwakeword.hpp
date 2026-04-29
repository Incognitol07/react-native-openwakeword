#pragma once

#include <vector>
#include <string>
#include "HybridOpenwakewordSpec.hpp"
#ifdef __APPLE__
#include <TensorFlowLiteC/TfLiteC.h>
#else
#include <tensorflow/lite/c/c_api.h>
#endif

namespace margelo::nitro::openwakeword {
class HybridOpenwakeword : public HybridOpenwakewordSpec {
    public:
        static constexpr auto TAG = "Openwakeword";
        HybridOpenwakeword() : HybridObject(TAG), HybridOpenwakewordSpec(), threshold_(0.5) {
            // Pre-allocate sliding window buffers
            audio_buffer_.reserve(1280 * 2);
            melspec_buffer_.assign(76 * 32, 0.0f);
            embedding_buffer_.assign(16 * 96, 0.0f);
        }
        ~HybridOpenwakeword() override;
       
        bool loadModels(const std::string& melspecPath, const std::string& embeddingPath, const std::string& wakeWordPath) override;
        DetectionResult processFrame(const std::shared_ptr<ArrayBuffer>& buffer) override;
        void setThreshold(double threshold) override;
        void reset() override;

    private:
        double threshold_;
        
        TfLiteModel* melspec_model_ = nullptr;
        TfLiteInterpreter* melspec_interp_ = nullptr;
        
        TfLiteModel* embedding_model_ = nullptr;
        TfLiteInterpreter* embedding_interp_ = nullptr;
        
        TfLiteModel* wakeword_model_ = nullptr;
        TfLiteInterpreter* wakeword_interp_ = nullptr;
        
        TfLiteInterpreterOptions* options_ = nullptr;

        // Streaming Buffers
        std::vector<int16_t> audio_buffer_;
        std::vector<float> melspec_buffer_; // 76 frames * 32 bins
        std::vector<float> embedding_buffer_; // 16 frames * 96 features
        
        int melspec_frames_filled_ = 0;
        int embedding_frames_filled_ = 0;

        void cleanupModels();
        void shiftLeft(std::vector<float>& buffer, int elements_to_shift);
    };
} // namespace margelo::nitro::openwakeword
