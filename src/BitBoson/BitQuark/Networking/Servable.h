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

#ifndef BITQUARK_SERVABLE_H
#define BITQUARK_SERVABLE_H

#include <mutex>
#include <string>
#include <thread>
#include <functional>
#include <unordered_map>
#include <corvusoft/restbed/service.hpp>
#include <corvusoft/restbed/session.hpp>
#include <corvusoft/restbed/request.hpp>
#include <corvusoft/restbed/response.hpp>
#include <corvusoft/restbed/resource.hpp>
#include <corvusoft/restbed/settings.hpp>
#include <BitBoson/StandardModel/Threading/ThreadSafeFlag.h>

using namespace BitBoson;
namespace BitBoson::BitQuark
{

    class Servable
    {

        // Public enumerations/structures
        public:
            enum HttpMethod
            {
                GET,
                POST
            };
            struct ResponseObj
            {
                int code;
                std::unordered_map<std::string, std::string> body;
            };

        // Private member variables
        private:
            int _port;
            std::mutex _lock;
            std::shared_ptr<StandardModel::ThreadSafeFlag> _isRunning;
            std::shared_ptr<restbed::Settings> _settings;
            std::shared_ptr<restbed::Service> _service;
            std::shared_ptr<std::thread> _backgroundThread;

        // Public member functions
        public:

            /**
             * Constructor used to setup the servable instace
             *
             * @param port Integer representing the port to listen on
             * @param isAuthenticated Boolean indicating whether the REST API
             *                        requires authentication or not
             */
            Servable(int port, bool isAuthenticated=false);

            /**
             * Virtual function used to start the service
             * NOTE: This is a non-blocking operation
             *
             * @param workerThreads Integer represeting the number of threads
             */
            virtual void start(int workerThreads=0);

            /**
             * Destructor used to cleanup the instance and stop
             * all background processes if they are running
             */
            virtual ~Servable();

        // Protected member functions
        protected:

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
            void addListener(HttpMethod method, const std::string& route,
                    const std::string& routeArg,
                    std::function<ResponseObj(std::unordered_map<std::string, std::string>&,
                        std::unordered_map<std::string, std::string>&, const std::string&)> handlerFunction);

        // Private member functions
        private:

            /**
             * Internal static function used to handle the request with all boilder-plate operations
             *
             * @param session Sessing representing the Rest-Bed session object
             * @param routeArg String representing the trailing route-argument
             *                 to use as a part of the processing
             * @param handlerFunction Handler function (pointer) used to handle the request
             */
            static void genericHandlerFunction(
                const std::shared_ptr<restbed::Session> session, const std::string& routeArg,
                std::function<ResponseObj(std::unordered_map<std::string, std::string>&,
                    std::unordered_map<std::string, std::string>&, const std::string&)> handlerFunction);

    };
}

#endif //BITQUARK_SERVABLE_H
