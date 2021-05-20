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

#ifndef BITQUARK_MASTERNODE_H
#define BITQUARK_MASTERNODE_H

#include <vector>
#include <unordered_map>
#include <BitBoson/StandardModel/Threading/ThreadPool.hpp>
#include <BitBoson/StandardModel/Threading/AsyncQueue.hpp>
#include <BitBoson/StandardModel/Threading/AsyncEventLoop.hpp>
#include <BitBoson/BitQuark/Networking/Servable.h>
#include <BitBoson/BitQuark/Storage/S3DataStore.h>
#include <BitBoson/BitQuark/Storage/S3Credentials.h>

using namespace BitBoson;
namespace BitBoson::BitQuark
{

    class MasterNode : public Servable
    {

        // Private structures
        private:
            struct MasterNodeState
            {
                bool contactable;
                std::string id;
                std::string url;
            };

        // Private member variables
        private:
            long _leftTimeout;
            long _workerTimeout;
            std::string _nodeId;
            std::string _nodeUrl;
            std::mutex _masterLock;
            std::vector<std::string> _leftMasterNodes;
            std::shared_ptr<S3Credentials> _s3Credentials;
            std::shared_ptr<S3DataStore> _s3DataStore;
            std::shared_ptr<StandardModel::AsyncEventLoop> _masterEventLoop;
            std::shared_ptr<StandardModel::AsyncEventLoop> _workerEventLoop;
            std::unordered_map<std::string, long> _leftMasterNodeTimes;
            std::unordered_map<std::string, long> _connectedWorkerNodes;
            std::shared_ptr<StandardModel::ThreadPool<std::string>> _masterThreadPool;
            std::unordered_map<std::string, MasterNodeState> _masterNodes;
            std::unordered_map<std::string, std::string> _workerNodes; // <--- TODO: CHECK IF WE CAN DELETE
            std::shared_ptr<StandardModel::AsyncQueue<std::pair<std::string, std::string>>> _masterNodesToJoin;

        // Public member functions
        public:

            /**
             * Constructor used to setup the servable for the master node
             *
             * @param hostname String representing the hostname for the server
             * @param port Integer representing the port to serve/listen on
             * @param nodeId String representing a node id for the master node
             */
            MasterNode(const std::string& hostname, int port,
                    const std::string& nodeId="");

            /**
             * Function used to set the time-out value for nodes which have
             * left the cluster so that we don't have to keep track of them
             * NOTE: This means that self-healing can occur after this point
             *
             * @param timeout Long reprenting the approximate timeout in seconds
             * @return Boolean indicating whether the value was accepted or not
             */
            bool setLeftNodeTimeout(long timeout);

            /**
             * Function used to set the time-out value for worker nodes which
             * we will remove from our list of connected workers so we can
             * re-distribute work, etc accordingly
             *
             * @param timeout Long reprenting the approximate timeout in seconds
             * @return Boolean indicating whether the value was accepted or not
             */
            bool setWorkerNodeTimeout(long timeout);

            /**
             * Function used to get a list of connected workers to this node
             *
             * @return Vector of Strings representing the worker ids
             */
            std::vector<std::string> getConnectedWorkers();

            /**
             * Function used to get a list of connected masters to this node
             *
             * @return Vector of Strings representing the master ids
             */
            std::vector<std::string> getConnectedMasters();

            /**
             * Function used to get the URL for the given connected node
             *
             * @param nodeId String representing the connected node id
             * @return String representing the URL for the connected node
             */
            std::string getUrlForConnectedMasterNode(const std::string& nodeId);

            /**
             * Function used to check if the node is in quorum or not
             *
             * @return Boolean indicating if the node is in quorum
             */
            bool isInQuorum();

            /**
             * Destructor used to cleanup the instance
             */
            virtual ~MasterNode();

        // Private member functions
        private:

            /**
             * Internal function used to handle the master event loop
             */
            void handleMasterEventLoop();

            /**
             * Internal function used to handle the worker event loop
             */
            void handleWorkerEventLoop();

            /**
             * Internal function used to handle the asynchronous request for a
             * master-node's status to update status/quorum information
             *
             * @param nodeId String representing the node id to request
             */
            void handleMasterNodeStatusRequest(const std::string& nodeId);

            /**
             * Internal handler function used to get the internal status of the master node
             *
             * @param headers Unordered-map of Strings representing header values
             * @param body Unordered-map of Strings representing request body values
             * @param routeArg String representing additional routing arguments
             * @return ResponseObj representing the response to the request handled
             */
            ResponseObj handlerGetInternalMasterStatus(std::unordered_map<std::string, std::string>& headers,
                std::unordered_map<std::string, std::string>& body, const std::string& routeArg);

            /**
             * Internal handler function used to post a join request for another master node
             *
             * @param headers Unordered-map of Strings representing header values
             * @param body Unordered-map of Strings representing request body values
             * @param routeArg String representing additional routing arguments
             * @return ResponseObj representing the response to the request handled
             */
            ResponseObj handlerPostInternalMasterJoin(std::unordered_map<std::string, std::string>& headers,
                std::unordered_map<std::string, std::string>& body, const std::string& routeArg);

            /**
             * Internal handler function used to post a leave request for another master node
             *
             * @param headers Unordered-map of Strings representing header values
             * @param body Unordered-map of Strings representing request body values
             * @param routeArg String representing additional routing arguments
             * @return ResponseObj representing the response to the request handled
             */
            ResponseObj handlerPostInternalMasterLeave(std::unordered_map<std::string, std::string>& headers,
                std::unordered_map<std::string, std::string>& body, const std::string& routeArg);

            /**
             * Internal handler function used to get the status of the cluster externally
             *
             * @param headers Unordered-map of Strings representing header values
             * @param body Unordered-map of Strings representing request body values
             * @param routeArg String representing additional routing arguments
             * @return ResponseObj representing the response to the request handled
             */
            ResponseObj handlerGetClusterStatus(std::unordered_map<std::string, std::string>& headers,
                std::unordered_map<std::string, std::string>& body, const std::string& routeArg);
    };
}

#endif //BITQUARK_MASTERNODE_H