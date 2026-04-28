#include "HybridOpenwakeword.hpp"
#include <iostream>

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

DetectionResult HybridOpenwakeword::processFrame(const std::shared_ptr<ArrayBuffer>& buffer) {
    if (!melspec_interp_ || !embedding_interp_ || !wakeword_interp_) {
        return DetectionResult(0.0, false);
    }
    
    // 1. Copy audio buffer to Melspec Input
    TfLiteTensor* melspec_input = TfLiteInterpreterGetInputTensor(melspec_interp_, 0);
    
    // Ensure we don't overflow the input tensor
    size_t input_byte_size = TfLiteTensorByteSize(melspec_input);
    size_t copy_size = std::min((size_t)buffer->size(), input_byte_size);
    
    // Convert 16-bit PCM to Float32 if required by the model, otherwise copy directly.
    // OpenWakeWord tflite models typically take float32 input shape [1, 1280] or similar.
    if (TfLiteTensorType(melspec_input) == kTfLiteFloat32) {
        int16_t* pcm_data = (int16_t*)buffer->data();
        float* input_data = (float*)TfLiteTensorData(melspec_input);
        size_t num_samples = copy_size / 2;
        for (size_t i = 0; i < num_samples; i++) {
            input_data[i] = pcm_data[i] / 32768.0f;
        }
    } else {
        // Fallback raw copy
        TfLiteTensorCopyFromBuffer(melspec_input, buffer->data(), copy_size);
    }
    
    if (TfLiteInterpreterInvoke(melspec_interp_) != kTfLiteOk) return DetectionResult(0.0, false);
    const TfLiteTensor* melspec_output = TfLiteInterpreterGetOutputTensor(melspec_interp_, 0);
    
    // 2. Feed Melspec Output to Embedding Input
    TfLiteTensor* embedding_input = TfLiteInterpreterGetInputTensor(embedding_interp_, 0);
    TfLiteTensorCopyFromBuffer(embedding_input, TfLiteTensorData(melspec_output), TfLiteTensorByteSize(melspec_output));
    
    if (TfLiteInterpreterInvoke(embedding_interp_) != kTfLiteOk) return DetectionResult(0.0, false);
    const TfLiteTensor* embedding_output = TfLiteInterpreterGetOutputTensor(embedding_interp_, 0);
    
    // 3. Feed Embedding Output to Wakeword Input
    TfLiteTensor* wakeword_input = TfLiteInterpreterGetInputTensor(wakeword_interp_, 0);
    TfLiteTensorCopyFromBuffer(wakeword_input, TfLiteTensorData(embedding_output), TfLiteTensorByteSize(embedding_output));
    
    if (TfLiteInterpreterInvoke(wakeword_interp_) != kTfLiteOk) return DetectionResult(0.0, false);
    const TfLiteTensor* wakeword_output = TfLiteInterpreterGetOutputTensor(wakeword_interp_, 0);
    
    // 4. Get probability
    float probability = ((float*)TfLiteTensorData(wakeword_output))[0];
    
    return DetectionResult(probability, probability >= threshold_);
}

void HybridOpenwakeword::setThreshold(double threshold) {
    threshold_ = threshold;
}

void HybridOpenwakeword::reset() {
    // TFLite models for OWW often have internal states (if exported as stateful).
    // Resetting them typically requires re-allocating or invoking a specific reset op.
    // For now, re-allocating tensors clears stateful RNN/LSTM buffers in TFLite.
    if (melspec_interp_) TfLiteInterpreterAllocateTensors(melspec_interp_);
    if (embedding_interp_) TfLiteInterpreterAllocateTensors(embedding_interp_);
    if (wakeword_interp_) TfLiteInterpreterAllocateTensors(wakeword_interp_);
}

} // namespace margelo::nitro::openwakeword
