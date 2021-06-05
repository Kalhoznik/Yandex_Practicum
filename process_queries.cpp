#include "process_queries.h"
#include <algorithm>
#include <execution>

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries)
{
	using SearchingDocuments = std::vector<Document>;
	if (queries.empty()) {
		return{};
	}
	std::vector<SearchingDocuments> results(queries.size());
	std::transform(std::execution::par, queries.begin(), queries.end(), results.begin(), [&search_server](const std::string& query) {
		return search_server.FindTopDocuments(query);
	});

	return results;
}

std::list<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries)
{
	if (queries.empty()) {
		return {};
	}

	auto searching_documents = ProcessQueries(search_server, queries);

	std::list<Document> result;
	result = std::transform_reduce(std::execution::par,
		searching_documents.begin(),
		searching_documents.end(),
		std::list<Document>{},
		[](std::list<Document> a, std::list<Document> b) {
		a.splice(a.end(), b);
		return a; },
		[&search_server](auto& request) {
			const std::list<Document> result(request.begin(), request.end());

			return result;
		});

	return result;
}