#ifndef API_CLIENT_H_
#define API_CLIENT_H_

#include <api/config.h>

#include <atomic>
#include <deque>
#include <map>
#include <string>
#include <core/net/http/request.h>
#include <core/net/uri.h>

#include <QJsonDocument>

namespace api {

/**
 * Provide a nice way to access the HTTP API.
 *
 * We don't want our scope's code to be mixed together with HTTP and JSON handling.
 */
class Client {
public:

    /**
     * Information about the stream
     */
    struct Stream {
        std::string title;      // Stream Title
        std::string game;       // Channel Game
        std::string name;       // Channel Name
        std::string viewers;    // Stream Viewers
        std::string url;        // Stream URL
        std::string logo;       // Stream Logo URL
        std::string thumbnail;  // Preview URL
    };

    /**
     * A list of weather information
     */
    typedef std::deque<Stream> StreamList;

    struct Streams {
        StreamList stream;
    };

    Client(Config::Ptr config);

    virtual ~Client() = default;

    /**
     * Get the weather forecast for the specified location and duration
     */
    virtual Streams query_streams(const std::string &query);
    virtual std::deque<std::string> query_deps();

    /**
     * Cancel any pending queries (this method can be called from a different thread)
     */
    virtual void cancel();

    virtual Config::Ptr config();

protected:
    void get(const core::net::Uri::Path &path,
             const core::net::Uri::QueryParameters &parameters,
             QJsonDocument &root);
    /**
     * Progress callback that allows the query to cancel pending HTTP requests.
     */
    core::net::http::Request::Progress::Next progress_report(
            const core::net::http::Request::Progress& progress);

    /**
     * Hang onto the configuration information
     */
    Config::Ptr config_;

    /**
     * Thread-safe cancelled flag
     */
    std::atomic<bool> cancelled_;
};

}

#endif // API_CLIENT_H_
