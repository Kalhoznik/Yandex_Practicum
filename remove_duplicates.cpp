#include "remove_duplicates.h"
#include "search_server.h"
#include <iostream>
#include <set>

using namespace std;
void RemoveDuplicates(SearchServer &search_server)
{
  set<set<string>> wrods_in_document;
  vector<int> duplicate_document_id;

  for(auto document_id = search_server.begin();document_id != search_server.end(); ++document_id){

    const auto &words_to_freq = search_server.GetWordFrequencies(*document_id);
    set<string> words;
    for(const auto& word:words_to_freq){
      words.insert(word.first);
    }

    if(!wrods_in_document.insert(words).second){
      duplicate_document_id.push_back(*document_id);
    }
  }
  for(const auto doc_id: duplicate_document_id){
    cout<<"Found duplicate document id "<<doc_id<<endl;
    search_server.RemoveDocument(doc_id);
  }
}
