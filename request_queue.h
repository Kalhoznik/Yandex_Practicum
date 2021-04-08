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
    return AddRequest(server.FindTopDocuments(raw_query,document_predicate));
  }

  std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

  std::vector<Document> AddFindRequest(const std::string& raw_query);

  int GetNoResultRequests() const;
private:
  struct QueryResult {
    std:: vector<Document> find_documents;
    bool is_empty;
  };
  std::deque<QueryResult> requests_;
  std::vector<Document> AddRequest(const std::vector<Document>& documents);

  const static int sec_in_day_ = 1440;
  int  current_time  = 0;
  const SearchServer& server;
  size_t empty_query_result_count =0;
};
