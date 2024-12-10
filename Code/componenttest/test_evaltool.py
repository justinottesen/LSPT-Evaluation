import json
import subprocess
from typing import Any
import pytest

EVALTOOL_PATH = "../evaltool/evaltool.py"

def run_evaltool(cmd: str, *args) -> subprocess.CompletedProcess[str]:
  return subprocess.run([EVALTOOL_PATH, "--ip", "127.0.0.1", "--port", "8080", cmd] + [str(arg) for arg in args], 
                        capture_output=True, text=True)

def parse_response(http_response: str) -> tuple[str, dict[str, str], dict[str, Any]]:
  lines = http_response.splitlines()
  start_line = lines[0]
  empty_line = lines.index('')
  header_lines = lines[1:empty_line]
  body_lines = [] if empty_line == len(lines) else lines[empty_line + 1:]

  header_dict = {}
  for line in header_lines:
    header, value = line.split(":", 1)
    header_dict[header] = value

  body_dict = json.loads("".join(body_lines)) if len(body_lines) > 0 else dict()
  return start_line, header_dict, body_dict

@pytest.mark.parametrize("num_suggestions", [0, 1, 3])
def test_get_autofill(num_suggestions: int):
  proc = run_evaltool("GetAutofill", "How do I?", num_suggestions)
  start_line, headers, body = parse_response(proc.stdout)
  assert start_line == "HTTP/1.1 200 OK", "Received non-OK HTTP Response"
  assert len(body["suggestions"]) == num_suggestions, "Received incorrect number of suggestions"

def test_get_query_id():
  proc = run_evaltool("GetQueryID")
  start_line, headers, body = parse_response(proc.stdout)
  assert start_line == "HTTP/1.1 200 OK", "Received non-OK HTTP Response"
  start_id = body["query_ID"]
  for id in range(1, 4):
    proc = run_evaltool("GetQueryID")
    start_line, headers, body = parse_response(proc.stdout)
    assert start_line == "HTTP/1.1 200 OK", "Received non-OK HTTP Response"
    assert body["query_ID"] == start_id + id
  
def test_report_search_results():
  results_json = {
    "query_ID": 12345,
    "raw_query": "best pizza places",
    "results": ["Pizza Palace", "Italian Bistro", "Gourmet Pizza Co."],
    "clicked": 1,
    "query_timestamp": "2024-12-08T12:34:56Z"
  }

  proc = run_evaltool("ReportSearchResults", json.dumps(results_json))
  start_line, headers, body = parse_response(proc.stdout)
  print(proc.stdout)
  assert start_line == "HTTP/1.1 200 OK", "Received non-OK HTTP Response"
  assert len(body) == 0

def test_submit_feedback():
  feedback_json = {
    "label": "Bug Report",
    "title": "The autofill reuslts are always the same",
    "text": "Regardless of the partial query and num suggestions provided, the same 3 autofill responses are always provided"
  }

  proc = run_evaltool("SubmitFeedback", json.dumps(feedback_json))
  start_line, headers, body = parse_response(proc.stdout)
  assert start_line == "HTTP/1.1 200 OK", "Received non-OK HTTP Response"
  assert len(body) == 0

def test_report_metrics():
  metrics_json = {
    "metrics": [
      {"label": "Bytes Used"},
      {"value": 7295878476}
    ]
  }

  proc = run_evaltool("ReportMetrics", "Document-Data-Store", json.dumps(metrics_json))
  start_line, headers, body = parse_response(proc.stdout)
  assert start_line == "HTTP/1.1 200 OK", "Received non-OK HTTP Response"
  assert len(body) == 0
