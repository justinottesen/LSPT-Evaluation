#!/usr/bin/python3 

import argparse
import requests
from typing import Any
import json

def get_url(ip: str, port: int, path: str):
  return f"http://{ip}:{port}{path}"

def make_request(ip: str, port: int, method: str, path: str, headers: dict[str, str] = {}, body: Any | None = None) -> None:
  url = get_url(ip, port, path)

  response = requests.request(method, url, headers=headers, json=None if not body else json.loads(body))

  print("HTTP/1.1", response.status_code, response.reason)
  for h, v in response.headers.items():
    print(f"{h}: {v}")
  print()
  if response.content: print(response.content.decode())

def cmd_get_autofill(args) -> None:
  ip = args.ip
  port = args.port

  partial_query = args.partial_query
  num_suggestinos = args.num_suggestions

  headers = {
    "partial_query": partial_query,
    "num_suggestions": str(num_suggestinos)
  }

  make_request(ip, port, "GET", "/v0/GetAutofill", headers = headers)

def cmd_get_query_id(args) -> None:
  ip = args.ip
  port = args.port

  make_request(ip, port, "GET", "/v0/GetQueryID")

def cmd_report_search_results(args) -> None:
  ip = args.ip
  port = args.port

  results_json = args.results_json

  make_request(ip, port, "POST", "/v0/ReportSearchResults", body = results_json)

def cmd_submit_feedback(args) -> None:
  ip = args.ip
  port = args.port

  feedback_json = args.feedback_json

  make_request(ip, port, "POST", "/v0/SubmitFeedback", body = feedback_json)

def cmd_get_query_data(args) -> None:
  ip = args.ip
  port = args.port

  query_ID = args.query_ID

  headers = {
    "query_ID": str(query_ID)
  }

  make_request(ip, port, "GET", "/v0/GetQueryData", headers = headers)

def cmd_report_metrics(args) -> None:
  ip = args.ip
  port = args.port

  component = args.component
  metrics_json = args.metrics_json

  headers = {
    "component": component
  }

  make_request(ip, port, "POST", "/v0/SubmitFeedback", headers = headers, body = metrics_json)

def cmd_proxy(args) -> None:
  raise NotImplementedError

if __name__ == "__main__":
  parser = argparse.ArgumentParser(
        description="An admin tool for testing and interfacing with the evaluation component"
    )
  
  parser.add_argument("--ip", type=str, default="127.0.0.1", required=True, help="The IP the evaluation component is running on")
  parser.add_argument("--port", "-p", type=int, default=8080, required=True, help="The port the evaluation component is running on")

  subparsers = parser.add_subparsers(
    title="commands",
    description="Available operations",
    dest="command", 
    required=True
  )

  # GetAutofill
  parser_autofill = subparsers.add_parser("GetAutofill", help = "Get autofill results for a partial query")
  parser_autofill.add_argument("partial_query", type=str, help="A partial query to generate autofill for")
  parser_autofill.add_argument("num_suggestions", type=int, help="The maximum number of responses wanted")

  # GetQueryID
  parser_getQueryID = subparsers.add_parser("GetQueryID", help = "Gets a unique query ID")
  
  # ReportSearchResults
  parser_reportSearchResults = subparsers.add_parser("ReportSearchResults", help="Report search results to the database")
  parser_reportSearchResults.add_argument("results_json", type=str, help = "The JSON body including all necessary information")

  # SubmitFeedback
  parser_submitFeedback = subparsers.add_parser("SubmitFeedback", help = "Submit user feedback to the component")
  parser_submitFeedback.add_argument("feedback_json", type=str, help = "The JSON body including all necessary information")

  # GetQeuryData
  parser_getQueryData = subparsers.add_parser("GetQueryData", help = "Gets the data associated with a query ID")
  parser_getQueryData.add_argument("query_ID", type=int, help="The unique identifier of the query of interest")

  # ReportMetrics
  parser_reportMetrics = subparsers.add_parser("ReportMetrics", help="Submit component metrics to the datastore")
  parser_reportMetrics.add_argument("component", type=str, help="The name of the component submitting metrics")
  parser_reportMetrics.add_argument("metrics_json", type=str, help = "The JSON body including all necessary information")

  # Proxy
  parser_proxy = subparsers.add_parser("proxy", help = "Intercept, print, and relay all messages sent to the component")
  parser_proxy.add_argument("proxy_port", type=int, help="Which port to listen for messages on")

  args = parser.parse_args()

  command_handler = {
    "GetAutofill": cmd_get_autofill,
    "GetQueryID": cmd_get_query_id,
    "ReportSearchResults": cmd_report_search_results,
    "SubmitFeedback": cmd_submit_feedback,
    "GetQueryData": cmd_get_query_data,
    "ReportMetrics": cmd_report_metrics,
    "proxy": cmd_proxy
  }

  if args.command in command_handler:
    command_handler[args.command](args)
  else:
    parser.print_help()