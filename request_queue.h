#pragma once
#include "search_server.h"
#include <vector>
#include <deque>
class RequestQueue {
public:
  explicit RequestQueue(SearchServer& search_server);
  // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
  template <typename DocumentPredicate>
  std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    ++current_time;

    if((current_time > sec_in_day_) && !requests_.empty()){
      if(!requests_.back().IsEmpty)
        --empty_query_result_count;
      requests_.pop_back();
    }


    QueryResult result;
    auto documents =  server.FindTopDocuments(raw_query,document_predicate);
    if(documents.empty()){
      result = {documents,true};
      ++empty_query_result_count;
    }
    result = {documents,false};
    requests_.push_front(result);
    return result.find_documents;
  }

  std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

  std::vector<Document> AddFindRequest(const std::string& raw_query);

  int GetNoResultRequests() const;
private:
  struct QueryResult {
    std:: vector<Document> find_documents;
    bool IsEmpty;
  };
  std::deque<QueryResult> requests_;
  const static int sec_in_day_ = 1440;
  int  current_time  = 0;
  const SearchServer& server;
  size_t empty_query_result_count =0;
};
