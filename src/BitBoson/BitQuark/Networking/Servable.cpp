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

#include <iostream>
#include <mutex>
#include <string>
#include <regex>
#include <thread>
#include <functional>
#include <unordered_map>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <corvusoft/restbed/service.hpp>
#include <corvusoft/restbed/session.hpp>
#include <corvusoft/restbed/request.hpp>
#include <corvusoft/restbed/response.hpp>
#include <corvusoft/restbed/resource.hpp>
#include <corvusoft/restbed/settings.hpp>
#include <BitBoson/BitQuark/Networking/Servable.h>

using namespace BitBoson;
using namespace BitBoson::BitQuark;

/**
 * Constructor used to setup the servable instace
 *
 * @param port Integer representing the port to listen on
 * @param isAuthenticated Boolean indicating whether the REST API
 *                        requires authentication or not
 */
Servable::Servable(int port, bool isAuthenticated)
{

    // Lock the thread for safe operation
    std::unique_lock<std::mutex> lock(_lock);

    // Save the port for later use
    _port = port;

    // Setup the settings object
    _settings = std::make_shared<restbed::Settings>();
    _settings->set_port(port);
    _settings->set_default_header("Connection", "close");

    // Setup the service object
    _service = std::make_shared<restbed::Service>();

    // Add-in authentication (if desired)
    // TODO: Implement authentication handler
    //if (isAuthenticated)
    //    _service->set_authentication_handler(
    //            std::bind(&Servable::authenticationHandler, this,
    //                std::placeholders::_1, std::placeholders::_2));

    // Setup the async job-handler for running the server
    _isRunning = std::make_shared<StandardModel::ThreadSafeFlag>();
}

/**
 * Virtual function used to start the service
 * NOTE: This is a non-blocking operation
 *
 * @param workerThreads Integer represeting the number of threads
 */
void Servable::start(int workerThreads)
{

    // Lock the thread for safe operation
    std::unique_lock<std::mutex> lock(_lock);

    // Only try to start the task if it hasn't been
    // started already (or attempted to)
    if (!_isRunning->getValue())
    {

        // Setup the concurrency logic for multi-threading
        // TODO: Implement actual concurrency - results in sanitize-thread data race errors
        //unsigned int numThreads = (workerThreads <= 0 ? std::thread::hardware_concurrency() : workerThreads);
        //numThreads = (numThreads <= 0 ? 4 : numThreads);
        //_settings->set_worker_limit(numThreads);

        // Setup the running variable as true
        _isRunning->setValue(true);

        // Actually start the server using the background thread
        _backgroundThread = std::make_shared<std::thread>(
            [this]()
            {
                _service->start(_settings);
            });
    }
}

/**
 * Function used to add a Http Listener route to the servable
 *
 * @param method HttpMethod indicating the method to add the
 *               listener onto
 * @param route String representing the route to add the
 *               listener onto
 * @param routeArg String representing the trailing route-argument
 *                 to use as a part of the processing
 * @param handlerFunction Callback Function used to handle
 *                        all requests made on this new route
 */
void Servable::addListener(HttpMethod method, const std::string& route,
        const std::string& routeArg,
        std::function<ResponseObj(std::unordered_map<std::string, std::string>&,
            std::unordered_map<std::string, std::string>&, const std::string&)> handlerFunction)
{

    // Create the rest-bed method handler resource
    auto resource = std::make_shared<restbed::Resource>();

    // Setup the path (including the route-argument if desired)
    if (routeArg.empty())
        resource->set_path(route);
    else
        resource->set_path(route + "/{" + routeArg + ": .*}");

    // Setup the resource to handle GET requests if desired
    if (method == HttpMethod::GET)
        resource->set_method_handler("GET",
            [handlerFunction, routeArg](const std::shared_ptr<restbed::Session> session)
        {
            genericHandlerFunction(session, routeArg, handlerFunction);
        });

    // Setup the resource to handle POST requests if desired
    else if (method == HttpMethod::POST)
        resource->set_method_handler("POST",
            [handlerFunction, routeArg](const std::shared_ptr<restbed::Session> session)
        {
            genericHandlerFunction(session, routeArg, handlerFunction);
        });

    // Add the resource to the service
    _service->publish(resource);
}

/**
 * Internal static function used to handle the request with all boilder-plate operations
 *
 * @param session Sessing representing the Rest-Bed session object
 * @param routeArg String representing the trailing route-argument
 *                 to use as a part of the processing
 * @param handlerFunction Handler function (pointer) used to handle the request
 */
void Servable::genericHandlerFunction(
    const std::shared_ptr<restbed::Session> session, const std::string& routeArg,
    std::function<ResponseObj(std::unordered_map<std::string, std::string>&,
        std::unordered_map<std::string, std::string>&, const std::string&)> handlerFunction)
{

    // Extract the request information from the session object
    const auto request = session->get_request();

    // Parse all standard header-items
    int contentLength = request->get_header("Content-Length", 0);

    // Mark the request as invalid if it was too large and trim it
    bool wasTooLarge = false;
    if (contentLength > (100 * 1024))
    {
        wasTooLarge = true;
        contentLength = (100 * 1024);
    }

    // Extract the route-argument if it was specified
    std::string routeArgVal;
    if (!routeArg.empty())
        routeArgVal = request->get_path_parameter(routeArg);

    // Get all of the provided header values
    std::unordered_map<std::string, std::string> headerValues;
    for (const auto& headerItem : request->get_headers())
        headerValues[headerItem.first] = headerItem.second;

    // Actually have the session handle the request
    session->fetch(contentLength,
        [handlerFunction, &headerValues, wasTooLarge, routeArgVal]
            (const std::shared_ptr<restbed::Session> session, const restbed::Bytes& body)
    {

        // If the request as too large, return appropriate error code
        if (wasTooLarge)
        {
            std::string returnJson = "Failed to read HTTP Request: Request Body Too Long";
            session->close(400, returnJson,
                {{"Content-Length", std::to_string(returnJson.size())}});
        }

        // Only continue if the session was not too large
        if (!wasTooLarge)
        {

            // Extract the JSON body from the request
            std::string bodyDataRaw = "";
            for (unsigned long ii = 0; ii < body.size(); ii++)
                bodyDataRaw += body[ii];

            // Parse the body data into a JSON object and then
            // get all of the provided body values
            bool jsonBodyPresentAndHasErrors = false;
            std::unordered_map<std::string, std::string> bodyValues;
            if (!bodyDataRaw.empty())
            {

                // Attempt to actually do the JSON object parsing
                rapidjson::Document jsonDoc;
                rapidjson::ParseResult parseResult = jsonDoc.Parse(bodyDataRaw.c_str());

                // Handle the case where the JSON parsing was unsuccessful
                if (!parseResult)
                {

                    // Return an error to the caller about the invalid JSON object body
                    std::string returnJson = "Failed to read HTTP Request: Invalid JSON Body";
                    session->close(400, returnJson,
                        {{"Content-Length", std::to_string(returnJson.size())}});

                    // Mark that there was a body with errors
                    jsonBodyPresentAndHasErrors = true;
                }

                // Handle the case where the JSON parsing was successful
                if (parseResult)
                {

                    // Actually parse the body values for handling later
                    for (rapidjson::Value::ConstMemberIterator itr = jsonDoc.MemberBegin();
                            itr != jsonDoc.MemberEnd(); ++itr)
                        if (jsonDoc[itr->name.GetString()].IsString())
                            bodyValues[itr->name.GetString()] = jsonDoc[itr->name.GetString()].GetString();
                }
            }

            // Only continue if there were no errors in parsing the JSON body (if present)
            if (!jsonBodyPresentAndHasErrors)
            {

                // Call the underlying handler function
                auto response = handlerFunction(headerValues, bodyValues, routeArgVal);

                // Convert the unordered map in the response to a json object string
                std::string returnMsg;
                for (const auto& reponseItem : response.body)
                    returnMsg += "\"" + reponseItem.first + "\":\"" + reponseItem.second + "\",";
                if (!returnMsg.empty())
                {
                    returnMsg = returnMsg.substr(0, returnMsg.size() - 1);
                    returnMsg = "{" + returnMsg + "}";
                }

                // Close the session object with the return information for the function
                session->close(response.code, returnMsg,
                    {{"Content-Length", std::to_string(returnMsg.size())}});
            }
        }

        // Ensure that the session is closed
        if (session->is_open())
        {
            std::string returnJson = "Invalid HTTP Request: Internal Error";
            session->close(500, returnJson,
                {{"Content-Length", std::to_string(returnJson.size())}});
        }
    });
}

/**
 * Destructor used to cleanup the instance and stop
 * all background processes if they are running
 */
Servable::~Servable()
{

    // Lock the thread for safe operation
    std::unique_lock<std::mutex> lock(_lock);

    // Stop the background service
    _service->stop();

    // Wait for the thread to complete (if it exists)
    if (_backgroundThread != nullptr)
        _backgroundThread->join();
    _backgroundThread = nullptr;

    // Indicate that the background service has stopped
    _isRunning->setValue(false);
}
