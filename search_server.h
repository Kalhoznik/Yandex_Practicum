#pragma once
#include "log_duration.h"
#include "string_processing.h"
#include "document.h"
#include "concurrent_map.h"
#include <tuple>
#include <map>
#include <set>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <execution>
#include <string_view>

const int MAX_RESULT_DOCUMENT_COUNT = 5;

enum class DocumentStatus {
	ACTUAL,
	IRRELEVANT,
	BANNED,
	REMOVED,
};

class SearchServer {
public:
	template <typename StringContainer>
	explicit SearchServer(const StringContainer& stop_words)
		: stop_words_(MakeUniqueNonEmptyStrings(stop_words))
	{
		if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
			throw std::invalid_argument("Some of stop words are invalid");
		}
	}

	explicit SearchServer(std::string_view stop_words_text);

	explicit SearchServer(std::string stop_words_text);

	void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);


	template <typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
		const auto query = ParseQuery(raw_query);
		auto matched_documents = FindAllDocuments(query, document_predicate);

		std::sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
			if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
				return lhs.rating > rhs.rating;
			}
			else {
				return lhs.relevance > rhs.relevance;
			}
		});
		if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
			matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
		}
		return matched_documents;
	}

	std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

	std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

	//parallel version
	template <typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(const std::execution::parallel_policy& policy, std::string_view raw_query, DocumentPredicate document_predicate) const {

		return GeneralizedFindTopDocuments(policy, raw_query, document_predicate);
	}

	std::vector<Document> FindTopDocuments(const std::execution::parallel_policy& policy, std::string_view raw_query, DocumentStatus status) const;

	std::vector<Document> FindTopDocuments(const std::execution::parallel_policy& policy, std::string_view raw_query) const;
	//////////////////////////


	template <typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy& policy, std::string_view raw_query, DocumentPredicate document_predicate) const {

		return GeneralizedFindTopDocuments(policy, raw_query, document_predicate);
	}

	std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy& policy, std::string_view raw_query, DocumentStatus status) const;

	std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy& policy, std::string_view raw_query) const;


	int GetDocumentCount() const;

	std::set<int>::iterator begin() const;

	std::set<int>::iterator end() const;

	const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;

	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy& policy, std::string_view raw_query, int document_id) const;

	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy& policy, std::string_view raw_query, int document_id) const;

	void RemoveDocument(int document_id);

	void RemoveDocument(const std::execution::sequenced_policy& policy, int document_id);

	void RemoveDocument(const std::execution::parallel_policy& policy, int document_id);

private:
	struct DocumentData {
		int rating;
		DocumentStatus status;
	};
	const std::set<std::string> stop_words_;
	std::map<std::string, std::map<int, double>> word_to_document_freqs_;
	std::map<int, DocumentData> documents_;
	std::set<int> document_ids_;
	std::map<int, std::map<std::string_view, double>> document_to_word_freq_;


	bool IsStopWord(std::string_view word)const;

	static bool IsValidWord(std::string_view word);

	std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

	static int ComputeAverageRating(const std::vector<int>& ratings);

	struct QueryWord {
		std::string_view data;
		bool is_minus;
		bool is_stop;
	};

	QueryWord ParseQueryWord(std::string_view text) const;

	struct Query {
		std::set<std::string_view> plus_words;
		std::set<std::string_view> minus_words;
	};

	Query ParseQuery(std::string_view text) const;


	double ComputeWordInverseDocumentFreq(const std::string& word) const;

	template <typename DocumentPredicate>
	std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {

		ConcurrentMap<int, double> document_to_relevance_conc(3);

		std::map<int, double> document_to_relevance;
		for (const std::string_view& word : query.plus_words) {
			const std::string word_(word);
			if (word_to_document_freqs_.count(word_) == 0) {
				continue;
			}
			const double inverse_document_freq = ComputeWordInverseDocumentFreq(word_);
			for (const auto[document_id, term_freq] : word_to_document_freqs_.at(word_)) {
				const auto& document_data = documents_.at(document_id);
				if (document_predicate(document_id, document_data.status, document_data.rating)) {
					document_to_relevance[document_id] += term_freq * inverse_document_freq;
				}
			}
		}

		for (const std::string_view& word : query.minus_words) {
			const std::string word_(word);
			if (word_to_document_freqs_.count(word_) == 0) {
				continue;
			}
			for (const auto[document_id, _] : word_to_document_freqs_.at(word_)) {
				document_to_relevance.erase(document_id);
			}
		}

		std::vector<Document> matched_documents;
		for (const auto[document_id, relevance] : document_to_relevance) {
			matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
		}
		return matched_documents;
	}

	template <typename DocumentPredicate, typename ExecutionPolicy>
	std::vector<Document> FindAllDocuments(const ExecutionPolicy& policy, const Query& query, DocumentPredicate document_predicate) const {

		ConcurrentMap<int, double> document_to_relevance_conc(3);

		for (const std::string_view& word : query.plus_words) {
			const std::string word_(word);
			if (word_to_document_freqs_.count(word_) == 0) {
				continue;
			}
			const double inverse_document_freq = ComputeWordInverseDocumentFreq(word_);

			for_each(word_to_document_freqs_.at(word_).begin(), word_to_document_freqs_.at(word_).end(), [&](const auto& doc_to_freq) {
				const auto&[document_id, term_freq] = doc_to_freq;
				const auto& document_data = documents_.at(document_id);
				if (document_predicate(document_id, document_data.status, document_data.rating)) {
					document_to_relevance_conc[document_id].ref_to_value += term_freq * inverse_document_freq;
				}
			});

		}

		auto restore_document_to_relevance = document_to_relevance_conc.BuildOrdinaryMap();

		for (const std::string_view& word : query.minus_words) {
			const std::string word_(word);
			if (word_to_document_freqs_.count(word_) == 0) {
				continue;
			}
			for (const auto[document_id, _] : word_to_document_freqs_.at(word_)) {
				restore_document_to_relevance.erase(document_id);
			}
		}

		std::vector<Document> matched_documents;
		for (const auto[document_id, relevance] : restore_document_to_relevance) {
			matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
		}
		return matched_documents;
	}


	template<typename ExecutionPolicy>
	void GeneralizedRemoveDocument(const ExecutionPolicy& policy, int document_id) {
		const auto& words = document_to_word_freq_.at(document_id);

		std::vector<std::string_view> keys;
		keys.reserve(words.size());

		for (const auto&[word, _] : words) {
			keys.push_back(word);
		}

		std::for_each(policy, keys.begin(), keys.end(), [&, document_id](const auto& key) {
			word_to_document_freqs_[std::string(key)].erase(document_id);
		});

		document_to_word_freq_.erase(document_id);
		documents_.erase(document_id);
		document_ids_.erase(document_id);
	}

	template<typename ExecutionPolicy>
	std::tuple<std::vector<std::string_view>, DocumentStatus> GeneralizedMatchDocument(const ExecutionPolicy& policy, std::string_view raw_query, int document_id) const {

		if (document_to_word_freq_.count(document_id) == 0) {
			throw std::out_of_range("passed a nonexistent document_id");
		}

		const auto query = ParseQuery(raw_query);

		std::vector<std::string_view> matched_words;

		std::for_each(policy, query.plus_words.begin(), query.plus_words.end(), [&, document_id](const auto& word) {

			const std::string word_(word);
			const auto& find_item = word_to_document_freqs_.find(word_);

			if (find_item != word_to_document_freqs_.end() && word_to_document_freqs_.at(word_).count(document_id)) {
				matched_words.push_back(find_item->first);
			}
		});

		std::for_each(policy, query.minus_words.begin(), query.minus_words.end(), [&, document_id](const auto& word) {

			const std::string word_(word);
			const auto& find_item = word_to_document_freqs_.find(word_);

			if (find_item != word_to_document_freqs_.end() && word_to_document_freqs_.at(word_).count(document_id)) {
				matched_words.clear();
				return;
			}
		});

		return { matched_words, documents_.at(document_id).status };
	}


	template <typename DocumentPredicate, typename ExecutionPolicy>
	std::vector<Document> GeneralizedFindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, DocumentPredicate document_predicate) const {

		const auto query = ParseQuery(raw_query);
		auto matched_documents = FindAllDocuments(policy, query, document_predicate);

		std::sort(policy, matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
			if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
				return lhs.rating > rhs.rating;
			}
			else {
				return lhs.relevance > rhs.relevance;
			}
		});
		if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
			matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
		}
		return matched_documents;
	}
};
