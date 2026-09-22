#pragma once

#include "bears_chess/mcts/evaluation.hpp"

#include <torch/torch.h>
#include <torch/csrc/inductor/aoti_package/model_package_loader.h>

#include <vector>
#include <string>

namespace bears_chess {

class TorchEvaluator {  
public:
    TorchEvaluator(const std::string& pt2_package_path, const std::string& device_name, int threads = 1);
    std::vector<EvalResult> evaluate(std::span<const EvalRequest> requests);
private:
    torch::inductor::AOTIModelPackageLoader loader;
    torch::Device device;
};

}