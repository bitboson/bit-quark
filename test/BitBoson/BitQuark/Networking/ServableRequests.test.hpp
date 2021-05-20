/* This file is part of bit-quark.
 *
 * Copyright (c) BitBoson
 *
 * bit-quark is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bit-quark is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bit-quark.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Written by:
 *     - Tyler Parcell <OriginLegend>
 */

#ifndef BITQUARK_SERVABLEREQUESTS_TEST_HPP
#define BITQUARK_SERVABLEREQUESTS_TEST_HPP

#include <catch.hpp>
#include <string>
#include <cpr/cpr.h>
#include <unordered_map>
#include <BitBoson/StandardModel/Crypto/Crypto.h>
#include <BitBoson/BitQuark/Networking/Servable.h>
#include <BitBoson/BitQuark/Networking/Requests.h>

using namespace BitBoson;
using namespace BitBoson::BitQuark;

class HelloServable : public Servable
{
    public:
        HelloServable(int port) : Servable(port)
        {

            // Setup the GET listener
            addListener(HttpMethod::GET, "/hello", "",
                [this](std::unordered_map<std::string, std::string>& headers,
                    std::unordered_map<std::string, std::string>& body,
                    const std::string& routeArg) -> ResponseObj
                {
                    return this->handleGetHello(headers, body, routeArg);
                });

            // Setup the GET-echo-headers listener
            addListener(HttpMethod::GET, "/helloheaders", "",
                [this](std::unordered_map<std::string, std::string>& headers,
                    std::unordered_map<std::string, std::string>& body,
                    const std::string& routeArg) -> ResponseObj
                {
                    return this->handleGetHeadersHello(headers, body, routeArg);
                });

            // Setup the GET-echo argument listener
            addListener(HttpMethod::GET, "/helloecho", "echo",
                [this](std::unordered_map<std::string, std::string>& headers,
                    std::unordered_map<std::string, std::string>& body,
                    const std::string& routeArg) -> ResponseObj
                {
                    return this->handleGetHelloEcho(headers, body, routeArg);
                });

            // Setup the POST listener
            addListener(HttpMethod::POST, "/hello2", "",
                [this](std::unordered_map<std::string, std::string>& headers,
                    std::unordered_map<std::string, std::string>& body,
                    const std::string& routeArg) -> ResponseObj
                {
                    return this->handlePostHello(headers, body, routeArg);
                });
        }

        /**
         * Destructor used to cleanup the instance
         */
        virtual ~HelloServable() = default;

    private:
        ResponseObj handleGetHello(std::unordered_map<std::string, std::string>& headers,
            std::unordered_map<std::string, std::string>& body, const std::string& routeArg)
        {
            return ResponseObj{200, {{"message", "world"}}};
        }
        ResponseObj handleGetHeadersHello(std::unordered_map<std::string, std::string>& headers,
            std::unordered_map<std::string, std::string>& body, const std::string& routeArg)
        {
            return ResponseObj{200, headers};
        }
        ResponseObj handleGetHelloEcho(std::unordered_map<std::string, std::string>& headers,
            std::unordered_map<std::string, std::string>& body, const std::string& routeArg)
        {
            return ResponseObj{200, {{"message", routeArg}}};
        }
        ResponseObj handlePostHello(std::unordered_map<std::string, std::string>& headers,
            std::unordered_map<std::string, std::string>& body, const std::string& routeArg)
        {
            return ResponseObj{201, {{"message", "world"}, {"name", body["name"]}}};
        }
};

TEST_CASE ("Generic Servable Test", "[ServableRequestsTest]")
{

    // Create a simple servable instance
    HelloServable helloServer(12345);
    helloServer.start();

    // Make a GET request to the simple server
    auto response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:12345/hello", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 1);
    REQUIRE(response.body["message"] == "world");

    // Make a POST request to the simple server
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:12345/hello2", {{"name", "tyler"}});
    REQUIRE(response.code == 201);
    REQUIRE(response.body.size() == 2);
    REQUIRE(response.body["message"] == "world");
    REQUIRE(response.body["name"] == "tyler");

    // Make a GET request to the echo-argument endpoint on the simple server
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:12345/helloecho/world", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() >= 1);
    REQUIRE(response.body["message"] == "world");
}

TEST_CASE ("Specified Multi-Threaded Servable Test", "[ServableRequestsTest]")
{

    // Create a simple servable instance
    HelloServable helloServer(12345);
    helloServer.start(10);

    // Make a GET request to the simple server
    auto response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:12345/hello", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 1);
    REQUIRE(response.body["message"] == "world");

    // Make a POST request to the simple server
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:12345/hello2", {{"name", "tyler"}});
    REQUIRE(response.code == 201);
    REQUIRE(response.body.size() == 2);
    REQUIRE(response.body["message"] == "world");
    REQUIRE(response.body["name"] == "tyler");

    // Make a GET request to the echo-argument endpoint on the simple server
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:12345/helloecho/world", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() >= 1);
    REQUIRE(response.body["message"] == "world");
}

TEST_CASE ("Specified Single-Threaded Servable Test", "[ServableRequestsTest]")
{

    // Create a simple servable instance
    HelloServable helloServer(12345);
    helloServer.start(1);

    // Make a GET request to the simple server
    auto response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:12345/hello", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 1);
    REQUIRE(response.body["message"] == "world");

    // Make a POST request to the simple server
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:12345/hello2", {{"name", "tyler"}});
    REQUIRE(response.code == 201);
    REQUIRE(response.body.size() == 2);
    REQUIRE(response.body["message"] == "world");
    REQUIRE(response.body["name"] == "tyler");

    // Make a GET request to the echo-argument endpoint on the simple server
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:12345/helloecho/world", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() >= 1);
    REQUIRE(response.body["message"] == "world");
}

TEST_CASE ("Too Large Body Size for Servable", "[ServableRequestsTest]")
{

    // Create a simple servable instance
    HelloServable helloServer(12345);
    helloServer.start();

    // Create a very unordered-map (10K hashes) to send as the body
    std::unordered_map<std::string, std::string> veryLongBody;
    for (auto ii = 0; ii < 10000; ii++)
        veryLongBody[std::to_string(ii)] = StandardModel::Crypto::sha256(std::to_string(ii));

    // Make a POST request to the simple server
    auto response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:12345/hello2", veryLongBody);
    REQUIRE(response.code == 400);
    REQUIRE(response.body.size() == 2);
    REQUIRE(response.body["Status"] == "Error");
    REQUIRE(response.body["Message"] == "Failed to read HTTP Request: Request Body Too Long");
}

TEST_CASE ("Invalid JSON Body Supplied to Servable", "[ServableRequestsTest]")
{

    // Create a simple servable instance
    HelloServable helloServer(12345);
    helloServer.start();

    // Manually construct a bad JSON body request for the servable
    std::string url = "http://localhost:12345/hello2";
    std::string bodyString = "ThisIsAnInvalidJsonBodyString";
    auto responseRaw = cpr::Post(cpr::Url{url}, cpr::Body{bodyString}, cpr::Timeout{1000});

    // Validate that the response was the appropriate error
    REQUIRE(responseRaw.status_code == 400);
    REQUIRE(responseRaw.text == "Failed to read HTTP Request: Invalid JSON Body");
}

#endif //BITQUARK_SERVABLEREQUESTS_TEST_HPP
