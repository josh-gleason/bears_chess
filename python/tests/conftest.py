import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

try:
    from _module_location import MODULE_DIR
except ImportError as e:
    raise ImportError(
        "python/tests/_module_location.py not found; configure the build first "
        "(cmake --preset release -DBUILD_PYTHON_BINDINGS=ON)"
    ) from e

sys.path.insert(0, MODULE_DIR)
