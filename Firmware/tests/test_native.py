"""Run this standalone PR's hardware-free production-source checks."""
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
for script in ("test_protocol_access.py", "test_shared_format.py", "test_messages.py"):
    subprocess.run([sys.executable, "-B", str(HERE / script)], check=True)
print("Standalone shared-machinery native battery PASS (no hardware access)")
