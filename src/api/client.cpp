#include <api/client.h>

#include <core/net/error.h>
#include <core/net/http/client.h>
#include <core/net/http/content_type.h>
#include <core/net/http/response.h>
#include <QVariantMap>
#include <iostream>
#include <typeinfo>

namespace http = core::net::http;
namespace net = core::net;

using namespace api;
using namespace std;

Client::Client(Config::Ptr config) :
    config_(config), cancelled_(false) {
}


void Client::get(const net::Uri::Path &path, const net::Uri::QueryParameters &parameters, QJsonDocument &root) {
    // Create a new HTTP client
    auto client = http::make_client();

    // Start building the request configuration
    http::Request::Configuration configuration;

    // Build the URI from its components
    net::Uri uri = net::make_uri(config_->apiroot, path, parameters);
    configuration.uri = client->uri_to_string(uri);

    std::cout << "client.cpp32 " << configuration.uri << std::endl;

    // Give out a user agent string
    configuration.header.add("User-Agent", config_->user_agent);

    // Build a HTTP request object from our configuration
    auto request = client->head(configuration);

    try {
        // Synchronously make the HTTP request
        // We bind the cancellable callback to #progress_report
        auto response = request->execute(
                    bind(&Client::progress_report, this, placeholders::_1));

        // Check that we got a sensible HTTP status code
        if (response.status != http::Status::ok) {
            throw domain_error(response.body);
        }

        // Parse the JSON from the response
        root = QJsonDocument::fromJson(response.body.c_str());

        // Open weather map API error code can either be a string or int
        QVariant cod = root.toVariant().toMap()["status"];

//        QVariantMap variant = root.toVariant().toMap();

//        for (int i=0; i<variant["status"].toList().size(); i++){
//            std::cout << "client.cpp60 " << variant["status"].toList()[i].toString().toStdString() << std::endl;
//        }

        if ((cod.canConvert<QString>() && cod.toString() != "200")
                || (cod.canConvert<unsigned int>() && cod.toUInt() != 200)) {
            throw domain_error(root.toVariant().toMap()["message"].toString().toStdString());
        }

    } catch (net::Error &) {
    }
}

std::deque<std::string> Client::query_deps() {
    QJsonDocument root;
    std::cout << "client.cpp75" << std::endl;

    std::deque<std::string> departments;

    std::cout << "client.cpp79" << std::endl;
    get( { "games", "top" }, { {"limit", "10"} }, root);

    QVariantMap returnMap = root.toVariant().toMap();
    for (QVariantMap::const_iterator iter = returnMap.begin(); iter != returnMap.end(); ++iter) {
        departments.emplace_back(iter.key().toStdString());
    }
    return departments;
}

Client::Streams Client::query_streams(const string &query) {
    QJsonDocument root;

    // Build a URI and get the contents
    // The fist parameter forms the path part of the URI.
    // The second parameter forms the CGI parameters.
    get( { "search", "streams" }, {{ "q", query }, {"limit", "10"}}, root);
    // https://api.twitch.tv/kraken/search/streams/q=<query>&limit=10

    Streams result;

    QVariantMap variant = root.toVariant().toMap();

    // Iterate through the stream data
    for (const QVariant &i : variant["streams"].toList()) {

        QVariantMap item = i.toMap();
        std::string game = item["game"].toString().toStdString();
        QVariantMap previews = item["preview"].toMap();
        item = item["channel"].toMap();

        // Add a result to the weather list
        result.stream.emplace_back(
                    Stream { item["status"].toString().toStdString(),
                             game,
                             item["name"].toString().toStdString(),
//                             item["viewers"].toString().toStdString(),
                             item["url"].toString().toStdString(),
                             item["logo"].toString().toStdString(),
                             previews["small"].toString().toStdString()
                    });

    }

    return result;
}

http::Request::Progress::Next Client::progress_report(
        const http::Request::Progress&) {

    return cancelled_ ?
                http::Request::Progress::Next::abort_operation :
                http::Request::Progress::Next::continue_operation;
}

void Client::cancel() {
    cancelled_ = true;
}

Config::Ptr Client::config() {
    return config_;
}

