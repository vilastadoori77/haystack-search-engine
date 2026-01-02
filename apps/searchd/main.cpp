#include <drogon/drogon.h>
#include "core/search_service.h"
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <string>

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

static bool has_flag(int argc, char **argv, const std::string &flag)
{
    for (int i = 1; i < argc; ++i)
        if (argv[i] == flag)
            return true;
    return false;
}

static std::string get_arg_value(int argc, char **argv, const std::string &key)
{
    for (int i = 1; i + 1 < argc; ++i)
        if (argv[i] == key)
            return argv[i + 1];
    return "";
}

static std::string getenv_str(const char *name)
{
    const char *v = std::getenv(name);
    return v ? std::string(v) : "";
}

static int getenv_int(const char *name, int def)
{
    auto s = getenv_str(name);
    if (s.empty())
        return def;
    try
    {
        return std::stoi(s);
    }
    catch (...)
    {
        return def;
    }
}

static int get_int_value(int argc, char **argv, const std::string &key, int def)
{
    auto v = get_arg_value(argc, argv, key);
    if (v.empty())
        return def;
    try
    {
        return std::stoi(v);
    }
    catch (...)
    {
        return def;
    }
}

int main(int argc, char **argv)
{

    const bool index_mode = has_flag(argc, argv, "--index");
    const bool serve_mode = has_flag(argc, argv, "--serve") || !index_mode; // default = serve

    // Docs path: CLI > ENV > default
    std::string docs = get_arg_value(argc, argv, "--docs");
    if (docs.empty())
        docs = getenv_str("DOCS_PATH");
    if (docs.empty())
        docs = "data/docs.json";

    // Port: CLI > ENV > fallback
#ifndef SEARCHD_PORT
#define SEARCHD_PORT 8900
#endif
    int port = get_int_value(argc, argv, "--port", getenv_int("SEARCHD_PORT", SEARCHD_PORT));

    // Create Search Engine Instance
    SearchService s;

    // Sample documents (temporary -later loaded from disk/db)
    // s.add_document(1, "Teamcenter migration guide: map attributes, validate schema, run dry-run.");
    // s.add_document(2, "PLM data migration: cleansing, mapping, validation.");
    // s.add_document(3, "Schema validation checklist for integration projects.");
    // s.add_document(1, "The quick brown fox jumps over the lazy dog.");
    // load_docs_from_json(s, "data/docs.json");
    if (docs.empty())
        docs = getenv_str("DOCS_PATH");
    if (docs.empty())
        docs = "data/docs.json";
    load_docs_from_json(s, docs);

    if (has_flag(argc, argv, "--index") && !has_flag(argc, argv, "--serve"))
    {
        std::cout << "Indexing built from: " << docs << "\n";
        return 0;
    }
    // Index-only mode: build index and exit
    if (index_mode && !has_flag(argc, argv, "--serve"))
    {
        std::cout << "Indexing completed. Exiting as per --index flag." << std::endl;
        return 0;
    }

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
            int k = 10;
            auto kstr = req->getParameter("k");
            if (!kstr.empty())
            {
                try
                {
                    k = std::max(1, std::stoi(kstr));
                }
                catch (...)
                {
                    // ignore invalid k param
                }
            }

            auto hits = s.search_with_snippets(q);
            if (hits.size() > static_cast<size_t>(k))
            {
                hits.resize(k);
            }

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

    drogon::app().addListener("0.0.0.0", port);
    std::cout << "Serving on port " << port << "using docs: " << docs << "\n";
    drogon::app().run();
}