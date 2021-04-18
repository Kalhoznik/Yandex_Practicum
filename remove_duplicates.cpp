#include "search_server.h"
#include <iostream>
#include <set>

using namespace std;
void RemoveDuplicates(SearchServer &search_server)
{
  set<std::string> words;
  set<set<string>> seen;

  for(auto begin = search_server.begin();begin != search_server.end();){

    const auto &words_to_freq = search_server.GetWordFrequencies(*begin);

    for(const auto& word:words_to_freq){
      words.insert(word.first);
    }

    if(seen.insert(words).second){
      ++begin;
    }else{
      cout<<"Found duplicate document id "<<*begin<<endl;
      search_server.RemoveDocument(*begin);
    }
    words.clear();
  }

}
