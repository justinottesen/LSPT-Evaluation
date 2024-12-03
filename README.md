# Evaluation

This is the repository for the Evaluation component of the search engine being built in CSCI 6460: Large Scale Programming & Testing for the Fall 2024 semester at Rensselaer Polytechnic Institute.

This README should contain everything the other teams should need to know about the Evaluation component. Please reach out to us on the Discord with any questions, corrections, or problems.

## Build + Development Instructions

See `Code/README.md` for these instructions.

## Interface

Hello other teams! This section is for you guys. Please reach out to us with problems or questions.

### General Interface Information

Before describing the specific messages & formats we will support, here are some rules that apply to all messaging we will be involved in:

- **HTTP Requests**: All communication with our component will be through HTTP. We will listen on one socket for requests. We will be using HTTP version 1.1
- **JSON**: All interactions will be done using JSON. This is for uniformity and ease of communication. So, every request body we receive should be in JSON format.
- **Error Handling**: For the beta release, very minimal error handling will be implemented. If we receive a badly formatted message, we will try to give a helpful message, however this is not a priority. If something goes wrong, read this file and if you still have problems, reach out to us.

We will be logging information with every received message to help facilitate debugging of messaging. If something is not working as you expect, please contact us on Discord, we can help troubleshoot.

If time permits, we will also implement an HTTP proxy to intercept, display, and relay requests to help with debugging. Check `Code/evaltool/README.md` for more information on this.

### Specific API Calls

Below is the description of all interactions we will support.

| API Function Name                           | Expected Callers | Purpose                                           | Current API Version | Minimum Supported Version |
|---------------------------------------------|------------------|---------------------------------------------------|---------------------|---------------------------|
| [GetAutofill](#getautofill)                 | UI/UX            | Generate completions for a partial query          | 0                   | 0                         |
| [GetQueryID](#getqueryid)                   | UI/UX            | Request a unique ID to use for a query            | 0                   | 0                         |
| [ReportSearchResults](#reportsearchresults) | UI/UX            | Report interaction data for a query               | 0                   | 0                         |
| [SubmitFeedback](#submitfeedback)           | UI/UX            | Store feedback / bug reports for admin to see     | 0                   | 0                         |
| [GetQueryData](#getquerydata)               | Ranking          | Request the interaction data for a query          | 0                   | 0                         |
| [ReportMetrics](#reportmetrics)             | All Components   | Report performance data                           | 0                   | 0                         |

Below is more information on the above API calls. Only documentation for the most recent version is shown. If you need help with an older version, which we hope you will not, please reach out to us (or check the commit history). If we no longer support the version, you are out of luck, and must upgrade.

To access an API call of a specific version, prepend `/v#/` to the resource path, where `#` is the version number. Examples are shown below.

#### GetAutofill

Request Format:
```
GET /v0/GetAutofill HTTP/1.1
Num-Suggestions: <The number of suggestions you want>
Partial-Query: <The query you would like to complete>
```

- **Num-Suggestions**: The maximum number of suggestions you would like in response. We may respond with any number of autofill suggestions less than or equal to this.
- **Partial-Query**: The partial query you would like autofill responses to. This may also be an empty string, if just the top suggestions are wanted.

Response Format:
```
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: <Length of JSON body below>

{
  "suggestions": [ <A list of suggestions for the complete query> ]
}
```

Side Effects:

None

#### GetQueryID

Request Format:
```
GET /v0/GetQueryID HTTP/1.1
```

Response Format:
```
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: <Length of JSON body below>

{
  "query_ID": <Unique identifier of the query>
}
```
- **query_ID**: A number, which is guaranteed to be unique between all calls of this function, even if the Evaluation component crashes.

Side Effects:

This query ID will never again be returned by `GetQueryID`.

#### ReportSearchResults

Request Format:
```
POST /v0/ReportSearchResults HTTP/1.1
Content-Type: application/json
Content-Length: <Length of JSON body below>

{
  "query_ID": <Unique identifier of the query>,
  "raw_query": <The exact query the user entered>,
  "results": [ <A list of results shown to the user> ],
  "clicked": <Index of the result the user chose in the above list>,
  "query_timestamp": <A timestamp for the query>
}
```
- **query_ID**: Unique identifier of the query, **which must be generated by a previous call of `GetQueryID`**. This will not be checked, however the query will not be stored if there is a conflict.
- **raw_query**: This should be what the user types in the search bar, with the exception of a "did you mean", in which case the corrected query should be used. This is ultimately up to the UI team's discretion, as it will be used as the dataset that informs autofill.
- **results**: A list of the results shown to the user, in order of their display. This can be used to infer which links were clicked, and which were ignored.
- **clicked**: This is the result that the user ultimately selected
- **query_timestamp**: The timestamp associated with the query

Response Format:

```
HTTP/1.1 200 OK
```

Side Effects:

The search results and interactions will be stored. Various information will be forwarded to the Link Analysis component for updating of the webgraph.

#### SubmitFeedback

Request Format:
```
POST /v0/SubmitFeedback HTTP/1.1
Content-Type: application/json
Content-Length: <Length of JSON body below>

{
  "label": <Some Label for the feedback>,
  "title": <Some title for the feedback>,
  "text": <Some text for the feedback>
}
```
We are flexible on this, and expect this method to be used for bug reports, right to be forgotten requests, and general user feedback. None of this will be automatically processed, it will be entirely admin driven.
- **label**: A label for the type of feedback. Maybe "bug report", "review", "right to be forgotten" or something similar. This does not have to be consistent across feedback of the same "type"
- **title**: A title for the feedback. Maybe a short description of what it is, what the problem is, etc.
- **text**: More detailed info than the title. Maybe steps to reproduce a bug, reasons for certain feedback, or extra data that is recorded about the user that gets stuffed into a string for this field

Response Format:

```
HTTP/1.1 200 OK
```

Side Effects:

The feedback is stored for admin viewing.

#### GetQueryData

Request Format:
```
GET /v0/GetQueryData HTTP/1.1
Query-ID: <Query ID of interest>
```

Response Format:
```
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: <Length of JSON body below>

{
  "queries": [
    {
      "query_ID": <Unique identifier of the query>,
      "results": [ <A list of results shown to the user> ],
      "clicked": <Index of the result the user chose in the above list>
    }, 
    <More queries in the above format>
  ]
}
```
- **query_ID**: The unique identifier associated with the query. These are taken from the input. If unable to find a query with the ID provided, it will not be included in the **queries** list
- **results**: The list of results which were displayed to the user.
- **clicked**: This is the result that the user ultimately selected

Side Effects:

None

#### ReportMetrics

Request Format:
```
POST /v0/ReportMetrics HTTP/1.1
Component: <Agreed upon identifier for your component & Team>
Content-Type: application/json
Content-Length: <Length of JSON body below>

{
  "metrics": [
    {
      "label": <Label for the metric>
      "value": <Value for the metric>
    },
    <More metrics in the above format>
  ]
}
```
We are accepting a list so that metrics can be grouped together and batched, which may help performance.
- **label**: The label for the metric. We will establish with other teams what metrics we would like to record, so this label will need to be an agreed upon string identifier.
- **value**: Again, this will depend on the metric itself. It may be a number, a list, a JSON object, or some other value.

The `Component` header value should be your component and team name.

Response Format:

```
HTTP/1.1 200 OK
```

Side Effects:

The metrics data is processed and stored for later use

## Metrics

TBD. We will communicate with other teams to establish what metrics we expect, and how to format their sending. They will be sent to us using the [ReportMetrics](#reportmetrics) API call.

## Design

### Data

The evaluation component is effectively a wrapper around stored data related to the search engine function & performance. The two main stores of data we maintain are:

- **Search History**: A list of all searches and interaction data from the users 
- **Metrics Data**: A collection of recorded metrics from other components which can be used to judge the performance of the engine.

In addition to these two main data stores, we also store **User Feedback**: This will hold any information that the user provides with the expectation of admin action in response. This may include (but is not limited to):

- **Bug Reports**: If the user encounters an issue and would like to report a bug, this report will be stored. The admins who read the report should reach out to the appropriate team to help diagnose and fix the issue
- **Right to be Forgotten**: A big recurring discussion around this search engine is that the user should be able to have their data scrubbed from the data store. These requests will also be stored here.
- **General Feedback**: The user will also have the option to provide general feedback, this will also be stored here.

The **Search History** will be stored in an SQLite table. **Metrics Data** is still somewhat up in the air, but we will likely do the same here. **User Feedback** is less structured, and will only be accessed by admin, so it will likely be stored in human readable text files, with the format TBD.

> How & Where are we storing everything?

#### Search History

The search history will be stored as a list of user queries, and the associated action that was taken from them. Each stored query will have the following data:

- **Query ID**: This is a unique identifier for each query, and is stored at request of the Ranking team
- **Raw Query**: The exact value the user entered for the search
- **Results**: The list of results which were shown to the user as a result of this query
- **Clicked**: Which result the user chose
- **Timestamp**: A timestamp associated with the query

This will be split among various SQLite tables. The exact structure is to be determined.

#### Metrics Data

TBD. Depends on what we want and how we want to show it.

#### User Feedback

Also TBD but maybe just dump into files or something, does not have to be fancy.

### Implementation

Everything the evaluation component does will be started by the receive of a message on a socket. We will have a pool of threads, which wait to be assigned to an incoming request.



When an incoming request is received, we will parse the wrapper JSON to see what needs to be done with the inner JSON. The correct function will be called to handle the inner JSON, propagating the necessary information from the outer JSON.

### Evaltool

We have chosen to split our component into two parts. The main component is what has been described above, and will handle all of the main responsibilities of the evaluation component. The second part is the admin and testing tool. There is a more detailed README for this in `Code/evaltool/README.md`, with instructions on each command.

This tool will provide an admin with the ability to view the data easily, and significantly help with testing the evaluation component. We plan on tying this very closely with our pytest test suite.