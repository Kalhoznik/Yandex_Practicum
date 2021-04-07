#include "request_queue.h"

RequestQueue::RequestQueue(SearchServer& search_server)
    :server(search_server){
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
  return AddRequest(server.FindTopDocuments(raw_query,status));
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
  return AddRequest(server.FindTopDocuments(raw_query));
}

int RequestQueue::GetNoResultRequests() const {
  return empty_query_result_count;
}

std::vector<Document> RequestQueue::AddRequest(const std::vector<Document>& documents)
{
  ++current_time;
  if((current_time > sec_in_day_) && !requests_.empty()){
    if(requests_.back().IsEmpty)
      --empty_query_result_count;
    requests_.pop_back();
  }

  QueryResult result;
  if(documents.empty()){
    result = {documents,true};
    ++empty_query_result_count;
  }else{
    result = {documents,false};
  }
  requests_.push_front(result);
  return result.find_documents;
}

