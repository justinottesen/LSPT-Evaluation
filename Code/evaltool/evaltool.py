#!/usr/bin/python3 

import argparse

def cmd_get_autofill(args) -> None:
  raise NotImplementedError

def cmd_get_query_id(args) -> None:
  raise NotImplementedError

def cmd_report_search_results(args) -> None:
  raise NotImplementedError

def cmd_submit_feedback(args) -> None:
  raise NotImplementedError

def cmd_get_query_data(args) -> None:
  raise NotImplementedError

def cmd_report_metrics(args) -> None:
  raise NotImplementedError

def cmd_proxy(args) -> None:
  raise NotImplementedError

if __name__ == "__main__":
  parser = argparse.ArgumentParser(
        description="An admin tool for testing and interfacing with the evaluation component"
    )
  
  parser.add_argument("--ip", type=str, default="127.0.0.1", help="The IP the evaluation component is running on")
  parser.add_argument("--port", "-p", type=int, default=8080, help="The port the evaluation component is running on")

  subparsers = parser.add_subparsers(
    title="commands",
    description="Available operations",
    dest="command", 
    required=True
  )

  # GetAutofill
  parser_autofill = subparsers.add_parser("GetAutofill", help = "Get autofill results for a partial query")
  parser_autofill.add_argument("partial_query", type=str, help="A partial query to generate autofill for")
  parser_autofill.add_argument("num_suggestions", type=str, help="The maximum number of responses wanted")

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

  command_handler[args.command](args)