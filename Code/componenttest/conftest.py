import pytest
import os
import subprocess
import time
import signal

EVALUATION_EXE_PATH = "../evaluation/bin/evaluation"

@pytest.fixture(scope="function", autouse=True)
def start_and_stop_evaluation_component():
  assert os.path.exists(EVALUATION_EXE_PATH), "evaluation binary not found, system tests will not run"
  evaluation = subprocess.Popen([EVALUATION_EXE_PATH], text=True)
  time.sleep(0.1) # Wait for the program to start listening before testing
  yield
  assert os.system(f"ps -p {evaluation.pid} > /dev/null") == 0, "evaluation crashed before test completion"
  os.kill(evaluation.pid, signal.SIGINT)  