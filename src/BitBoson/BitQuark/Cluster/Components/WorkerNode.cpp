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

#include <algorithm>
#include <BitBoson/StandardModel/Crypto/Crypto.h>
#include <BitBoson/BitQuark/Networking/Requests.h>
#include <BitBoson/BitQuark/Cluster/Components/WorkerNode.h>

using namespace BitBoson;
using namespace BitBoson::BitQuark;

/**
 * Constructor used to setup the servable for the worker node
 *
 * @param hostname String representing the hostname for the server
 * @param port Integer representing the port to serve/listen on
 * @param nodeId String representing a node id for the worker node
 */
WorkerNode::WorkerNode(const std::string& hostname, int port,
        const std::string& nodeId) : Servable(port)
{

    // TODO: Sanitize node id to be alpha-numeric

    // Setup the default timeout for master nodes
    // to be 1 minute (60 seconds)
    _masterTimout = 60;

    // Initialize the in-cluster value to false
    _inCluster = false;

    // Initialize the current master node reference to 0
    _currMasterNode = 0;

    // Setup the node id based on what was given
    _nodeId = (nodeId.empty() ? StandardModel::Crypto::getRandomSha256() : nodeId);
    _nodeUrl = ("http://" + hostname + ":" + std::to_string(port));

    // Setup the asynchronous event loops
    _workerEventLoop = std::make_shared<StandardModel::AsyncEventLoop>(
        [this]() {
            handleWorkerEventLoop();
        });

    // Setup the REST API Listener to get the internal status of the master node
    addListener(HttpMethod::GET, "/internal/worker/status", "",
        [this](std::unordered_map<std::string, std::string>& headers,
            std::unordered_map<std::string, std::string>& body,
            const std::string& routeArg) -> ResponseObj
        {
            return this->handlerGetInternalWorkerStatus(headers, body, routeArg);
        });

    // Setup the REST API Listener to post a join request for the worker to a master node
    addListener(HttpMethod::POST, "/internal/worker/join", "",
        [this](std::unordered_map<std::string, std::string>& headers,
            std::unordered_map<std::string, std::string>& body,
            const std::string& routeArg) -> ResponseObj
        {
            return this->handlerPostInternalMasterJoin(headers, body, routeArg);
        });
}

/**
 * Function used to set the time-out value for the master node before
 * attempting to round-robbin connect to all other known master nodes
 *
 * @param timeout Long reprenting the approximate timeout in seconds
 * @return Boolean indicating whether the value was accepted or not
 */
bool WorkerNode::setMasterNodeTimeout(long timeout)
{

    // Create a return flag
    bool retFlag = false;

    // Only set the value if it is at least 30 seconds
    if (timeout >= 30)
    {

        // Lock before we attempt to access shared-memory
        std::unique_lock<std::mutex> lock(_workerLock);

        // Actually set the value
        _masterTimout = timeout;

        // Mark the operation as successful
        retFlag = true;
    }

    // Return the return flag
    return retFlag;
}

/**
 * Function used to get a list of connected masters to this node
 *
 * @return Vector of Strings representing the master ids
 */
std::vector<std::string> WorkerNode::getKnownMasters()
{

    // Create a return vector
    std::vector<std::string> retVect;

    // Extract the knows master nodes ids as the return vector
    // Do this in a separate context to leverage RAII for the mutex/lock
    {

        // Lock before we attempt to access shared-memory
        std::unique_lock<std::mutex> lock(_workerLock);

        // Only continue if we have any master nodes at all
        if (!_knownMasterNodes.empty())
        {

            // Copy-over the list of known master node ids into
            // the return vector
            for (const auto& knownMaster : _knownMasterNodes)
                retVect.push_back(knownMaster.id);
        }
    }

    // Return the return vector
    return retVect;
}

/**
 * Function used to get the connected master for this node
 *
 * @return String representing the connected master id
 */
std::string WorkerNode::getConnectedMaster()
{

    // Create a return value
    std::string retValue;

    // Extract the connected master node id as the return value
    // Do this in a separate context to leverage RAII for the mutex/lock
    {

        // Lock before we attempt to access shared-memory
        std::unique_lock<std::mutex> lock(_workerLock);

        // Only continue if we have any master nodes at all
        if (!_knownMasterNodes.empty())
        {

            // Extract the Id for the master node we are connected to
            retValue = _knownMasterNodes[_currMasterNode].id;
        }
    }

    // Return the return value
    return retValue;
}

/**
 * Function used to check if the node is in quorum or not
 *
 * @return Boolean indicating if the node is in quorum
 */
bool WorkerNode::isInQuorum()
{

    // Create a return flag
    bool retFlag = false;

    // Check whether this worker is in quorum or not
    // Do this in a separate context to leverage RAII for the mutex/lock
    {

        // Lock before we attempt to access shared-memory
        std::unique_lock<std::mutex> lock(_workerLock);

        // Copy-over the in-cluster value as in-quorum for returning
        retFlag = _inCluster;
    }

    // Return the return flag
    return retFlag;
}

/**
 * Internal function used to handle the worker event loop
 */
void WorkerNode::handleWorkerEventLoop()
{

    // Create a flag to keep track of whether we are in the cluster
    // or not so that we can update the global state accordingly
    bool localIsInClusterValue = false;

    // Extract the currently known master node state locally
    // Do this in a separate context to leverage RAII for the mutex/lock
    std::string thisNodeId;
    bool hasMasterNodes = false;
    KnownMasterNode connectedMasterNode;
    {

        // Lock before we attempt to access shared-memory
        std::unique_lock<std::mutex> lock(_workerLock);

        // Only continue if we have any master nodes at all
        if (!_knownMasterNodes.empty())
        {

            // Copy over the local master node state
            // and any other relevant state
            thisNodeId = _nodeId;
            connectedMasterNode = _knownMasterNodes[_currMasterNode];

            // Indicate that we have master nodes
            hasMasterNodes = true;
        }
    }

    // Only continue if we have any master nodes at all
    if (hasMasterNodes)
    {

        // Make a request to the currently connected master node
        auto response = Requests::makeRequest(
                Servable::HttpMethod::GET,
                connectedMasterNode.url + "/internal/master/status/" + thisNodeId, {});

        // If the master node was not contactable or the master node
        // is not in the current cluster quorum count, increment the
        // time we haven't been able to contact it, otherwise reset
        // the time we haven't been able to contact it
        if ((response.code >= 300) || (response.body["QuorumMet"] != "True"))
            connectedMasterNode.lastContacted += 5;
        else
            connectedMasterNode.lastContacted = 0;

        // Write-back the new delay state to the currently connected node
        // Do this in a separate context to leverage RAII for the mutex/lock
        {

            // Lock before we attempt to access shared-memory
            std::unique_lock<std::mutex> lock(_workerLock);

            // Actually re-assign the structure
            _knownMasterNodes[_currMasterNode] = connectedMasterNode;
        }

        // Determine if we are in the cluster or not so we can update state later
        // This basically required we are connected to a master which has quorum
        if ((response.code < 300) && (response.body["QuorumMet"] == "True"))
            localIsInClusterValue = true;

        // Only perform any adjustments to the master list if we are in
        // the actual cluster (connected to a quorum master node)
        if (localIsInClusterValue)
        {

            // Extract just the nodes in the response from the master node
            // so that we can update our known master list as necessary
            std::vector<std::string> remoteNodes;
            for (const auto& responseItem : response.body)
                if ((responseItem.second == "Connected")
                        || (responseItem.second == "NotConnected"))
                    remoteNodes.push_back(responseItem.first);

            // Essentially we need to do a diff on the response from the master
            // adding missing nodes here and removing ones which are no longer
            // valid on our end (i.e. not in the response from the master node)
            // Do this in a separate context to leverage RAII for the mutex/lock
            {

                // Lock before we attempt to access shared-memory
                std::unique_lock<std::mutex> lock(_workerLock);

                // Find all of the nodes which are missing locally
                // Since these items are added to the end, we don't have to worry
                // about messing-up our current master node index reference value
                for (const auto& remoteNode : remoteNodes)
                {

                    // First check if the item exists locally or not
                    bool itemExistsLocally = false;
                    for (unsigned long ii = 0; ii < _knownMasterNodes.size(); ii++)
                        if (_knownMasterNodes[ii].id == remoteNode)
                            itemExistsLocally = true;

                    // If the item doesn't exist locally, add it
                    if (!itemExistsLocally)
                        _knownMasterNodes.push_back(
                                KnownMasterNode{0, remoteNode, response.body["URL-" + remoteNode]});
                }

                // Find all nodes which exist locally, but do not exist remotely
                // We will then proceed to remove them from the vector
                bool removedItemsLocally = false;
                for (const auto& localNode : _knownMasterNodes)
                {

                    // Determine if the local node exists remotely or not
                    auto doesNodeExistRemotely = true;
                    if(std::find(remoteNodes.begin(),
                            remoteNodes.end(), localNode.id) == remoteNodes.end())
                        if (localNode.id != connectedMasterNode.id)
                            doesNodeExistRemotely = false;

                    // If the local node does not exist remotely, then we
                    // will need to remove it from the local vector
                    if (!doesNodeExistRemotely)
                    {

                        // Find and remove the node in question
                        unsigned long itemToRemove =  _knownMasterNodes.size() + 1;
                        for (unsigned long ii = 0; ii < _knownMasterNodes.size(); ii++)
                            if (_knownMasterNodes[ii].id == localNode.id)
                                itemToRemove = ii;
                        if (itemToRemove < _knownMasterNodes.size())
                        {

                            // Actually remove the item locally
                            _knownMasterNodes.erase(_knownMasterNodes.begin() + itemToRemove);

                            // Take note of the fact that we remove a local item
                            removedItemsLocally = true;
                        }
                    }
                }

                // If we happened to remove a local node, then re-align our index
                // to the node we were previously on and making the request with
                if (removedItemsLocally)
                    for (unsigned long ii = 0; ii < _knownMasterNodes.size(); ii++)
                        if (_knownMasterNodes[ii].id == connectedMasterNode.id)
                            _currMasterNode = ii;
            }
        }

        // If the tine we were last able to contact it exceeds configured
        // requirements, then we can formally search for a new master node
        if (connectedMasterNode.lastContacted > _masterTimout)
        {

            // If we get here, it means we were not able to contact the master
            // node for a while and thus we should try to change master nodes
            // Do this in a separate context to leverage RAII for the mutex/lock
            {

                // Lock before we attempt to access shared-memory
                std::unique_lock<std::mutex> lock(_workerLock);

                // Increment the current master node and handle wrap-around
                _currMasterNode++;
                if (_currMasterNode >= _knownMasterNodes.size())
                    _currMasterNode = 0;

                // Reset the last contacted value for the new node selection
                _knownMasterNodes[_currMasterNode].lastContacted = 0;
            }
        }
    }

    // At the end of the internal state updates, setup whether this node
    // is actually in the cluster or not
    // Do this in a separate context to leverage RAII for the mutex/lock
    {

        // Lock before we attempt to access shared-memory
        std::unique_lock<std::mutex> lock(_workerLock);

        // Update the global state on whether this node is in the
        // cluster or not
        _inCluster = localIsInClusterValue;
    }

    // Wait an additional 5 seconds so that we do not overload
    // each master node which is also handling other requests
    std::this_thread::sleep_for(std::chrono::seconds(5));
}

/**
 * Internal handler function used to get the internal status of the worker node
 *
 * @param headers Unordered-map of Strings representing header values
 * @param body Unordered-map of Strings representing request body values
 * @param routeArg String representing additional routing arguments
 * @return ResponseObj representing the response to the request handled
 */
WorkerNode::ResponseObj WorkerNode::handlerGetInternalWorkerStatus(
    std::unordered_map<std::string, std::string>& headers,
    std::unordered_map<std::string, std::string>& body, const std::string& routeArg)
{

    // Create a return object
    ResponseObj retObj;

    // Get the node connection information from the internal state
    // Do this in a separate context to leverage RAII for the mutex/lock
    std::unordered_map<std::string, std::string> connectionStatus;
    {

        // Lock before we attempt to access shared-memory
        std::unique_lock<std::mutex> lock(_workerLock);

        // Extract the connection information for each node
        for (const auto& nodeState : _knownMasterNodes)
            connectionStatus[nodeState.id] = std::to_string(nodeState.lastContacted);

        // Add-in whether the current worker not is actually in the cluster or not
        // This basically required we are connected to a master which has quorum
        connectionStatus["InCluster"] = (_inCluster ? "True" : "False");

        // Add in which master node we are connected to as well
        if ((!_knownMasterNodes.empty()) && (_currMasterNode < _knownMasterNodes.size()))
            connectionStatus["ConnectedTo"] = _knownMasterNodes[_currMasterNode].id;
        else
            connectionStatus["ConnectedTo"] = "None";
    }

    // Construct a response based on the node information
    // and the build-up unordered map for connection info
    retObj = ResponseObj{200, connectionStatus};

    // Return the response object
    return retObj;
}

/**
 * Internal handler function used to post a join request for the worker to a master node
 *
 * @param headers Unordered-map of Strings representing header values
 * @param body Unordered-map of Strings representing request body values
 * @param routeArg String representing additional routing arguments
 * @return ResponseObj representing the response to the request handled
 */
WorkerNode::ResponseObj WorkerNode::handlerPostInternalMasterJoin(
    std::unordered_map<std::string, std::string>& headers,
    std::unordered_map<std::string, std::string>& body,
    const std::string& routeArg)
{

    // Create a return object
    ResponseObj retObj;

    // Extract the new node details from the supplied body
    // TODO: Add some kind of token for later authentication purposes
    auto nodeId = body["NodeId"];
    auto nodeUrl = body["NodeUrl"];

    // Construct a response object if an item is missing
    if (nodeId.empty())
        retObj = ResponseObj{400, {{"MissingArgument", "NodeId"}}};
    if (nodeUrl.empty())
        retObj = ResponseObj{400, {{"MissingArgument", "NodeUrl"}}};

    // Only continue if the required arguments have been supplied
    if (!nodeId.empty() && !nodeUrl.empty())
    {

        // Add-in the new node details into the node-states map
        // Do this in a separate context to leverage RAII for the mutex/lock
        bool alreadyExistedId = false;
        bool alreadyExistedUrl = false;
        {

            // Lock before we attempt to access shared-memory
            std::unique_lock<std::mutex> lock(_workerLock);

            // Attempt to see if any node with the same id or url exists already
            for (const auto& localMasterNode : _knownMasterNodes)
            {

                // Determine if the id matches an existing node
                if (localMasterNode.id == nodeId)
                    alreadyExistedId = true;

                // Determine if the url matches an existing node
                if (localMasterNode.url == nodeUrl)
                    alreadyExistedUrl = true;
            }

            // Also mark the node as found if it contains any of our own data
            if (nodeId == _nodeId)
                alreadyExistedId = true;
            if (nodeUrl == _nodeUrl)
                alreadyExistedUrl = true;

            // Only proceed to add the item if it doesn't already exist
            if (!alreadyExistedId && !alreadyExistedUrl)
                _knownMasterNodes.push_back(KnownMasterNode{0, nodeId, nodeUrl});
        }

        // Construct a response based on whether the node already existed or not
        if (alreadyExistedId)
            retObj = ResponseObj{400, {{"AddedNode", "False"},
                                       {"NodeId", nodeId},
                                       {"NodeUrl", nodeUrl},
                                       {"Message", "A node with the same ID already exists"}}};
        else if (alreadyExistedUrl)
            retObj = ResponseObj{400, {{"AddedNode", "False"},
                                       {"NodeId", nodeId},
                                       {"NodeUrl", nodeUrl},
                                       {"Message", "A node with the same URL already exists"}}};
        else
            retObj = ResponseObj{201, {{"AddedNode", "True"},
                                       {"NodeId", nodeId},
                                       {"NodeUrl", nodeUrl},
                                       {"Message", "The node will be added to the cluster"}}};
    }

    // Return the response object
    return retObj;
}

/**
 * Destructor used to cleanup the instance
 */
WorkerNode::~WorkerNode()
{

    // Stop the event loop
    _workerEventLoop = nullptr;

    // Lock before we attempt to destroy shared-memory
    std::unique_lock<std::mutex> lock(_workerLock);
}
