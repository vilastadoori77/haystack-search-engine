#include <drogon/drogon.h>
#include "core/search_service.h"
#include <fstream>

static void load_docs_from_json(SearchService &s, const std::string &path)
{
    std::ifstream in(path);
    if (!in)
    {
        throw std::runtime_error("Failed to open docs file: " + path);
    }

    Json::Value root;
    in >> root;

    for (const auto &item : root)
    {
        int docId = item["docId"].asInt();
        std::string text = item["text"].asString();
        s.add_document(docId, text);
    }
}

int main()
{

    // Create Search Engine Instance

    SearchService s;

    // Sample documents (temporary -later loaded from disk/db)
    // s.add_document(1, "Teamcenter migration guide: map attributes, validate schema, run dry-run.");
    // s.add_document(2, "PLM data migration: cleansing, mapping, validation.");
    // s.add_document(3, "Schema validation checklist for integration projects.");
    // s.add_document(1, "The quick brown fox jumps over the lazy dog.");
    load_docs_from_json(s, "data/docs.json");

    // Health check
    drogon::app().registerHandler(
        "/health",
        [](const drogon::HttpRequestPtr &req,
           std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k200OK);
            resp->setContentTypeCode(drogon::CT_TEXT_PLAIN);
            resp->setBody("OK");
            callback(resp);
        });
    // Search endpoint
    drogon::app().registerHandler(
        "/search",
        [&s](const drogon::HttpRequestPtr &req,
             std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            auto q = req->getParameter("q");
            auto hits = s.search_with_snippets(q);
            Json::Value json(Json::objectValue);
            json["query"] = q;

            Json::Value arr(Json::arrayValue);
            for (const auto &hit : hits)
            {
                Json::Value h(Json::objectValue);
                h["docId"] = hit.docId;
                h["score"] = hit.score;
                h["snippet"] = hit.snippet;
                arr.append(h);
            }
            json["results"] = arr;
            callback(drogon::HttpResponse::newHttpJsonResponse(json));
        });

    // start Server

    // drogon::app().addListener("0.0.0.0", 8900);
    // drogon::app().run();

#ifndef SEARCHD_PORT
#define SEARCHD_PORT 8900
#endif

    drogon::app().addListener("0.0.0.0", SEARCHD_PORT);
    drogon::app().run();
}