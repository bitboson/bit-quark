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
#include <cpr/cpr.h>
#include <functional>
#include <unordered_map>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <BitBoson/BitQuark/Networking/Requests.h>
#include <BitBoson/BitQuark/Networking/Servable.h>

using namespace BitBoson;
using namespace BitBoson::BitQuark;

/**
 * Function used to make a request on the provided endpoint with the
 * provided details (method, headers, body, etc)
 *
 * @param method Servable HTTP Method indicating the method to use
 * @param url String representing the URL/URI for the request
 * @param body Unordered String-String map representing the body "json"
 * @param timeout Integer representing the timeout (in milliseconds) to use
 * @param retryLimit Integer representing the number of retrties to attempt
 * @return Servable Response-Object representing the response information
 */
Servable::ResponseObj Requests::makeRequest(Servable::HttpMethod method,
        const std::string& url, std::unordered_map<std::string, std::string> body,
        int timeout, int retryLimit)
{

    // Create a response/return object
    Servable::ResponseObj returnObj;
    returnObj.code = 400;

    // Enforce a retry limit of at least one
    if (retryLimit <= 0)
        retryLimit = 1;

    // Retry until a return code less than 300 is returned
    // or the retry-limit has been reached
    int currentRetryCount = 0;
    while ((returnObj.code >= 300) && (currentRetryCount < retryLimit))
    {

        // Increment the retry count first thing
        currentRetryCount++;

        // Add-in the request body
        std::string bodyString;
        for (const auto& reponseItem : body)
            bodyString += "\"" + reponseItem.first + "\":\"" + reponseItem.second + "\",";
        if (!bodyString.empty())
        {
            bodyString = bodyString.substr(0, bodyString.size() - 1);
            bodyString = "{" + bodyString + "}";
        }

        // Actually perform the request
        cpr::Response responseRaw;
        if (method == Servable::HttpMethod::GET)
            responseRaw = cpr::Get(cpr::Url{url}, cpr::Timeout{timeout});
        else if (method == Servable::HttpMethod::POST)
            responseRaw = cpr::Post(cpr::Url{url}, cpr::Body{bodyString}, cpr::Timeout{timeout});

        // Parse the results into our version of the response object
        rapidjson::Document jsonDoc;
        rapidjson::ParseResult parseResult = jsonDoc.Parse(responseRaw.text.c_str());
        if (parseResult)
        {

            // Write the JSON object into the return body
            for (rapidjson::Value::ConstMemberIterator itr = jsonDoc.MemberBegin(); itr != jsonDoc.MemberEnd(); ++itr)
                if (jsonDoc[itr->name.GetString()].IsString())
                    returnObj.body[itr->name.GetString()] = jsonDoc[itr->name.GetString()].GetString();

            // Write the status/return code into the return object
            returnObj.code = responseRaw.status_code;
        }

        // Handle the case where the parsing was unsuccessful
        if (!parseResult)
        {

            // Write a standard return body with information about the contents
            returnObj.body["Status"] = "Error";
            returnObj.body["Message"] = responseRaw.text;
        }
    }

    // Return the response/return object
    return returnObj;
}
