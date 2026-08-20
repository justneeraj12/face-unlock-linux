from __future__ import annotations


class YuNetFaceDetector:
    backend_name = "yunet"

    def __init__(self) -> None:
        raise NotImplementedError(
            "YuNet backend is planned but not implemented yet. "
            "Use --backend haar for now."
        )
