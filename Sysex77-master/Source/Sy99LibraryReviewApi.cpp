#include "Sy99LibraryReviewApi.h"

extern juce::File appDirPath;

namespace Sy99LibraryReviewApi
{
    namespace
    {
        juce::File reviewJsonFile (const juce::String& reviewId)
        {
            return libraryReviewsDirPath().getChildFile (reviewId + ".json");
        }

        juce::File reviewMarkdownFile (const juce::String& reviewId)
        {
            const juce::File mirror = libraryReviewsMirrorDirPath();

            if (mirror.isDirectory())
                return mirror.getChildFile (reviewId + ".md");

            return {};
        }

        juce::String reviewPageUrl (const juce::var& review)
        {
            const juce::String pageId = review.getProperty ("pageId", {}).toString();
            const int mm = (int) review.getProperty ("mm", 0);
            const juce::String id = review.getProperty ("id", {}).toString();
            return "http://localhost:5173/library/sy99/" + pageId + "/" + juce::String (mm)
                   + "?review=" + id;
        }

        juce::String formatReviewMarkdown (const juce::var& review)
        {
            juce::String md;
            md << "# Library review: " << review.getProperty ("voiceName", {}).toString() << "\n\n";
            md << "- **id:** `" << review.getProperty ("id", {}).toString() << "`\n";
            md << "- **page:** " << review.getProperty ("pageId", {}).toString()
               << " slot " << review.getProperty ("slotCode", {}).toString()
               << " (mm=" << (int) review.getProperty ("mm", 0) << ")\n";
            md << "- **capture:** " << review.getProperty ("captureFile", {}).toString() << "\n";
            md << "- **url:** " << reviewPageUrl (review) << "\n";
            md << "- **api:** `GET http://127.0.0.1:8765/api/library/reviews/"
               << review.getProperty ("id", {}).toString() << "`\n\n";

            md << "## Manual (LCD confirmed)\n\n";

            if (auto* manual = review.getProperty ("manualItems", {}).getArray())
            {
                if (manual->isEmpty())
                    md << "_none_\n\n";
                else
                    for (const auto& item : *manual)
                    {
                        md << "- **" << item.getProperty ("paramId", {}).toString()
                           << "** El." << (int) item.getProperty ("elementIndex", 0)
                           << " `" << item.getProperty ("field", {}).toString()
                           << "`: app `" << item.getProperty ("appValue", {}).toString()
                           << "` → synth `" << item.getProperty ("synthValue", {}).toString() << "`\n";
                    }
            }

            md << "\n## Auto (unconfirmed)\n\n";

            if (auto* autoItems = review.getProperty ("autoChecksSnapshot", {}).getArray())
            {
                if (autoItems->isEmpty())
                    md << "_none_\n\n";
                else
                    for (const auto& item : *autoItems)
                    {
                        md << "- **" << item.getProperty ("paramId", {}).toString()
                           << "** El." << (int) item.getProperty ("elementIndex", 0)
                           << " `" << item.getProperty ("autoStatus", {}).toString() << "`";

                        if (item.hasProperty ("expectedUi"))
                            md << " expected UI `" << item.getProperty ("expectedUi", {}).toString() << "`";

                        md << " app UI `" << item.getProperty ("appUi", {}).toString() << "`\n";
                    }
            }

            const juce::String fixtureId = review.getProperty ("fixtureId", {}).toString();

            if (fixtureId.isNotEmpty())
            {
                md << "\n## Regression\n\n";
                md << "```bash\npython _agent_context/fixtures/_validate_bulk_parse.py\n```\n";
                md << "Fixture ref: `" << fixtureId << "`\n";
            }

            md << "\n---\n\n";
            md << "After fix: `DELETE http://127.0.0.1:8765/api/library/reviews/"
               << review.getProperty ("id", {}).toString() << "`\n";

            return md;
        }

        void writeMarkdownMirror (const juce::var& review)
        {
            const juce::File mdFile = reviewMarkdownFile (review.getProperty ("id", {}).toString());

            if (mdFile == juce::File())
                return;

            mdFile.getParentDirectory().createDirectory();
            mdFile.replaceWithText (formatReviewMarkdown (review));
        }
    }

    juce::File libraryReviewsDirPath() noexcept
    {
        return appDirPath.getChildFile ("reviews");
    }

    juce::File libraryReviewsMirrorDirPath() noexcept
    {
        const juce::File config = appDirPath.getChildFile ("agent_reviews_mirror.txt");

        if (! config.existsAsFile())
            return {};

        const juce::String path = config.loadFileAsString().trim();

        if (path.isEmpty())
            return {};

        return juce::File (path);
    }

    juce::var createReviewFromJsonBody (const juce::var& body, juce::String& errorOut)
    {
        if (! body.isObject())
        {
            errorOut = "review body must be a JSON object";
            return {};
        }

        const juce::File dir = libraryReviewsDirPath();

        if (! dir.exists())
            dir.createDirectory();

        const juce::String id = juce::Uuid().toString();
        auto* review = new juce::DynamicObject();
        review->setProperty ("id", id);
        review->setProperty ("createdAt", juce::Time::getCurrentTime().toISO8601 (true));
        review->setProperty ("pageId", body.getProperty ("pageId", {}));
        review->setProperty ("pageLabel", body.getProperty ("pageLabel", {}));
        review->setProperty ("mm", body.getProperty ("mm", {}));
        review->setProperty ("slotCode", body.getProperty ("slotCode", {}));
        review->setProperty ("voiceName", body.getProperty ("voiceName", {}));
        review->setProperty ("captureFile", body.getProperty ("captureFile", {}));
        review->setProperty ("fixtureId", body.getProperty ("fixtureId", {}));
        review->setProperty ("manualItems", body.getProperty ("manualItems", juce::var (juce::Array<juce::var>())));
        review->setProperty ("confirmedOk", body.getProperty ("confirmedOk", juce::var (juce::Array<juce::var>())));
        review->setProperty ("autoChecksSnapshot",
                             body.getProperty ("autoChecksSnapshot", juce::var (juce::Array<juce::var>())));

        const juce::var reviewVar (review);
        const juce::File jsonFile = reviewJsonFile (id);

        if (! jsonFile.replaceWithText (juce::JSON::toString (reviewVar, true)))
        {
            errorOut = "failed to write review file";
            return {};
        }

        writeMarkdownMirror (reviewVar);

        auto* result = new juce::DynamicObject();
        result->setProperty ("id", id);
        result->setProperty ("url", reviewPageUrl (reviewVar));
        result->setProperty ("apiUrl", "http://127.0.0.1:8765/api/library/reviews/" + id);
        return result;
    }

    juce::var reviewToJsonVar (const juce::String& reviewId)
    {
        const juce::File file = reviewJsonFile (reviewId);

        if (! file.existsAsFile())
            return {};

        return juce::JSON::parse (file.loadFileAsString());
    }

    juce::var listReviewsToJsonVar()
    {
        juce::Array<juce::var> items;
        const juce::File dir = libraryReviewsDirPath();

        if (! dir.isDirectory())
            return items;

        for (const auto& entry : dir.findChildFiles (juce::File::findFiles, false, "*.json"))
        {
            const juce::var parsed = juce::JSON::parse (entry.loadFileAsString());

            if (parsed.isVoid())
                continue;

            auto* summary = new juce::DynamicObject();
            summary->setProperty ("id", parsed.getProperty ("id", entry.getFileNameWithoutExtension()));
            summary->setProperty ("voiceName", parsed.getProperty ("voiceName", {}));
            summary->setProperty ("pageId", parsed.getProperty ("pageId", {}));
            summary->setProperty ("mm", parsed.getProperty ("mm", {}));
            summary->setProperty ("createdAt", parsed.getProperty ("createdAt", {}));
            summary->setProperty ("url", reviewPageUrl (parsed));
            items.add (summary);
        }

        return items;
    }

    bool deleteReview (const juce::String& reviewId, juce::String& errorOut)
    {
        const juce::File jsonFile = reviewJsonFile (reviewId);

        if (! jsonFile.existsAsFile())
        {
            errorOut = "review not found";
            return false;
        }

        jsonFile.deleteFile();

        const juce::File mdFile = reviewMarkdownFile (reviewId);

        if (mdFile != juce::File() && mdFile.existsAsFile())
            mdFile.deleteFile();

        return true;
    }

    juce::String extractReviewIdFromPath (const juce::String& path) noexcept
    {
        const juce::String prefix = "/api/library/reviews/";

        if (! path.startsWithIgnoreCase (prefix))
            return {};

        juce::String id = path.substring (prefix.length());

        if (id.contains ("/"))
            id = id.upToFirstOccurrenceOf ("/", false, false);

        return id.trim();
    }
} // namespace Sy99LibraryReviewApi
