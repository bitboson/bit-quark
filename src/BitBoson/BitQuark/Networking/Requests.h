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

#ifndef BITQUARK_REQUESTS_H
#define BITQUARK_REQUESTS_H

#include <string>
#include <unordered_map>
#include <BitBoson/BitQuark/Networking/Servable.h>

// Setup the place-holders namespace reference/usage
using namespace std::placeholders;

using namespace BitBoson;
namespace BitBoson::BitQuark
{

    namespace Requests
    {

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
        Servable::ResponseObj makeRequest(Servable::HttpMethod method,
                const std::string& url, std::unordered_map<std::string, std::string> body,
                int timeout = 10000, int retryLimit = -1);
    }
}

#endif //BITQUARK_REQUESTS_H
