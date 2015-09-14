#include <boost/algorithm/string/trim.hpp>

#include <scope/localization.h>
#include <scope/query.h>

#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/SearchReply.h>
#include <unity/scopes/Department.h>

#include <iomanip>
#include <sstream>

namespace sc = unity::scopes;
namespace alg = boost::algorithm;

using namespace std;
using namespace api;
using namespace scope;


/**
 * Define the layout for the stream results
 *
 * The icon size is small, and ask for the card layout
 * itself to be horizontal. I.e. the text will be placed
 * next to the image.
 */
const static string STREAM_TEMPLATE =
        R"(
{
        "schema-version": 1,
        "template": {
        "category-layout": "grid",
        "card-layout": "horizontal",
        },
        "components": {
        "title": "title",
        "mascot" : "mascot",
        "subtitle": "subtitle"
        }
        }
        )";

Query::Query(const sc::CannedQuery &query, const sc::SearchMetadata &metadata,
             Config::Ptr config) :
    sc::SearchQueryBase(query, metadata), client_(config) {
}

void Query::cancelled() {
    client_.cancel();
}


void Query::run(sc::SearchReplyProxy const &reply) {
    try {
        sc::Department::SPtr all_depts = sc::Department::create("", query(), "All departments");
        // Create new base department
        for (const string &d : client_.query_deps()){
            all_depts->add_subdepartment(sc::Department::create(d, query(), d));
        }

        // Register departments on the reply
        reply->register_departments(all_depts);

        // Start by getting information about the query
        const sc::CannedQuery &query(sc::SearchQueryBase::query());

        // Trim the query string of whitespace
        string query_string = alg::trim_copy(query.query_string());

        Client::Stream streams;

        if (!query.department_id().empty()) {
            // If there is a department selected, use its id for the query string
            streams = client_.query_streams(query.department_id());
        } else if (query_string.empty()) {
            // If there is no search string, get streams related to programming/development
            streams = client_.query_streams("development");
        } else {
            // otherwise, get the forecast for the search string
            streams = client_.query_streams(query_string);
        }

        // Register a category for the forecast
        auto forecast_cat = reply->register_category("forecast",
                                                     _(""), "", sc::CategoryRenderer(STREAM_TEMPLATE));

        // For each of the forecast days
        for (const auto &stream : streams.stream) {
            // Create a result
            sc::CategorisedResult res(forecast_cat);

            // We must have a URI
            res.set_uri(stream.url);
            res.set_title(stream.title);

            // Set the rest of the attributes
            res["mascot"] = stream.thumbnail;
            res["subtitle"] = stream.name;

            // Push the result
            if (!reply->push(res)) {
                // If we fail to push, it means the query has been cancelled.
                // So don't continue;
                return;
            }
        }

    } catch (domain_error &e) {
        // Handle exceptions being thrown by the client API
        cerr << e.what() << endl;
        reply->error(current_exception());
    }
}

