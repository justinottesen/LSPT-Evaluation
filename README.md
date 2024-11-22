# Evaluation

This is the repository for the Evaluation component of the search engine being built in CSCI 6460: Large Scale Programming & Testing for the Fall 2024 semester at Rensselaer Polytechnic Institute.

This README should contain everything the other teams should need to know about the Evaluation component. Please reach out to us on the Discord with any questions, corrections, or problems.

## Build + Development Instructions

See `Code/README.md` for these instructions.

## Interface

Hello other teams! This section is for you guys. Please reach out to us with problems or questions.

### General Interface Information

Before describing the specific messages & formats we will support, here are some rules that apply to all messaging we will be involved in:

- **UDP Sockets**: All communication with our component will be through UDP socket communication. We will listen on one or more (TBD) sockets for messages, and will respond using UDP sockets. *If this is a significant problem for your component, discuss this with us, and we can try to work something else out*.
> TCP will be slow, I don't think UDP will be a problem for our case, especially because we are all running within the same network.
- **JSON**: All interactions will be done using JSON. This is for uniformity and ease of communication. There is one exception to this, the first 4 bytes will always be the length of the message (**NOT including this four byte length encoding**).
- **Error Handling**: For the beta release, very minimal error handling will be implemented. If we receive a badly formatted message, we will ignore it. We will not respond with "success" or "failure" messages. This may result in the loss of some metrics or search data, however it will significantly reduce both ther server and developer load. If you send us a message and don't receive a response when expecting one, you will have to retry sending the message.
> Not too sure if we want to not respond. Some data loss in our case is fine, and other teams probably wouldn't even be able to fix errors on the fly. Only problem here is if they are sending to the wrong socket or something. 
>
>Maybe we have separate `DEBUG` and `RELEASE` modes which respond? Or we could expand the idea of using `evaltool` as a proxy (see my note right above [Specific API Calls](#specific-api-calls)). The evaltool route is probably easier and better, that way we don't have to recompile when someone has a problem

As mentioned above, all communication will be done in JSON (with the exception of the length encoding). Below there are several examples listed of what JSON format we will be accepting. Each of these must be wrapped in the following JSON:

```
{
  "component": <Your Component Name>,
  "function": <The API "function" name you are calling>,
  "version": <The version of the API "function" that you are using>,
  "message": <The message contents as a JSON object>
}
```

Some more detail about each field:
- **component**: The name of your component. We will expect one of the following:
  - `"Crawling"`
  - `"Ranking"`
  - `"Querying"`
  - `"Doc Data Store"`
  - `"Text Transform"`
  - `"Link Analysis"`
  - `"Indexing"`
  - `"UI/UX"`
  - `"Evaluation"`
- **function**: The name of the "function" you are calling. This will be from the table found below in the [Specific API Calls](#specific-api-calls) section.
- **version**: We hope to not have to, but we may have to change the expected interface. If this happens, we will update the version number. To reduce impact on other groups, we will attempt to support current as well as previous versions, however we may not be able to for various reasons. The table in [Specific API Calls](#specific-api-calls) shows the **Current API Version** we support, as well as the **Minimum Supported Version** you may use.
- **message**: The message is the JSON object for the API "function" you are calling. These formats are described below in [Specific API Calls](#specific-api-calls).

We will be logging information with every received message to help facilitate debugging of messaging. If something is not working as you expect, please contact us on Discord, we can help troubleshoot.

> It may be worth implementing a feature in `evaltool` which simply receives and prints the received message (and maybe whether it is in compliance with our expectations).
>
> For example, command is `./evaltool.py proxy -p 9999 -f 8888` and it runs as a "proxy" evaluation component on port 9999 that just prints what it receives (and maybe forwards the message to 8888 where the actual evaluation component is running).
>
> In my experience, networking components together is one of the most annoying parts of a project like this, especially when everything is in different languages. This could be a very low effort way to significantly help with testing and debugging, and conclusively say who is the one messing up the interface.

### Specific API Calls

Below is the description of all interactions we will support. Remember that each of these JSON objects will need to be wrapped in a JSON object matching the above specification.

| API Function Name                           | Expected Callers | Purpose                                           | Current API Version | Minimum Supported Version |
|---------------------------------------------|------------------|---------------------------------------------------|---------------------|---------------------------|
| [GetAutofill](#getautofill)                 | UI/UX            | Generate completions for a partial query          | 0                   | 0                         |
| [GetQueryID](#getqueryid)                   | UI/UX            | Request a unique ID to use for a query            | 0                   | 0                         |
| [ReportSearchResults](#reportsearchresults) | UI/UX            | Report interaction data for a query               | 0                   | 0                         |
| [SubmitFeedback](#submitfeedback)           | UI/UX            | Store feedback / bug reports for admin to see     | 0                   | 0                         |
| [GetQueryData](#getquerydata)               | Ranking          | Request the interaction data for a query          | 0                   | 0                         |
| [ReportMetrics](#reportmetrics)             | All Components   | Report performance data                           | 0                   | 0                         |

> Am I missing anything? We don't have to specify what we are calling of other teams here, that is their problem.

Below is more information on the above API calls. Only documentation for the most recent version is shown. If you need help with an older version, which we hope you will not, please reach out to us (or check the commit history). If we no longer support the version, you are out of luck, and must upgrade.

#### GetAutofill

Request Format:

``` 
{
  "num_suggestions": <Number of suggestions requested>,
  "partial_query": <The query which will be autofilled>
}
```

- **num_suggestions**: The maximum number of suggestions you would like in response. We may respond with any number of autofill suggestions less than or equal to this.
- **partial_query**: The partial query you would like autofill responses to. This may also be an empty string, if just the top suggestions are wanted.

Response Format:
```
{
  "suggestions": [ <A list of suggestions for the complete query> ]
}
```
> We do not need to include the number of suggestions we are sending, that is inferred by the size of the list

> Should we wrap responses with the same JSON we expect as well? That probably makes the most sense

Side Effects:

None

#### GetQueryID

Request Format:
```
{}
```
All necessary information is in the wrapper JSON

Response Format:
```
{
  "query_ID": <Unique identifier of the query>
}
```
- **query_ID**: A number, which is guaranteed to be unique between all calls of this function, even if the Evaluation component crashes.

> Number? Hash? String? Defaulting to just a monotonically increasing number. 0, 1, 2, 3, etc...

Side Effects:

This query ID will never again be returned by `GetQueryID`.

#### ReportSearchResults

Request Format:
```
{
  "query_ID": <Unique identifier of the query>
  "raw_query": <The exact query the user entered>,
  "results": [ <A list of results shown to the user> ],
  "clicked": <Index of the result the user chose in the above list>,
  "query_timestamp": <A timestamp for the query>
}
```
- **query_ID**: Unique identifier of the query, **which must be generated by a previous call of `GetQueryID`**. This will not be checked, however the query will not be stored if there is a conflict.
- **raw_query**: This should be what the user types in the search bar, with the exception of a "did you mean", in which case the corrected query should be used. This is ultimately up to the UI team's discretion, as it will be used as the dataset that informs autofill.
- **results**: A list of the results shown to the user, in order of their display. This can be used to infer which links were clicked, and which were ignored.
> How does webgraph deal with this? Maybe their list is the links the user "previews" by clicking the node without actually following the link. This gets weird if they are re-querying and doing new subgraphs somehow

> Are results just links? Is this JSON of label, link, snippet, etc? This depends on what UI wants. Maybe v0 is just link, future versions have more? Could be hard to change this with our database
- **clicked**: This is the result that the user ultimately selected
- **query_timestamp**: The timestamp associated with the query
> Why do we need this again?

Response Format:

No response is sent.

Side Effects:

The search results and interactions will be stored. Various information will be forwarded to the Link Analysis component for updating of the webgraph.

#### SubmitFeedback

Request Format:
```
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

No response is sent.

Side Effects:

The feedback is stored for admin viewing.

#### GetQueryData

Request Format:
```
{
  "query_IDs": [ <List of unique identifiers of queries> ]
}
```
- **query_IDs**: A list of query IDs for which we will get the interaction info. If only one is needed, just supply one query ID in the list.

Response Format:
```
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
> Should we add timestamp? Should we add the raw query?

- **query_ID**: The unique identifier associated with the query. These are taken from the input. If unable to find a query with the ID provided, it will not be included in the **queries** list
- **results**: The list of results which were displayed to the user.
> See above question about results
- **clicked**: This is the result that the user ultimately selected

Side Effects:

None

#### ReportMetrics

Request Format:
```
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

Response Format:

No response is sent.

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
> Timestamp: When the query started? when it ended? Why do we even need this? I think it is for link analysis webgraph, we don't have to store it, we can just send it to them.
>
> Counterargument: We can use this to see how frequently queries occur and how much load is on the search engine

This will be split among various SQLite tables. The exact structure is to be determined.

#### Metrics Data

TBD. Depends on what we want and how we want to show it.

#### User Feedback

Also TBD but maybe just dump into files or something, does not have to be fancy.

### Implementation

Everything the evaluation component does will be started by the receive of a message on a socket. We will have a pool of threads, which wait to be assigned to an incoming request.

> Do we want one port per message type? One port, a threadpool to handle all requests? One thread per request type? Does it even matter if we only have one core and are sharing with DDS?

> How many threads? If the machine has only one core, and we are sharing with another team, how are we gonna do this?

When an incoming request is received, we will parse the wrapper JSON to see what needs to be done with the inner JSON. The correct function will be called to handle the inner JSON, propagating the necessary information from the outer JSON.

### Evaltool

We have chosen to split our component into two parts. The main component is what has been described above, and will handle all of the main responsibilities of the evaluation component. The second part is the admin and testing tool. There is a more detailed README for this in `Code/evaltool/README.md`, with instructions on each command.

This tool will provide an admin with the ability to view the data easily, and significantly help with testing the evaluation component. We plan on tying this very closely with our pytest test suite.