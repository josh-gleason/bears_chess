import os

import pytest

import bears_chess as bc

if not bc.HAS_TORCH:
    pytest.skip("bears_chess built without torch support", allow_module_level=True)

import torch
import torch.nn as nn

from ursus_rasa.export import export_model

MATE_IN_1 = "6k1/5ppp/8/8/8/8/8/4R2K w - - 0 1"
POSITIONS = [bc.INITIAL_POSITION_FEN, bc.PERFT_POSITION_2_FEN, bc.PERFT_POSITION_5_FEN, MATE_IN_1]

DEVICES = ["cpu"]
if torch.cuda.is_available() and os.environ.get("CUDA_HOME"):
    DEVICES.append("cuda")


class TinyNet(nn.Module):
    def __init__(self, policy_size=bc.POLICY_SIZE):
        super().__init__()
        self.conv = nn.Conv2d(bc.INPUT_PLANES, 8, 3, padding=1)
        self.policy = nn.Linear(8 * 64, policy_size)
        self.value = nn.Linear(8 * 64, 1)

    def forward(self, x):
        h = torch.relu(self.conv(x)).flatten(1)
        return self.policy(h), torch.tanh(self.value(h))


def make_net(seed=0, **kwargs):
    torch.manual_seed(seed)
    return TinyNet(**kwargs).eval()


def reference(net, board):
    with torch.no_grad():
        logits, value = net(torch.from_numpy(bc.encode_board(board)).unsqueeze(0))
    indices = torch.tensor([bc.policy_index(board, move) for move in board.legal_moves()])
    return torch.softmax(logits[0][indices], dim=0).tolist(), value.item()


@pytest.mark.parametrize("device", DEVICES)
def test_torch_mcts_matches_pytorch(tmp_path, device):
    net = make_net()
    package = export_model(make_net(), tmp_path / f"net_{device}.pt2", device)
    mcts = bc.TorchMCTS(str(package), device=device)
    for fen in POSITIONS:
        board = bc.Board(fen)
        priors, value = reference(net, board)
        result = mcts.go(board, 0)
        assert [s.move for s in result.moves] == board.legal_moves()
        assert [s.prior for s in result.moves] == pytest.approx(priors, abs=1e-5)
        assert result.eval_value == pytest.approx(value, abs=1e-4)


def test_torch_mcts_batching_finds_mate(tmp_path):
    package = export_model(make_net(), tmp_path / "net.pt2", "cpu")
    for batch_size in [1, 8]:
        result = bc.TorchMCTS(str(package), batch_size=batch_size).go(bc.Board(MATE_IN_1), 200)
        assert result.simulations == 200
        best = max(result.moves, key=lambda s: s.visits)
        assert best.move.uci == "e1e8" and best.q == 1.0


def test_export_rejects_mismatched_policy_size(tmp_path):
    with pytest.raises(ValueError):
        export_model(make_net(policy_size=bc.POLICY_SIZE + 1), tmp_path / "bad.pt2", "cpu")
