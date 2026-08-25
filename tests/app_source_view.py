from pathlib import Path

APP_SOURCE_FILES = (
    "src/main_round5.cpp",
    "src/app_runtime_state.ipp",
    "src/app_audit.ipp",
    "src/app_window.ipp",
    "src/app_entry.ipp",
)


def read_app_source(root: Path) -> str:
    """Return the exact production app-entry source view compiled by main_round5.cpp."""
    chunks = []
    for relative in APP_SOURCE_FILES:
        path = root / relative
        if not path.exists():
            raise FileNotFoundError(f"production app source missing: {relative}")
        chunks.append(path.read_text(encoding="utf-8"))
    return "\n".join(chunks)
