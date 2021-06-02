#include "process_queries.h"
#include "search_server.h"

#include <execution>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

void PrintDocument(const Document& document) {
	cout << "{ "s
		<< "document_id = "s << document.id << ", "s
		<< "relevance = "s << document.relevance << ", "s
		<< "rating = "s << document.rating << " }"s << endl;
}

int main() {
	
	
	SearchServer search_server("and with"s);

	int id = 0;
	for (
		const string& text : {
			"funny pet and nasty rat"s,
			"funny pet with curly hair"s,
			"funny pet and not very nasty rat"s,
			"pet with rat and rat and rat"s,
			"nasty rat with curly hair"s,
		}
		) {
		search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
	}

	const vector<string> queries = {
		"nasty rat -not"s,
		"not very funny nasty pet"s,
		"curly hair"s
	};
	for (const Document& document : ProcessQueriesJoined(search_server, queries)) {
		cout << "Document "s << document.id << " matched with relevance "s << document.relevance << endl;
	}
	
	
	
	//SearchServer search_server("and with"s);

	//int id = 0;
	//for (
	//	const string& text : {
	//		"white cat and yellow hat"s,
	//		"curly cat curly tail"s,
	//		"nasty dog with big eyes"s,
	//		"nasty pigeon john"s,
	//	}
	//	) {
	//	search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
	//}


	//cout << "ACTUAL by default:"s << endl;
	//// последовательная версия
	//for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
	//	PrintDocument(document);
	//}
	//cout << "BANNED:"s << endl;
	//// последовательная версия
	//for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
	//	PrintDocument(document);
	//}

	//cout << "Even ids:"s << endl;
	//// параллельная версия
	//for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
	//	PrintDocument(document);
	//}

	system("pause");
	return 0;
}