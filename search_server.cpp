#include "search_server.h"
#include <cmath>

SearchServer::SearchServer(const std::string& stop_words_text)
	: SearchServer(SplitIntoWords(stop_words_text)) {

}

SearchServer::SearchServer(std::string_view stop_words_text)
	: SearchServer(SplitIntoWords(stop_words_text)) {

}
void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
	if ((document_id < 0) || (documents_.count(document_id) > 0)) {
		throw std::invalid_argument("Invalid document_id");
	}
	const auto& words = SplitIntoWordsNoStop(document);
	
	const double inv_word_count = 1.0 / words.size();
	for (const std::string_view& word : words) {
		const std::string word_ = std::string(word);
		word_to_document_freqs_[word_][document_id] += inv_word_count;

		const auto& view_word = word_to_document_freqs_.find(word_)->first;
		document_to_word_freq_[document_id][view_word] += inv_word_count;
	}
	documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
	document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
	return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
		return document_status == status;
	});
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
	return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}
/////paralel vesion implimentation
std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy& policy, std::string_view raw_query, DocumentStatus status) const
{
	return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
		return document_status == status;
	});
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy& policy, std::string_view raw_query) const
{
	return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

/////
std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy& policy, std::string_view raw_query, DocumentStatus status) const
{
	return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
		return document_status == status;
	});
}
std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy& policy, std::string_view raw_query) const
{
	return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
	return documents_.size();
}

std::set<int>::iterator SearchServer::begin() const
{
	return document_ids_.cbegin();
}

std::set<int>::iterator SearchServer::end() const
{
	return document_ids_.cend();
}

const std::map<std::string_view, double> &SearchServer::GetWordFrequencies(int document_id) const
{
	if (documents_.count(document_id) == 0) {
		static const std::map<std::string_view, double> empty_map;
		return empty_map;
	}
	return document_to_word_freq_.at(document_id);
}


std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query, int document_id) const {

	if (document_to_word_freq_.count(document_id) == 0) {
		throw std::out_of_range("passed a nonexistent document_id");
	}

	const auto query = ParseQuery(raw_query);

	std::vector<std::string_view> matched_words;

	for (const std::string_view& word : query.plus_words) {
		const std::string word_(word);
		const auto& find_item = word_to_document_freqs_.find(word_);
		if (find_item == word_to_document_freqs_.end()) {
			continue;
		}
		if (word_to_document_freqs_.count(std::string(word)) == 0) {
			continue;
		}
		if (word_to_document_freqs_.at(word_).count(document_id)) {
			matched_words.push_back(find_item->first);
		}
	}

	for (const std::string_view& word : query.minus_words) {

		const std::string word_(word);
		const auto& find_item = word_to_document_freqs_.find(word_);
		if (find_item == word_to_document_freqs_.end()) {
			continue;
		}
				
		if (word_to_document_freqs_.at(word_).count(document_id)) {
			matched_words.clear();
			break;
		}
	}

	return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy& policy,
	std::string_view raw_query, int document_id) const {

	return GeneralizedMatchDocument(policy, raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy& policy,
	std::string_view raw_query, int document_id) const {

	return GeneralizedMatchDocument(policy, raw_query, document_id);
}

void SearchServer::RemoveDocument(int document_id)
{
	const auto& words = document_to_word_freq_.at(document_id);
	for (const auto&[word, _] : words) {
		word_to_document_freqs_[std::string(word)].erase(document_id);
	}
	document_to_word_freq_.erase(document_id);
	documents_.erase(document_id);
	document_ids_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy& policy, int document_id) {
	GeneralizedRemoveDocument(policy, document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy& policy, int document_id) {
	GeneralizedRemoveDocument(policy, document_id);
}


bool SearchServer::IsStopWord(std::string_view word) const {
	return stop_words_.count(std::string(word)) > 0;
}

bool SearchServer::IsValidWord(std::string_view word) {
	// A valid word must not contain special characters
	return std::none_of(word.begin(), word.end(), [](char c) {
		return c >= '\0' && c < ' ';
	});
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
	std::vector<std::string_view> words;
	for (const std::string_view& word : SplitIntoWords(text)) {
		if (!IsValidWord(word)) {
			throw std::invalid_argument("Word " + static_cast<std::string>(word) + " is invalid");
		}
		if (!IsStopWord(word)) {
			words.push_back(word);
		}
	}
	return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
	if (ratings.empty()) {
		return 0;
	}
	int rating_sum = 0;
	for (const int rating : ratings) {
		rating_sum += rating;
	}
	return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
	if (text.empty()) {
		throw std::invalid_argument("Query word is empty");
	}
	std::string_view word = text;
	bool is_minus = false;
	if (word[0] == '-') {
		is_minus = true;
		word = word.substr(1);
	}
	if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
		throw std::invalid_argument("Query word " + static_cast<std::string>(text) + " is invalid");
	}

	return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text) const {
	Query result;
	for (const std::string_view& word : SplitIntoWords(text)) {
		const auto query_word = ParseQueryWord(word);
		if (!query_word.is_stop) {
			if (query_word.is_minus) {
				result.minus_words.insert(query_word.data);
			}
			else {
				result.plus_words.insert(query_word.data);
			}
		}
	}
	return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
	return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
