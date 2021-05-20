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

#include <thread>
#include <algorithm>
#include <BitBoson/StandardModel/Crypto/Crypto.h>
#include <BitBoson/BitQuark/Networking/Requests.h>
#include <BitBoson/BitQuark/Cluster/Components/MasterNode.h>

using namespace BitBoson;
using namespace BitBoson::BitQuark;

/**
 * Constructor used to setup the servable for the master node
 *
 * @param hostname String representing the hostname for the server
 * @param port Integer representing the port to serve/listen on
 * @param nodeId String representing a node id for the master node
 */
MasterNode::MasterNode(const std::string& hostname, int port,
        const std::string& nodeId) : Servable(port)
{

    // TODO: Sanitize node id to be alpha-numeric

    // Setup the default timeout for removed nodes
    // to be 5 minutes (300 seconds)
    _leftTimeout = 300;

    // Setup the default timeout for worker node
    // to be 30 seconds
    _workerTimeout = 30;

    // Setup the node id based on what was given
    _nodeId = (nodeId.empty() ? StandardModel::Crypto::getRandomSha256() : nodeId);
    _nodeUrl = ("http://" + hostname + ":" + std::to_string(port));

    // Create an asynchronous queue to store additional master nodes
    // this node would like to join if needed
    _masterNodesToJoin = std::make_shared<StandardModel::AsyncQueue<std::pair<std::string, std::string>>>();

    // Setup the thread-pools for making requests to other components
    _masterThreadPool = std::make_shared<StandardModel::ThreadPool<std::string>>(
        [this](std::shared_ptr<std::string> value) {
            handleMasterNodeStatusRequest(*value);
        });

    // Setup the asynchronous event loops
    _masterEventLoop = std::make_shared<StandardModel::AsyncEventLoop>(
        [this]() {
            handleMasterEventLoop();
        });
    _workerEventLoop = std::make_shared<StandardModel::AsyncEventLoop>(
        [this]() {
            handleWorkerEventLoop();
        });

    // Setup the REST API Listener to get the internal status of the master node
    addListener(HttpMethod::GET, "/internal/master/status", "",
        [this](std::unordered_map<std::string, std::string>& headers,
            std::unordered_map<std::string, std::string>& body,
            const std::string& routeArg) -> ResponseObj
        {
            return this->handlerGetInternalMasterStatus(headers, body, routeArg);
        });

    // Setup the REST API Listener to get the internal status of the master node
    addListener(HttpMethod::GET, "/internal/master/status", "worker",
        [this](std::unordered_map<std::string, std::string>& headers,
            std::unordered_map<std::string, std::string>& body,
            const std::string& routeArg) -> ResponseObj
        {
            return this->handlerGetInternalMasterStatus(headers, body, routeArg);
        });

    // Setup the REST API Listener to post a join request for another master node
    addListener(HttpMethod::POST, "/internal/master/join", "",
        [this](std::unordered_map<std::string, std::string>& headers,
            std::unordered_map<std::string, std::string>& body,
            const std::string& routeArg) -> ResponseObj
        {
            return this->handlerPostInternalMasterJoin(headers, body, routeArg);
        });

    // Setup the REST API Listener to post a leave request for another master node
    addListener(HttpMethod::POST, "/internal/master/leave", "",
        [this](std::unordered_map<std::string, std::string>& headers,
            std::unordered_map<std::string, std::string>& body,
            const std::string& routeArg) -> ResponseObj
        {
            return this->handlerPostInternalMasterLeave(headers, body, routeArg);
        });

    // Setup the REST API Listener to get the status of the cluster externally
    addListener(HttpMethod::GET, "/cluster/status", "",
        [this](std::unordered_map<std::string, std::string>& headers,
            std::unordered_map<std::string, std::string>& body,
            const std::string& routeArg) -> ResponseObj
        {
            return this->handlerGetClusterStatus(headers, body, routeArg);
        });
}

/**
 * Function used to set the time-out value for nodes which have
 * left the cluster so that we don't have to keep track of them
 * NOTE: This means that self-healing can occur after this point
 *
 * @param timeout Long reprenting the approximate timeout in seconds
 * @return Boolean indicating whether the value was accepted or not
 */
bool MasterNode::setLeftNodeTimeout(long timeout)
{

    // Create a return flag
    bool retFlag = false;

    // Only set the value if it is at least 30 seconds
    if (timeout >= 30)
    {

        // Lock before we attempt to access shared-memory
        std::unique_lock<std::mutex> lock(_masterLock);

        // Actually set the value
        _leftTimeout = timeout;

        // Mark the operation as successful
        retFlag = true;
    }

    // Return the return flag
    return retFlag;
}

/**
 * Function used to set the time-out value for worker nodes which
 * we will remove from our list of connected workers so we can
 * re-distribute work, etc accordingly
 *
 * @param timeout Long reprenting the approximate timeout in seconds
 * @return Boolean indicating whether the value was accepted or not
 */
bool MasterNode::setWorkerNodeTimeout(long timeout)
{

    // Create a return flag
    bool retFlag = false;

    // Only set the value if it is at least 30 seconds
    if (timeout >= 30)
    {

        // Lock before we attempt to access shared-memory
        std::unique_lock<std::mutex> lock(_masterLock);

        // Actually set the value
        _workerTimeout = timeout;

        // Mark the operation as successful
        retFlag = true;
    }

    // Return the return flag
    return retFlag;
}

/**
 * Function used to get a list of connected workers to this node
 *
 * @return Vector of Strings representing the worker ids
 */
std::vector<std::string> MasterNode::getConnectedWorkers()
{

    // Create a return vector
    std::vector<std::string> retVect;

    // Copy-over all currently connected workers
    // Do this in a separate context to leverage RAII for the mutex/lock
    {

        // Lock before we attempt to access shared-memory
        std::unique_lock<std::mutex> lock(_masterLock);

        // Copy over the worker ids
        for (const auto& workerItem : _connectedWorkerNodes)
            retVect.push_back(workerItem.first);
    }

    // Return the return vector
    return retVect;
}

/**
 * Function used to get a list of connected masters to this node
 *
 * @return Vector of Strings representing the master ids
 */
std::vector<std::string> MasterNode::getConnectedMasters()
{

    // Create a return vector
    std::vector<std::string> retVect;

    // Copy-over all currently connected masters
    // Do this in a separate context to leverage RAII for the mutex/lock
    {

        // Lock before we attempt to access shared-memory
        std::unique_lock<std::mutex> lock(_masterLock);

        // Copy over the master ids (if contactable)
        for (const auto& masterItem : _masterNodes)
            if (masterItem.second.contactable)
                retVect.push_back(masterItem.first);
    }

    // Return the return vector
    return retVect;
}

/**
 * Function used to get the URL for the given connected node
 *
 * @param nodeId String representing the connected node id
 * @return String representing the URL for the connected node
 */
std::string MasterNode::getUrlForConnectedMasterNode(const std::string& nodeId)
{

    // Create a return value
    std::string retVal;

    // Search for the connected node by id for the URL
    // Do this in a separate context to leverage RAII for the mutex/lock
    {

        // Lock before we attempt to access shared-memory
        std::unique_lock<std::mutex> lock(_masterLock);

        // First search for the given node id in the master-node list
        for (const auto& masterItem : _masterNodes)
            if ((masterItem.second.id == nodeId)
                    && (masterItem.second.contactable))
                retVal = masterItem.second.url;
    }

    // Return the return value
    return retVal;
}

/**
 * Function used to check if the node is in quorum or not
 *
 * @return Boolean indicating if the node is in quorum
 */
bool MasterNode::isInQuorum()
{

    // Get the node connection information from the internal state
    // Do this in a separate context to leverage RAII for the mutex/lock
    int numberOfNodes = 0;
    int connectedNodes = 0;
    {

        // Lock before we attempt to access shared-memory
        std::unique_lock<std::mutex> lock(_masterLock);

        // Extract the connection information for each node
        numberOfNodes = _masterNodes.size() + 1;
        for (const auto& nodeState : _masterNodes)
            if (nodeState.second.contactable)
                connectedNodes++;
        connectedNodes++;
    }

    // Return whether quorum is met or not
    return (connectedNodes > (0.5 * numberOfNodes));
}

/**
 * Internal function used to handle the master event loop
 */
void MasterNode::handleMasterEventLoop()
{

    // Enqueue all of the connected node's to be updated
    // via contacting their individual status APIs
    // Do this in a separate context to leverage RAII for the mutex/lock
    {

        // Lock before we attempt to access shared-memory
        std::unique_lock<std::mutex> lock(_masterLock);

        // Actually dispatch the different request items
        for (const auto& nodeState : _masterNodes)
            _masterThreadPool->enqueue(
                    std::make_shared<std::string>(nodeState.first));
    }

    // Wait until the queue is empty before allowing the
    // system to re-query for status again
    while (!_masterThreadPool->isQueueEmpty())
        std::this_thread::sleep_for(std::chrono::seconds(1));

    // Handle updating the left-nodes vecter by removing expired nodes
    // Do this in a separate context to leverage RAII for the mutex/lock
    {

        // Lock before we attempt to access shared-memory
        std::unique_lock<std::mutex> lock(_masterLock);

        // Remove any master nodes from the left list if they
        // have been in that list sufficiently long
        std::vector<std::string> timedOutNodes;
        for (const auto& leftNode : _leftMasterNodes)
        {

            // Remove the node if they are sufficiently old enough
            if (_leftMasterNodeTimes[leftNode] > _leftTimeout)
                timedOutNodes.push_back(leftNode);

            // Increment the node's counter information regardless
            _leftMasterNodeTimes[leftNode] += 5;
        }

        // At this point we have marked the nodes which can be
        // removed from the left-nodes list, so remove them here
        for (const auto& expiredNode : timedOutNodes)
        {

            // Attempt to find the node in the vector
            auto foundNode = std::find(_leftMasterNodes.begin(),
                    _leftMasterNodes.end(), expiredNode);

            // Remove the node if we were able to find it
            if(foundNode != _leftMasterNodes.end())
            {
                _leftMasterNodes.erase(foundNode);
                _leftMasterNodeTimes.erase(expiredNode);
            }
        }
    }

    // Once the queue has emptied-out, we'll try to join any other
    // nodes which we discovered that doesn't yet know of this node
    if (!_masterNodesToJoin->isQueueEmpty())
    {

        // Keep track of the nodes we have already sent requests to
        std::vector<std::string> nodesRequested;

        // Get the node id and url in a thread-safe way
        // Do this in a separate context to leverage RAII for the mutex/lock
        std::string nodeId;
        std::string nodeUrl;
        {

            // Lock before we attempt to access shared-memory
            std::unique_lock<std::mutex> lock(_masterLock);

            // Actually copy over the content
            nodeId = _nodeId;
            nodeUrl = _nodeUrl;

            // Also copy-over the left-nodes vector so that we do
            // not accidentally re-add them during self healing
            for (const auto& leftNode : _leftMasterNodes)
                nodesRequested.push_back(leftNode);
        }

        // Start de-queuing items from the join list
        while (!_masterNodesToJoin->isQueueEmpty())
        {

            // Only add the node if we haven't already done so
            auto nextItem = _masterNodesToJoin->dequeue();
            if(std::find(nodesRequested.begin(),
                    nodesRequested.end(), nextItem.first) == nodesRequested.end())
            {

                // If we get here, it means we need to submit a join request
                Requests::makeRequest(
                    Servable::HttpMethod::POST, nextItem.second + "/internal/master/join",
                    {{"NodeId", nodeId}, {"NodeUrl", nodeUrl}});

                // Regardless of if we were successful or not, we'll add the
                // node to our internal state for quorum purposes
                // Do this in a separate context to leverage RAII for the mutex/lock
                {

                    // Lock before we attempt to access shared-memory
                    std::unique_lock<std::mutex> lock(_masterLock);

                    // Actually copy over the content
                    _masterNodes[nextItem.first] = MasterNodeState{false, nextItem.first, nextItem.second};
                }

                // Save this node item in our vector of already seen items
                nodesRequested.push_back(nextItem.first);
            }
        }
    }

    // Wait an additional 5 seconds so that we do not overload
    // each master node which is also handling other requests
    std::this_thread::sleep_for(std::chrono::seconds(5));
}

/**
 * Internal function used to handle the worker event loop
 */
void MasterNode::handleWorkerEventLoop()
{

    // Remove any workers from the connected worker nodes list
    // which has a last-connected value sufficiently old enough
    // Do this in a separate context to leverage RAII for the mutex/lock
    {

        // Lock before we attempt to access shared-memory
        std::unique_lock<std::mutex> lock(_masterLock);

        // Start by incrementing everyone's connection state
        // by the default loop-time (i.e. 5 seconds)
        std::vector<std::string> nodesToRemove;
        for (auto& workerItem : _connectedWorkerNodes)
        {

            // Save any old-enough workers to a removal list
            if (workerItem.second >= _workerTimeout)
                nodesToRemove.push_back(workerItem.first);

            // Increment every worker's connection value
            workerItem.second += 5;
        }

        // Remove all worker items which were found to have
        // been expired up to this point
        for (const auto& nodeToRemove : nodesToRemove)
            _connectedWorkerNodes.erase(nodeToRemove);
    }

    // Wait an additional 5 seconds to update the workers'
    // leaky connection state in-case there are interruptions
    std::this_thread::sleep_for(std::chrono::seconds(5));
}

/**
 * Internal function used to handle the asynchronous request for a
 * master-node's status to update status/quorum information
 *
 * @param nodeId String representing the node id to request
 */
void MasterNode::handleMasterNodeStatusRequest(const std::string& nodeId)
{

    // Get the node's information (i.e url, etc)
    // Do this in a separate context to leverage RAII for the mutex/lock
    std::string thisNodeId;
    unsigned long masterNodeCount = 0;
    MasterNodeState nodeState;
    {

        // Lock before we attempt to access shared-memory
        std::unique_lock<std::mutex> lock(_masterLock);

        // Get the information we need
        masterNodeCount = _masterNodes.size();
        nodeState = _masterNodes[nodeId];
        thisNodeId = _nodeId;
    }

    // Make a request on the provided node
    auto response = Requests::makeRequest(
            Servable::HttpMethod::GET, nodeState.url + "/internal/master/status", {});

    // TODO: Validate some kind of token from the other node

    // Handle the response we receive to update the node's state
    nodeState.contactable = (response.code < 300);

    // We will also try to determine if the other node we just contacted
    // is aware of our existence, and, if not, request to join the cluster
    // from their perspective
    if (response.body[thisNodeId].empty())
            _masterNodesToJoin->enqueue(std::make_pair(nodeState.id, nodeState.url));

    // Extract just the nodes in the response
    std::vector<std::string> remoteNodes;
    for (const auto& responseItem : response.body)
        if ((responseItem.second == "Connected")
                || (responseItem.second == "NotConnected"))
            remoteNodes.push_back(responseItem.first);

    // Additionally, we'll look at the response object we got back
    // and determine if any of the nodes in it are not in our list
    if (remoteNodes.size() > masterNodeCount)
    {

        // If we get here, it means it is fairly likely that the
        // node we just contacted has nodes we are not aware of
        // As a result, we'll need to at least copy our list of
        // Node IDs over so we can compare the two
        // Do this in a separate context to leverage RAII for the mutex/lock
        std::vector<std::string> ourMasterNodes;
        {

            // Lock before we attempt to access shared-memory
            std::unique_lock<std::mutex> lock(_masterLock);

            // Get the list of Node IDs we have locally
            for (const auto& masterNode : _masterNodes)
                ourMasterNodes.push_back(masterNode.first);

            // Add-in nodes which are in the left vector, since
            // we don't want to self-heal using them
            for (const auto& leftNode : _leftMasterNodes)
                ourMasterNodes.push_back(leftNode);
        }

        // Find all nodes which exist in the response object but do
        // not exist locally and add them as a node we want to join
        for (const auto& remoteNode : remoteNodes)
            if (remoteNode != thisNodeId)
                if (std::find(ourMasterNodes.begin(),
                        ourMasterNodes.end(), remoteNode) == ourMasterNodes.end())
                    _masterNodesToJoin->enqueue(std::make_pair(remoteNode, response.body["URL-" + remoteNode]));
    }

    // Update the node's state for the managing node's instance
    // Do this in a separate context to leverage RAII for the mutex/lock
    {

        // Lock before we attempt to access shared-memory
        std::unique_lock<std::mutex> lock(_masterLock);

        // Update the managing node's state for this node
        _masterNodes[nodeId] = nodeState;
    }
}

/**
 * Internal handler function used to get the internal status of the master node
 *
 * @param headers Unordered-map of Strings representing header values
 * @param body Unordered-map of Strings representing request body values
 * @param routeArg String representing additional routing arguments
 * @return ResponseObj representing the response to the request handled
 */
MasterNode::ResponseObj MasterNode::handlerGetInternalMasterStatus(
    std::unordered_map<std::string, std::string>& headers,
    std::unordered_map<std::string, std::string>& body, const std::string& routeArg)
{

    // Create a return object
    ResponseObj retObj;

    // Construct the response with information on quorum status
    // TODO: Integrate some kind of token to validate identity with
    retObj = handlerGetClusterStatus(headers, body, routeArg);

    // Add-in the details of the URLs for each of the nodes we know about
    // Do this in a separate context to leverage RAII for the mutex/lock
    if (retObj.code < 300)
    {

        // Lock before we attempt to access shared-memory
        std::unique_lock<std::mutex> lock(_masterLock);

        // Actually add the url items from our local list
        for (const auto& masterNode : _masterNodes)
            retObj.body["URL-" + masterNode.first] = masterNode.second.url;
    }

    // If we were supplied a route argument, that means we had status requested
    // to us by a worker node, so we should add it to our internal list
    // Do this in a separate context to leverage RAII for the mutex/lock
    if (!routeArg.empty() && (retObj.code < 300))
    {

        // Lock before we attempt to access shared-memory
        std::unique_lock<std::mutex> lock(_masterLock);

        // Simply set the node's last connected state to zero
        _connectedWorkerNodes[routeArg] = 0;
    }

    // Return the response object
    return retObj;
}

/**
 * Internal handler function used to post a join request for another master node
 *
 * @param headers Unordered-map of Strings representing header values
 * @param body Unordered-map of Strings representing request body values
 * @param routeArg String representing additional routing arguments
 * @return ResponseObj representing the response to the request handled
 */
MasterNode::ResponseObj MasterNode::handlerPostInternalMasterJoin(
    std::unordered_map<std::string, std::string>& headers,
    std::unordered_map<std::string, std::string>& body, const std::string& routeArg)
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
            std::unique_lock<std::mutex> lock(_masterLock);

            // Attempt to see if any node with the same id or url exists already
            for (const auto& nodeState : _masterNodes)
            {

                // Determine if the id matches an existing node
                if (nodeState.first == nodeId)
                    alreadyExistedId = true;

                // Determine if the url matches an existing node
                if (nodeState.second.url == nodeUrl)
                    alreadyExistedUrl = true;
            }

            // Also mark the node as found if it contains any of our own data
            if (nodeId == _nodeId)
                alreadyExistedId = true;
            if (nodeUrl == _nodeUrl)
                alreadyExistedUrl = true;

            // If the node we are going to add is in our leave-vector, we must be sure
            // to remove it here so that it is included in self-healing processes
            if (!alreadyExistedId && !alreadyExistedUrl)
            {

                // Find the item in the leave vector
                auto foundItem = std::find(_leftMasterNodes.begin(), _leftMasterNodes.end(), nodeId);

                // Remove the item if found
                if (foundItem != _leftMasterNodes.end())
                {
                    _leftMasterNodes.erase(foundItem);
                    _leftMasterNodeTimes.erase(nodeId);
                }
            }

            // Only proceed to add the item if it doesn't already exist
            if (!alreadyExistedId && !alreadyExistedUrl)
                _masterNodes[nodeId] = MasterNodeState{false, nodeId, nodeUrl};
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
 * Internal handler function used to post a leave request for another master node
 *
 * @param headers Unordered-map of Strings representing header values
 * @param body Unordered-map of Strings representing request body values
 * @param routeArg String representing additional routing arguments
 * @return ResponseObj representing the response to the request handled
 */
MasterNode::ResponseObj MasterNode::handlerPostInternalMasterLeave(
    std::unordered_map<std::string, std::string>& headers,
    std::unordered_map<std::string, std::string>& body, const std::string& routeArg)
{

    // Create a return object
    ResponseObj retObj;

    // Extract the new node details from the supplied body
    // TODO: Add some kind of token for later authentication purposes
    auto nodeId = body["NodeId"];

    // Construct a response object if an item is missing
    if (nodeId.empty())
        retObj = ResponseObj{400, {{"MissingArgument", "NodeId"}}};

    // Only continue if the required arguments have been supplied
    if (!nodeId.empty())
    {

        // Look for and remove the provided node object if it exists
        // Do this in a separate context to leverage RAII for the mutex/lock
        bool foundExistingNodeId = false;
        {

            // Lock before we attempt to access shared-memory
            std::unique_lock<std::mutex> lock(_masterLock);

            // Attempt to see if the node exists or not
            if (_masterNodes[nodeId].id == nodeId)
                foundExistingNodeId = true;

            // If we found the existing node, we'll add it to the "leaving" vector
            // so that self-healing doesn't try to "revive" the left node
            if (foundExistingNodeId)
            {
                _leftMasterNodes.push_back(nodeId);
                _leftMasterNodeTimes[nodeId] = 0;
            }

            // Only proceed to remove the item if it already exists
            if (foundExistingNodeId)
                _masterNodes.erase(nodeId);
        }

        // Construct a response based on whether we removed the node or not
        if (!foundExistingNodeId)
            retObj = ResponseObj{400, {{"RemovedNode", "False"},
                                       {"NodeId", nodeId},
                                       {"Message", "No node exists with the provided ID"}}};
        else
            retObj = ResponseObj{202, {{"RemovedNode", "True"},
                                       {"NodeId", nodeId},
                                       {"Message", "The node will be removed from the cluster"}}};
    }

    // Return the response object
    return retObj;
}

/**
 * Internal handler function used to get the status of the cluster externally
 *
 * @param headers Unordered-map of Strings representing header values
 * @param body Unordered-map of Strings representing request body values
 * @param routeArg String representing additional routing arguments
 * @return ResponseObj representing the response to the request handled
 */
MasterNode::ResponseObj MasterNode::handlerGetClusterStatus(
    std::unordered_map<std::string, std::string>& headers,
    std::unordered_map<std::string, std::string>& body, const std::string& routeArg)
{

    // Create a return object
    ResponseObj retObj;

    // Get the node connection information from the internal state
    // Do this in a separate context to leverage RAII for the mutex/lock
    std::string nodeId;
    std::unordered_map<std::string, std::string> connectionStatus;
    {

        // Lock before we attempt to access shared-memory
        std::unique_lock<std::mutex> lock(_masterLock);

        // Extract the connection information for each node
        for (const auto& nodeState : _masterNodes)
            connectionStatus[nodeState.first] = (
                nodeState.second.contactable ? "Connected" : "NotConnected");

        // Also get the id of this node instance
        nodeId = _nodeId;
    }

    // Determine the number of nodes which are connected
    int connectedNodes = 0;
    for (const auto& nodeItem : connectionStatus)
        if (nodeItem.second == "Connected")
            connectedNodes++;

    // Add-in our own self instance into the cluster information into
    // the unordered map which we are effectively now using as the
    // response object as the body
    connectionStatus[nodeId] = "SelfInstance";
    connectedNodes++;

    // Add-in whether quorum is met into the unordered map which
    // we are effectively now using as the response object
    auto quorumMet = (connectedNodes > (0.5 * connectionStatus.size()));
    connectionStatus["QuorumMet"] = (quorumMet ? "True" : "False");

    // Add in the details of the cluster size as well into the
    // unordered map which we are effectively now using as the
    // response object
    connectionStatus["ClusterSize"] = (
        std::to_string(connectedNodes) + "/" + std::to_string(connectionStatus.size() - 1));

    // Construct a response based on the node information
    // and the build-up unordered map for connection info
    retObj = ResponseObj{200, connectionStatus};

    // Return the response object
    return retObj;
}

/**
 * Destructor used to cleanup the instance
 */
MasterNode::~MasterNode()
{

    // Stop the event loops
    _masterEventLoop = nullptr;
    _workerEventLoop = nullptr;

    // Stop the background thread pool
    _masterThreadPool->flushQueue();
    _masterThreadPool = nullptr;

    // Stop all nodes-to-join queing and flush
    _masterNodesToJoin->flushQueue();

    // Lock before we attempt to destroy shared-memory
    std::unique_lock<std::mutex> lock(_masterLock);
}
