#pragma once
#include <vector>
#include "HybridOpenwakewordSpec.hpp"

namespace margelo::nitro::openwakeword {
class HybridOpenwakeword : public HybridOpenwakewordSpec {
    public:
        HybridOpenwakeword() : HybridObject(TAG), HybridOpenwakewordSpec() {}
       
        double sum(double a, double b) override;
    };
} // namespace margelo::nitro::openwakeword
