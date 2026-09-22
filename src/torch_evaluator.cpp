#include "bears_chess/mcts/torch_evaluator.hpp"
#include "bears_chess/mcts/encoding.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace bears_chess {

TorchEvaluator::TorchEvaluator(
    const std::string& pt2_package_path,
    const std::string& device_name,
    int threads
) : loader(pt2_package_path), device(device_name)
{
    torch::set_num_threads(threads);
    auto metadata = loader.get_metadata();
    if (std::stoul(metadata.at("input_planes")) != InputPlanes::COUNT || std::stoul(metadata.at("policy_size")) != POLICY_SIZE) {
        throw std::runtime_error("package " + pt2_package_path + " was exported using a different encoding");
    }
}

std::vector<EvalResult> TorchEvaluator::evaluate(std::span<const EvalRequest> requests) {
    const int64_t batch_size = static_cast<int64_t>(requests.size());
    torch::Tensor input = torch::empty({batch_size, InputPlanes::COUNT, 8, 8}, torch::kFloat32);
    float* input_data_ptr = input.data_ptr<float>();
    for (const EvalRequest& request : requests) {
        InputPlanes board_encoding = encode_board(request.board);
        std::span<const float> data = board_encoding.view();
        std::copy(data.begin(), data.end(), input_data_ptr);
        input_data_ptr += INPUT_SIZE;
    }

    torch::NoGradGuard no_grad;
    std::vector<torch::Tensor> outputs = loader.run({input.to(device)});
    torch::Tensor policy = outputs[0].to(torch::kCPU).contiguous();
    torch::Tensor value = outputs[1].to(torch::kCPU).contiguous();

    assert(policy.size(0) == batch_size && policy.size(1) == static_cast<int64_t>(POLICY_SIZE));
    assert(value.size(0) == batch_size);
    
    const auto policy_view = policy.accessor<float, 2>();
    const auto value_view = value.accessor<float, 2>();

    std::vector<EvalResult> results(requests.size());
    for (size_t i = 0; i < requests.size(); ++i) {
        const EvalRequest& request = requests[i];
        EvalResult& result = results[i];

        // softmax policy over valid moves
        float max_logit = -std::numeric_limits<float>::infinity();
        for (const Move& move : request.moves) {
            const float logit = policy_view[i][policy_index(move, request.board.side_to_move)];
            result.priors.emplace_back(logit);
            max_logit = std::max(max_logit, logit);
        }
        float total = 0.0f;
        for (float& prior : result.priors) {
            prior = std::exp(prior - max_logit);
            total += prior;
        }
        for (float& prior : result.priors) {
            prior /= total;
        }
        // clamp value
        result.value = std::clamp(value_view[i][0], -1.0f, 1.0f);
    }
    return results;
}

} // namespace bears_chess