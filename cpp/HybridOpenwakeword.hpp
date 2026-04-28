#pragma once

#include <vector>
#include <string>
#include "HybridOpenwakewordSpec.hpp"
#include <tensorflow/lite/c/c_api.h>

namespace margelo::nitro::openwakeword {
class HybridOpenwakeword : public HybridOpenwakewordSpec {
    public:
        HybridOpenwakeword() : HybridObject(TAG), HybridOpenwakewordSpec(), threshold_(0.5) {}
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

        void cleanupModels();
    };
} // namespace margelo::nitro::openwakeword
