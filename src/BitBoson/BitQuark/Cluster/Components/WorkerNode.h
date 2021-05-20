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

#ifndef BITQUARK_WORKERNODE_H
#define BITQUARK_WORKERNODE_H

#include <memory>
#include <vector>
#include <BitBoson/StandardModel/Threading/AsyncEventLoop.hpp>
#include <BitBoson/BitQuark/Networking/Servable.h>

using namespace BitBoson;
namespace BitBoson::BitQuark
{

    class WorkerNode : public Servable
    {

        // Private structures
        private:
            struct KnownMasterNode
            {
                long lastContacted;
                std::string id;
                std::string url;
            };

        // Private member variables
        private:
            bool _inCluster;
            long _masterTimout;
            std::string _nodeId;
            std::string _nodeUrl;
            std::mutex _workerLock;
            unsigned long _currMasterNode;
            std::vector<KnownMasterNode> _knownMasterNodes;
            std::shared_ptr<StandardModel::AsyncEventLoop> _workerEventLoop;

        // Public member functions
        public:

            /**
             * Constructor used to setup the servable for the worker node
             *
             * @param hostname String representing the hostname for the server
             * @param port Integer representing the port to serve/listen on
             * @param nodeId String representing a node id for the worker node
             */
            WorkerNode(const std::string& hostname, int port,
                    const std::string& nodeId="");

            /**
             * Function used to set the time-out value for the master node before
             * attempting to round-robbin connect to all other known master nodes
             *
             * @param timeout Long reprenting the approximate timeout in seconds
             * @return Boolean indicating whether the value was accepted or not
             */
            bool setMasterNodeTimeout(long timeout);

            /**
             * Function used to get a list of connected masters to this node
             *
             * @return Vector of Strings representing the master ids
             */
            std::vector<std::string> getKnownMasters();

            /**
             * Function used to get the connected master for this node
             *
             * @return String representing the connected master id
             */
            std::string getConnectedMaster();

            /**
             * Function used to check if the node is in quorum or not
             *
             * @return Boolean indicating if the node is in quorum
             */
            bool isInQuorum();

            /**
             * Destructor used to cleanup the instance
             */
            virtual ~WorkerNode();

        // Private member functions
        private:

            /**
             * Internal function used to handle the worker event loop
             */
            void handleWorkerEventLoop();

            /**
             * Internal handler function used to get the internal status of the worker node
             *
             * @param headers Unordered-map of Strings representing header values
             * @param body Unordered-map of Strings representing request body values
             * @param routeArg String representing additional routing arguments
             * @return ResponseObj representing the response to the request handled
             */
            ResponseObj handlerGetInternalWorkerStatus(std::unordered_map<std::string, std::string>& headers,
                std::unordered_map<std::string, std::string>& body, const std::string& routeArg);

            /**
             * Internal handler function used to post a join request for the worker to a master node
             *
             * @param headers Unordered-map of Strings representing header values
             * @param body Unordered-map of Strings representing request body values
             * @param routeArg String representing additional routing arguments
             * @return ResponseObj representing the response to the request handled
             */
            ResponseObj handlerPostInternalMasterJoin(std::unordered_map<std::string, std::string>& headers,
                std::unordered_map<std::string, std::string>& body, const std::string& routeArg);
    };
}

#endif //BITQUARK_WORKERNODE_H