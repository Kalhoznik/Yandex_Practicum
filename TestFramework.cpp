#include<iostream>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cassert>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;


template<typename Key, typename Value >
ostream& operator<<(ostream& out, const pair<Key, Value>& cont) {

	out << cont.first << ": " << cont.second;

	return out;
}

template<typename Documents>
ostream& Print(ostream& out, const Documents& documents) {
	bool isFirst = true;
	for (const auto& el : documents) {
		if (isFirst) {
			out << el;
			isFirst = false;
		}
		else { out << ", " << el; }

	}
	return out;
}

template<typename Key, typename Value >
ostream& operator<<(ostream& out, const map<Key, Value>& cont) {
	out << "{";
	return  Print(out, cont) << "}";
}

template<typename Element>
ostream& operator<<(ostream& out, const vector<Element>& cont) {
	out << "[";
	return  Print(out, cont) << "]";
}

template<typename Element>
ostream& operator<<(ostream& out, const set<Element>& cont) {
	out << "{";
	return  Print(out, cont) << "}";

}

enum class DocumentStatus {
	ACTUAL,
	IRRELEVANT,
	BANNED,
	REMOVED,
};

struct Document {
	int id;
	double relevance;
	int rating;
};

string ReadLine() {
	string s;
	getline(cin, s);
	return s;
}

int ReadLineWithNumber() {
	int result;
	cin >> result;
	ReadLine();
	return result;
}

vector<string> SplitIntoWords(const string& text) {
	vector<string> words;
	string word;
	for (const char c : text) {
		if (c == ' ') {
			words.push_back(word);
			word = "";
		}
		else {
			word += c;
		}
	}
	words.push_back(word);

	return words;
}

class SearchServer {
public:
	void SetStopWords(const string& text) {
		for (const string& word : SplitIntoWords(text)) {
			stop_words_.insert(word);
		}
	}

	void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
		const vector<string> words = SplitIntoWordsNoStop(document);
		const double inv_word_count = 1.0 / words.size();
		for (const string& word : words) {
			word_to_document_freqs_[word][document_id] += inv_word_count;
		}
		documents_.emplace(document_id,
			DocumentData{
				ComputeAverageRating(ratings),
				status
			});
	}

	template<typename Predicate>
	vector<Document> FindTopDocuments(const string& raw_query, Predicate predicate) const {
		const Query query = ParseQuery(raw_query);
		auto matched_documents = FindAllDocuments(query, predicate);

		sort(matched_documents.begin(), matched_documents.end(),
			[](const Document& lhs, const Document& rhs) {
			if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
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
	vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status_) const {
		return FindTopDocuments(raw_query, [&status_](int document_id, DocumentStatus status, int rating) { return status == status_; });
	}
	vector<Document> FindTopDocuments(const string& raw_query) const {
		return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
	}

	int GetDocumentCount() const {
		return documents_.size();
	}

	tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
		const Query query = ParseQuery(raw_query);
		vector<string> matched_words;
		for (const string& word : query.plus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			if (word_to_document_freqs_.at(word).count(document_id)) {
				matched_words.push_back(word);
			}
		}
		for (const string& word : query.minus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			if (word_to_document_freqs_.at(word).count(document_id)) {
				matched_words.clear();
				break;
			}
		}
		return { matched_words, documents_.at(document_id).status };
	}

private:

	struct DocumentData {
		int rating;
		DocumentStatus status;
	};
	
	set<string> stop_words_;
	map<string, map<int, double>> word_to_document_freqs_;
	map<int, DocumentData> documents_;

	bool IsStopWord(const string& word) const {
		return stop_words_.count(word) > 0;
	}

	vector<string> SplitIntoWordsNoStop(const string& text) const {
		vector<string> words;
		for (const string& word : SplitIntoWords(text)) {
			if (!IsStopWord(word)) {
				words.push_back(word);
			}
		}
		return words;
	}

	static int ComputeAverageRating(const vector<int>& ratings) {
		int rating_sum = 0;
		for (const int rating : ratings) {
			rating_sum += rating;
		}
		return rating_sum / static_cast<int>(ratings.size());
	}

	struct QueryWord {
		string data;
		bool is_minus;
		bool is_stop;
	};

	QueryWord ParseQueryWord(string text) const {
		bool is_minus = false;
		// Word shouldn't be empty
		if (text[0] == '-') {
			is_minus = true;
			text = text.substr(1);
		}
		return {
			text,
			is_minus,
			IsStopWord(text)
		};
	}

	struct Query {
		set<string> plus_words;
		set<string> minus_words;
	};

	Query ParseQuery(const string& text) const {
		Query query;
		for (const string& word : SplitIntoWords(text)) {
			const QueryWord query_word = ParseQueryWord(word);
			if (!query_word.is_stop) {
				if (query_word.is_minus) {
					query.minus_words.insert(query_word.data);
				}
				else {
					query.plus_words.insert(query_word.data);
				}
			}
		}
		return query;
	}

	double ComputeWordInverseDocumentFreq(const string& word) const {
		return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
	}

	template<typename Predicate>
	vector<Document> FindAllDocuments(const Query& query, Predicate predicate) const {
		map<int, double> document_to_relevance;
		for (const string& word : query.plus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
			for (const auto[document_id, term_freq] : word_to_document_freqs_.at(word)) {
				const DocumentData& doc_data = documents_.at(document_id);
				if (predicate(document_id, doc_data.status, doc_data.rating)) {
					document_to_relevance[document_id] += term_freq * inverse_document_freq;
				}
			}
		}

		for (const string& word : query.minus_words) {
			if (word_to_document_freqs_.count(word) == 0) {
				continue;
			}
			for (const auto[document_id, _] : word_to_document_freqs_.at(word)) {
				document_to_relevance.erase(document_id);
			}
		}

		vector<Document> matched_documents;
		for (const auto[document_id, relevance] : document_to_relevance) {
			matched_documents.push_back({
				document_id,
				relevance,
				documents_.at(document_id).rating
				});
		}
		return matched_documents;
	}
};


template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
	const string& func, unsigned line, const string& hint) {
	if (t != u) {
		cerr << boolalpha;
		cerr << file << "("s << line << "): "s << func << ": "s;
		cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
		cerr << t << " != "s << u << "."s;
		if (!hint.empty()) {
			cerr << " Hint: "s << hint;
		}
		cerr << endl;
		abort();
	}
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a),(b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
	const string& hint) {
	if (!value) {
		cerr << file << "("s << line << "): "s << func << ": "s;
		cerr << "ASSERT("s << expr_str << ") failed."s;
		if (!hint.empty()) {
			cerr << " Hint: "s << hint;
		}
		cerr << endl;
		abort();
	}
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename TestFunction>
void RunTestImpl(TestFunction func, const string& func_name) {
	func();
	cerr << func_name << " OK" << endl;
}

#define RUN_TEST(func) RunTestImpl((func),#func)

// -------- Начало модульных тестов поисковой системы ----------

void TestExcludeStopWordsFromAddedDocumentContent() {
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = { 1, 2, 3 };
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("in"s);
		ASSERT_EQUAL(found_docs.size(), 1u);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, doc_id);
	}

	{
		SearchServer server;
		server.SetStopWords("in the"s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
	}
}

void TestAddingDocument() {
	SearchServer search_server;
	search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
	search_server.AddDocument(1, "пушистый пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
	search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
	search_server.AddDocument(4, "собака на сене"s, DocumentStatus::ACTUAL, { 9 });
	search_server.AddDocument(5, "кот в сапогах"s, DocumentStatus::ACTUAL, { 1,1,1 });
	{
		const auto& doc = search_server.FindTopDocuments("ухоженный хвост"s);
		ASSERT_EQUAL(doc.size(), 2);
		ASSERT_EQUAL(doc[0].id, 1);
		ASSERT_EQUAL(doc[1].id, 2);
			}
	{
		const auto& doc = search_server.FindTopDocuments("чайка летит"s);
		ASSERT_HINT(doc.empty(), "Size must equal to zero");
	}
}

void TestExludeDocmentsWhoContainsMinusWords() {
	SearchServer search_server;
	const string document_conetent = "ухоженный пес и хвост";

	{
		SearchServer search_server;
		search_server.AddDocument(0, document_conetent, DocumentStatus::ACTUAL, { 3,2,1,6 });
		const string query = "пес и хвост";
		const auto& docs = search_server.FindTopDocuments(query);
		ASSERT_EQUAL(docs.size(), 1u);
		ASSERT_EQUAL(docs[0].id, 0);

	}
	{
		SearchServer search_server;
		search_server.AddDocument(0, document_conetent, DocumentStatus::ACTUAL, { 3,2,1,6 });
		const string query = "пес и -хвост";
		const auto& docs = search_server.FindTopDocuments(query);
		ASSERT_HINT(docs.empty(), "Size must be empty");
	}

}
void TestMatch() {

	const string document_content = "слепая собака и глухой кот"s;
	const string stop_words = "и"s;
	
	{
		SearchServer server;
		server.AddDocument(4, document_content, DocumentStatus::ACTUAL, { 5,2,1 });
		const string query = "собака слепая"s;
		const auto&[words, status] = server.MatchDocument(query, 4);
		const vector<string> correct_words = { "слепая"s, "собака"s };
		ASSERT_EQUAL_HINT(words, correct_words, "Sets of words must be equal"s);

	}

	{
		SearchServer server;
		server.SetStopWords(stop_words);
		server.AddDocument(4, document_content, DocumentStatus::ACTUAL, { 5,2,1 });
		const string query = "собака и слепая"s;
		const auto&[words, status] = server.MatchDocument(query, 4);
		const vector<string> correct_words = { "слепая"s, "собака"s };
		ASSERT_EQUAL_HINT(words, correct_words, "Sets of words must be equal and stop words must be excluded"s);

	}

	{
		SearchServer server;
		server.AddDocument(4, document_content, DocumentStatus::ACTUAL, { 5,2,1 });
		const string query = "чайка"s;
		const auto&[words, status] = server.MatchDocument(query, 4);
		ASSERT_HINT(words.empty(), "The set of words must be empty"s);
	}

	{
		SearchServer server;
		server.AddDocument(4, document_content, DocumentStatus::ACTUAL, { 5,2,1 });
		const string query = "глухой -кот"s;
		const auto&[words, status] = server.MatchDocument(query, 4);
		ASSERT_HINT(words.empty(), "The set of words must be empty"s);
	}

}

void TestRelevantseSort() {
	const string first_doc_content = "белый кот и модный ошейник"s;
	const string second_doc_content = "ухоженный пёс выразительные глаза"s;
	const string third_doc_content = "пушистый пушистый хвост"s;

	SearchServer server;
	server.AddDocument(1, first_doc_content, DocumentStatus::ACTUAL, { 5,2,1 });
	server.AddDocument(2, second_doc_content, DocumentStatus::ACTUAL, { 3,2,5 });
	server.AddDocument(3, third_doc_content, DocumentStatus::ACTUAL, { 2,7,10 });
	const string query = "кот пёс хвост";
	const auto& docs = server.FindTopDocuments(query);
	size_t prev = 0;
	for (size_t curr = 1; curr < docs.size(); ++curr) {
		ASSERT_HINT(docs[prev].relevance > docs[curr].relevance, "Previous document must be great then current");
		prev = curr;
	}
}

void TestComputeRating() {
	string first_doc_content = "белый кот и модный ошейник"s;
	string second_doc_content = "ухоженный пёс выразительные глаза"s;
	SearchServer server;
	server.AddDocument(1, first_doc_content, DocumentStatus::ACTUAL, { 5,2,1 });
	server.AddDocument(2, second_doc_content, DocumentStatus::ACTUAL, { 3,2,5 });
	string query = "кот пёс хвост";
	const auto& docs = server.FindTopDocuments(query);
	ASSERT_EQUAL(docs[0].rating, 3);
	ASSERT_EQUAL_HINT(docs[1].rating, 2, "Rating must be equal average");
}

void TestFilteringWhithPredicate() {
	const string first_doc_content = "белый кот и модный ошейник"s;
	const string second_doc_content = "ухоженный пёс выразительные глаза"s;
	const string third_doc_content = "пушистый пушистый хвост"s;
	
	SearchServer server;
	server.AddDocument(1, first_doc_content, DocumentStatus::ACTUAL, { 5,2,1 });
	server.AddDocument(2, second_doc_content, DocumentStatus::ACTUAL, { 3,2,5 });
	server.AddDocument(3, third_doc_content, DocumentStatus::ACTUAL, { 2,5,6 });
	const string query = "кот пёс хвост";
	const auto& docs = server.FindTopDocuments(query, [](int document_id, DocumentStatus status, int rating) { return document_id >= 2; });

	ASSERT(docs.size() == 2);
	ASSERT_EQUAL(docs[0].id, 3);
	ASSERT_EQUAL(docs[1].id, 2);

}

void TestFilteringDocumentWhithDocumentStatus() {
	const string first_doc_content = "белый кот и модный ошейник"s;
	const string second_doc_content = "ухоженный пёс выразительные глаза"s;
	

	{
		SearchServer server;
		server.AddDocument(1, first_doc_content, DocumentStatus::ACTUAL, { 5,2,1 });
		server.AddDocument(2, second_doc_content, DocumentStatus::BANNED, { 3,2,5 });
		const string query = "кот пёс хвост";
		const auto& docs = server.FindTopDocuments(query, DocumentStatus::ACTUAL);
		ASSERT_EQUAL(docs[0].id, 1);
	}

	{
		SearchServer server;
		server.AddDocument(1, first_doc_content, DocumentStatus::ACTUAL, { 5,2,1 });
		server.AddDocument(2, second_doc_content, DocumentStatus::BANNED, { 3,2,5 });
		const string query = "кот пёс хвост";
		const auto& docs = server.FindTopDocuments(query, DocumentStatus::BANNED);
		ASSERT_EQUAL(docs[0].id, 2);
	}
}


void TestComputeRelevance() {
	const string first_doc_content = "белый кот и модный ошейник"s;
	const string second_doc_content = "ухоженный пёс выразительные глаза"s;
	const string third_doc_content = "пушистый пушистый хвост"s;

	SearchServer server;
	server.AddDocument(1, first_doc_content, DocumentStatus::ACTUAL, { 5,2,1 });
	server.AddDocument(2, second_doc_content, DocumentStatus::ACTUAL, { 3,2,5 });
	server.AddDocument(3, third_doc_content, DocumentStatus::ACTUAL, { 2,5,6 });
	const string query = "кот пёс хвост";
	const auto& docs = server.FindTopDocuments(query, DocumentStatus::ACTUAL);
	const double EPSILON = 1e-6;
	ASSERT(abs(docs[0].relevance - 0.366204) < EPSILON);
	ASSERT(abs(docs[1].relevance - 0.274653) < EPSILON);
	ASSERT_HINT(abs(docs[2].relevance - 0.219722) < EPSILON, "Difference between document relevance and current relevance must be equal to zero");
}

void TestSearchServer() {
	RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
	RUN_TEST(TestAddingDocument);
	RUN_TEST(TestExludeDocmentsWhoContainsMinusWords);
	RUN_TEST(TestMatch);
	RUN_TEST(TestRelevantseSort);
	RUN_TEST(TestComputeRating);
	RUN_TEST(TestFilteringWhithPredicate);
	RUN_TEST(TestFilteringDocumentWhithDocumentStatus);
	RUN_TEST(TestComputeRelevance);
	
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
	TestSearchServer();
	
	cout << "Search server testing finished"s << endl;
}