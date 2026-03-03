#include "ingestion/ingestion_pipeline.hpp"
#include "ingestion/file_scanner.hpp"
#include "ingestion/txt_processor.hpp"
#include "ingestion/pdf_processor.hpp"
#include "ingestion/deterministic_aggregator.hpp"
#include "ingestion/document_builder.hpp"
#include <stdexcept>

namespace haystack
{
    namespace phase2_5
    {

        namespace
        {

            void validate_root(const std::filesystem::path &root)
            {
                if (root.empty())
                    throw std::invalid_argument("ingest_directory: root path must not be empty");

                if (!std::filesystem::exists(root))
                    throw std::invalid_argument("ingest_directory: root path does not exist: " + root.string());

                if (!std::filesystem::is_directory(root))
                    throw std::invalid_argument("ingest_directory: root path is not a directory: " + root.string());
            }

            class VectorEmitter : public IDocumentEmitter
            {
            public:
                explicit VectorEmitter(std::vector<IngestedDocument> &out) : out_(out) {}

                void emit(IngestedDocument &&doc) override
                {
                    out_.push_back(std::move(doc));
                }

            private:
                std::vector<IngestedDocument> &out_;
            };

        } // anonymous namespace

        std::vector<IngestedDocument> ingest_directory(const std::filesystem::path &root)
        {
            validate_root(root);

            const auto paths = scan_inputs(root);

            std::vector<IngestedDocument> docs;
            VectorEmitter emitter(docs);

            for (const auto &path : paths)
            {
                const auto ext = path.extension().string();
                if (ext == ".txt")
                {
                    process_txt(path, emitter);
                }
                else if (ext == ".pdf")
                {
                    process_pdf(path, emitter);
                }
            }

            return finalize_documents(std::move(docs));
        }

    } // namespace phase2_5
} // namespace haystack
