#include "HybridOpenwakeword.hpp"
#include <iostream>
#include <cstring>

namespace margelo::nitro::openwakeword {

HybridOpenwakeword::~HybridOpenwakeword() {
    cleanupModels();
}

void HybridOpenwakeword::cleanupModels() {
    if (melspec_interp_) TfLiteInterpreterDelete(melspec_interp_);
    if (melspec_model_) TfLiteModelDelete(melspec_model_);
    
    if (embedding_interp_) TfLiteInterpreterDelete(embedding_interp_);
    if (embedding_model_) TfLiteModelDelete(embedding_model_);
    
    if (wakeword_interp_) TfLiteInterpreterDelete(wakeword_interp_);
    if (wakeword_model_) TfLiteModelDelete(wakeword_model_);
    
    if (options_) TfLiteInterpreterOptionsDelete(options_);
    
    melspec_interp_ = nullptr;
    melspec_model_ = nullptr;
    embedding_interp_ = nullptr;
    embedding_model_ = nullptr;
    wakeword_interp_ = nullptr;
    wakeword_model_ = nullptr;
    options_ = nullptr;
}

bool HybridOpenwakeword::loadModels(const std::string& melspecPath, const std::string& embeddingPath, const std::string& wakeWordPath) {
    cleanupModels();
    
    options_ = TfLiteInterpreterOptionsCreate();
    TfLiteInterpreterOptionsSetNumThreads(options_, 2);
    
    // Load Melspec
    melspec_model_ = TfLiteModelCreateFromFile(melspecPath.c_str());
    if (!melspec_model_) return false;
    melspec_interp_ = TfLiteInterpreterCreate(melspec_model_, options_);
    if (!melspec_interp_) return false;
    if (TfLiteInterpreterAllocateTensors(melspec_interp_) != kTfLiteOk) return false;
    
    // Load Embedding
    embedding_model_ = TfLiteModelCreateFromFile(embeddingPath.c_str());
    if (!embedding_model_) return false;
    embedding_interp_ = TfLiteInterpreterCreate(embedding_model_, options_);
    if (!embedding_interp_) return false;
    if (TfLiteInterpreterAllocateTensors(embedding_interp_) != kTfLiteOk) return false;
    
    // Load Wakeword
    wakeword_model_ = TfLiteModelCreateFromFile(wakeWordPath.c_str());
    if (!wakeword_model_) return false;
    wakeword_interp_ = TfLiteInterpreterCreate(wakeword_model_, options_);
    if (!wakeword_interp_) return false;
    if (TfLiteInterpreterAllocateTensors(wakeword_interp_) != kTfLiteOk) return false;
    
    return true;
}

void HybridOpenwakeword::shiftLeft(std::vector<float>& buffer, int elements_to_shift) {
    if (elements_to_shift <= 0 || elements_to_shift >= buffer.size()) return;
    std::memmove(buffer.data(), buffer.data() + elements_to_shift, (buffer.size() - elements_to_shift) * sizeof(float));
}

DetectionResult HybridOpenwakeword::processFrame(const std::shared_ptr<ArrayBuffer>& buffer) {
    if (!melspec_interp_ || !embedding_interp_ || !wakeword_interp_) {
        return DetectionResult(0.0, false);
    }
    
    // 1. Append new audio to buffer
    int16_t* pcm_data = (int16_t*)buffer->data();
    size_t num_samples = buffer->size() / 2;
    audio_buffer_.insert(audio_buffer_.end(), pcm_data, pcm_data + num_samples);
    
    float latest_probability = 0.0f;
    
    // 2. Process audio in 1280-sample chunks
    while (audio_buffer_.size() >= 1280) {
        TfLiteTensor* melspec_input = TfLiteInterpreterGetInputTensor(melspec_interp_, 0);
        float* melspec_input_data = (float*)TfLiteTensorData(melspec_input);
        
        // Normalize 16-bit PCM to float32
        for (size_t i = 0; i < 1280; i++) {
            melspec_input_data[i] = audio_buffer_[i] / 32768.0f;
        }
        
        // Remove processed samples (sliding window)
        audio_buffer_.erase(audio_buffer_.begin(), audio_buffer_.begin() + 1280);
        
        // Run Melspec
        if (TfLiteInterpreterInvoke(melspec_interp_) != kTfLiteOk) continue;
        const TfLiteTensor* melspec_output = TfLiteInterpreterGetOutputTensor(melspec_interp_, 0);
        float* melspec_out_data = (float*)TfLiteTensorData(melspec_output);
        
        // Output is [1, 8, 32]. 8 frames of 32 features = 256 floats.
        int num_melspec_floats_out = 8 * 32;
        
        // Shift melspec buffer left by 8 frames
        shiftLeft(melspec_buffer_, num_melspec_floats_out);
        
        // Append new 8 frames at the end
        std::memcpy(melspec_buffer_.data() + (melspec_buffer_.size() - num_melspec_floats_out), melspec_out_data, num_melspec_floats_out * sizeof(float));
        
        if (melspec_frames_filled_ < 76) {
            melspec_frames_filled_ += 8;
        }
        
        // 3. If melspec buffer is full (at least 76 frames), run Embedding model
        if (melspec_frames_filled_ >= 76) {
            TfLiteTensor* embedding_input = TfLiteInterpreterGetInputTensor(embedding_interp_, 0);
            
            // Embedding input shape [1, 76, 32, 1]
            std::memcpy(TfLiteTensorData(embedding_input), melspec_buffer_.data(), 76 * 32 * sizeof(float));
            
            if (TfLiteInterpreterInvoke(embedding_interp_) != kTfLiteOk) continue;
            const TfLiteTensor* embedding_output = TfLiteInterpreterGetOutputTensor(embedding_interp_, 0);
            float* embedding_out_data = (float*)TfLiteTensorData(embedding_output);
            
            // Output is [1, 96]. 1 frame of 96 features.
            int num_embedding_floats_out = 96;
            
            // Shift embedding buffer left by 1 frame
            shiftLeft(embedding_buffer_, num_embedding_floats_out);
            
            // Append new 1 frame at the end
            std::memcpy(embedding_buffer_.data() + (embedding_buffer_.size() - num_embedding_floats_out), embedding_out_data, num_embedding_floats_out * sizeof(float));
            
            if (embedding_frames_filled_ < 16) {
                embedding_frames_filled_ += 1;
            }
            
            // 4. If embedding buffer is full (at least 16 frames), run Wakeword model
            if (embedding_frames_filled_ >= 16) {
                TfLiteTensor* wakeword_input = TfLiteInterpreterGetInputTensor(wakeword_interp_, 0);
                
                // Wakeword input shape [1, 16, 96]
                std::memcpy(TfLiteTensorData(wakeword_input), embedding_buffer_.data(), 16 * 96 * sizeof(float));
                
                if (TfLiteInterpreterInvoke(wakeword_interp_) != kTfLiteOk) continue;
                const TfLiteTensor* wakeword_output = TfLiteInterpreterGetOutputTensor(wakeword_interp_, 0);
                
                latest_probability = ((float*)TfLiteTensorData(wakeword_output))[0];
            }
        }
    }
    
    return DetectionResult(latest_probability, latest_probability >= threshold_);
}

void HybridOpenwakeword::setThreshold(double threshold) {
    threshold_ = threshold;
}

void HybridOpenwakeword::reset() {
    audio_buffer_.clear();
    melspec_buffer_.assign(76 * 32, 0.0f);
    embedding_buffer_.assign(16 * 96, 0.0f);
    melspec_frames_filled_ = 0;
    embedding_frames_filled_ = 0;
    
    if (melspec_interp_) TfLiteInterpreterAllocateTensors(melspec_interp_);
    if (embedding_interp_) TfLiteInterpreterAllocateTensors(embedding_interp_);
    if (wakeword_interp_) TfLiteInterpreterAllocateTensors(wakeword_interp_);
}

} // namespace margelo::nitro::openwakeword
