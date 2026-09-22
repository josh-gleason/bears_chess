from pathlib import Path
import torch
import torch.nn as nn

import bears_chess as bc

INPUT_SHAPE = (bc.INPUT_PLANES, 8, 8)


def export_model(model: nn.Module, path: str | Path, device: str = "cpu") -> Path:
    """ Compile model into a .pt2 package that TorchMCTS can load """
    model = model.eval().to(device)
    example = torch.zeros(2, *INPUT_SHAPE, device=device)

    with torch.no_grad():
        outputs = model(example)

    if not (isinstance(outputs, tuple) and len(outputs) == 2):
        raise ValueError("model must return (policy_logits, value)")

    policy, value = outputs

    batch_size = example.shape[0]
    if tuple(policy.shape) != (batch_size, bc.POLICY_SIZE):
        raise ValueError(f"policy output must have shape (batch, {bc.POLICY_SIZE}), got {tuple(policy.shape)}")
    if tuple(value.shape) != (batch_size, 1):
        raise ValueError(f"value output must have shape (batch, 1), got {tuple(value.shape)}")

    batch = torch.export.Dim("batch", min=1)
    program = torch.export.export(model, (example,), dynamic_shapes={"x": {0: batch}})

    metadata = {
        "input_planes": str(bc.INPUT_PLANES),
        "policy_size": str(bc.POLICY_SIZE),
        "device": device,
    }
    return Path(torch._inductor.aoti_compile_and_package(
        program,
        package_path=str(path),
        inductor_configs={"aot_inductor.metadata": metadata},
    ))
