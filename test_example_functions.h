#pragma once
#include "search_server.h"

void AddDocument (SearchServer& search_server,int document_id, const std::string& document, DocumentStatus status,const std::vector<int>& rating);

void PrintDocument(const Document& document);