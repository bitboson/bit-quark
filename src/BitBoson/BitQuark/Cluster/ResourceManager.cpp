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

#include <cstdlib>
#include <BitBoson/BitQuark/Networking/Requests.h>
#include <BitBoson/BitQuark/Cluster/ResourceManager.h>

using namespace BitBoson;
using namespace BitBoson::BitQuark;

/**
 * Constructor used to setup a new request instance for voting
 *
 * @param resourceManagerId String representing the resource manager id
 * @param operation Operation representing the operation to perform
 * @param resourceGroup String representing the resource group in question
 * @param quorum Long representing the quorum needed for passage
 */
ResourceManager::ResourceRequest::ResourceRequest(
        const std::string& resourceManagerId, Operation operation,
        const std::string& resourceGroup, long quorum)
{

    // Setup the member variables
    _quorum = quorum;
    _operationInQuestion = operation;
    _resourceManagerId = resourceManagerId;
    _resourceGroupInQuestion = resourceGroup;

    // Setup the age with variance to further prevent dead-locks
    // between the various manager instances
    _age = rand() % 10;
}

/**
 * Function used to increment the request's current age
 */
void ResourceManager::ResourceRequest::incrementAge()
{

    // Increment the request's age member variable
    _age++;
}

/**
 * Function used to get the operation for the request
 *
 * @return Operation representing the operation for the request
 */
ResourceManager::ResourceRequest::Operation
ResourceManager::ResourceRequest::getOperation() const
{

    // Simply return the member operation
    return _operationInQuestion;
}

/**
 * Function used to get the resource group for the request
 *
 * @return String representing the  resource group  for the request
 */
std::string ResourceManager::ResourceRequest::getResourceGroup() const
{

    // Simply return the member resource group
    return _resourceGroupInQuestion;
}

/**
 * Function used to vote for the resource request in question
 *
 * @param resourceManagerId String representing the voting manager id
 * @param vote Vote representing the vote on the matter in question
 * @return Boolean representing whether the vote was registered or not
 */
bool ResourceManager::ResourceRequest::vote(
        const std::string& resourceManagerId, Vote vote)
{

    // Setup a return flag
    bool retFlag = false;

    // Only continue if the specified manager id has not voted yes
    if (std::find(_yayVotes.begin(), _yayVotes.end(),
                  resourceManagerId) == _yayVotes.end()
            && std::find(_nayVotes.begin(), _nayVotes.end(),
                         resourceManagerId) == _nayVotes.end())
    {

        // If we get here, it means that the manager id has not voted
        // yet (either yay or nay), and so we can accept their vote
        if (vote == Vote::YAY)
            _yayVotes.push_back(resourceManagerId);
        else if (vote == Vote::NAY)
            _nayVotes.push_back(resourceManagerId);

        // Mark the operation as successful
        retFlag = true;
    }

    // Return the return flag
    return retFlag;
}

/**
 * Function used to get whether the operation passes or not
 *
 * @param quorum Integer representing the quorum to require to pass
 * @param passRate Float representing the required percentage to pass
 * @return Boolean indicating whether the proposal/request passed
 */
bool ResourceManager::ResourceRequest::didRequestPass(float passRate) const
{

    // Create a return flag
    bool retFlag = false;

    // Determine if the operation passes based on the pass-rate
    if ((_nayVotes.size() == 0)
            || ((1.0f * (_yayVotes.size()) / (1.0f * _nayVotes.size())) > passRate))
        retFlag = true;

    // Return the return flag
    return retFlag;
}

/**
 * Function used to get whether the request has met quorum
 *
 * @return Boolean indicating whether quorum was met or not
 */
bool ResourceManager::ResourceRequest::hasMetQuorum() const
{

    // Return whether quorum has been met or not
    return ((_yayVotes.size() + _nayVotes.size()) >= ((unsigned long) _quorum));
}

/**
 * Function used to get the request's current age
 *
 * @return Long representing the request's age
 */
long ResourceManager::ResourceRequest::getRequestAge() const
{

    // Return the request's age member variable
    return _age;
}

/**
 * Constructor used to setup the voting history instance
 *
 * @param resourceManagerId String representing the resource manager
 * @param resourceGroup String representing the resource group
 * @param voteValue Vote representing the vote for the resource group
 */
ResourceManager::VotingHistory::VotingHistory(
        const std::string& resourceManagerId,
        const std::string& resourceGroup, Vote voteValue,
        long voteExpirationTimeout)
{

    // Setup the member variables accordingly
    _voteValue = voteValue;
    _voteExpirationTimeout = voteExpirationTimeout;
    _resourceGroup = resourceGroup;
    _resourceManagerId = resourceManagerId;
}

/**
 * Function used to get the resource manager id
 *
 * @return String representing the resource manager id
 */
std::string ResourceManager::VotingHistory::getResourceManagerId() const
{

    // Return the corresponding member variable
    return _resourceManagerId;
}

/**
 * Function used to get the resource group
 *
 * @return String representing the resource group
 */
std::string ResourceManager::VotingHistory::getResourceGroup() const
{

    // Return the corresponding member variable
    return _resourceGroup;
}

/**
 * Function used to get the resource vote value
 *
 * @return Vote representing the resource vote value
 */
ResourceManager::VotingHistory::Vote
ResourceManager::VotingHistory::getVoteValue() const
{

    // Return the corresponding member variable
    return _voteValue;
}

/**
 * Function used to decrement the timeout for the vote
 */
void ResourceManager::VotingHistory::decrementVoteTime()
{

    // Decrement the voting history value (if applicable)
    if (_voteExpirationTimeout > 0)
        _voteExpirationTimeout--;
}

/**
 * Function used to get whether the vote has expired or not
 *
 * @return Boolean indicating whether the vote has expired
 */
bool ResourceManager::VotingHistory::isVoteExpired() const
{

    // Return whether the vote has expired or not
    return (_voteExpirationTimeout <= 0);
}

/**
 * Function used to add a resource group to the managed resources
 *
 * @param resourceGroup String representing the resource group to add
 * @param resource Resource (pointer) representing the actual resource
 * @return Boolean indcating whether the resource group was added or not
 */
bool ResourceManager::ManagedResources::addResourceGroup(
        const std::string& resourceGroup, std::shared_ptr<Resource> resource)
{

    // Create a return flag
    bool retFlag = false;

    // Only continue if the resource hasn't already been added
    if (_resources.find(resourceGroup) == _resources.end())
    {

        // Update the total managed resources cost
        auto resourceCost = resource->getResourceCost();
        setResourceSize(getResourceSize() + resourceCost.getResourceSize());
        setMemoryRequirements(getMemoryRequirements() + resourceCost.getMemoryRequirements());
        setResourceThreads(getResourceThreads() + resourceCost.getResourceThreads());

        // Add the resource to the managed ones
        _resources[resourceGroup] = resource;

        // Mark the return flag as true
        retFlag = true;
    }

    // Return the return flag
    return retFlag;
}

/**
 * Function used to remove a resource group from the managed resources
 *
 * @param resourceGroup String representing the resource group to remove
 * @return Boolean indcating whether the resource group was removed or not
 */
bool ResourceManager::ManagedResources::removeResourceGroup(
        const std::string& resourceGroup)
{

    // Create a return flag
    bool retFlag = false;

    // Only continue if the resource has already been added
    auto resourceIterator = _resources.find(resourceGroup);
    if (resourceIterator != _resources.end())
    {

        // Extract the reference to the resource
        auto resource = resourceIterator->second;

        // Update the total managed resources cost
        auto resourceCost = resource->getResourceCost();
        setResourceSize(getResourceSize() - resourceCost.getResourceSize());
        setMemoryRequirements(getMemoryRequirements() - resourceCost.getMemoryRequirements());
        setResourceThreads(getResourceThreads() - resourceCost.getResourceThreads());

        // Remove the resource from the managed ones
        _resources.erase(resourceIterator);

        // Mark the return flag as true
        retFlag = true;
    }

    // Return the return flag
    return retFlag;
}

/**
 * Function used to get/return the unordered-map of managed resource groups
 *
 * @return Unordered-Map of String-Resources representing the managed resource groups
 */
std::unordered_map<std::string, std::shared_ptr<Resource>>
ResourceManager::ManagedResources::getManagedResourceGroups()
{

    // Simply return the unordered-map of managed resource groups
    return _resources;
}

/**
 * Constructor used to setup the servable and resource management
 * for a master node (resource manager) in the cluster
 *
 * @param hostname String representing the hostname for the server
 * @param port Integer representing the port to serve/listen on
 * @param credentials S3Credentials to setup the instance on/using
 */
ResourceManager::ResourceManager(const std::string& hostname, int port,
        std::shared_ptr<S3Credentials> credentials) : MasterNode(hostname, port)
{

    // Setup the default member values
    _ageTimeout = 30;

    // Setup access to the global state using the provided credentials
    _globalState = std::make_shared<GlobalState>(credentials);

    // Setup the local master state on its built-in temporary directory
    _masterState = std::make_shared<MasterState>();

    // Setup the thread-pools for making requests to other components
    _claimResourceRequests = std::make_shared<StandardModel::ThreadPool<std::string>>(
        [this](std::shared_ptr<std::string> value) {
            handleClaimResourceRequest(*value);
        });

    // Setup the asynchronous event loop for resource management
    _resourceEventLoop = std::make_shared<StandardModel::AsyncEventLoop>(
        [this]() {
            handleResourceEventLoop();
        });

    // Setup the REST API Listener receive resource handler requests
    addListener(HttpMethod::POST, "/internal/master/resources", "resourceId",
        [this](std::unordered_map<std::string, std::string>& headers,
            std::unordered_map<std::string, std::string>& body,
            const std::string& routeArg) -> ResponseObj
        {
            return this->handlerGetInternalMasterResources(headers, body, routeArg);
        });
}

/**
 * Function used to set the maximum request age timeout for new resources
 *
 * @param ageTimeout Long representing the request age timeout to use
 * @return Boolean indcating whether the new value was accepted or not
 */
bool ResourceManager::setRequestAgeTimeout(long ageTimeout)
{

    // Create a return flag
    bool retFlag = false;

    // Only continue if the value is a valid one
    // Do this in a separate context to leverage RAII for the mutex/lock
    if ((ageTimeout > 10) && (ageTimeout < 180))
    {

        // Lock before we attempt to destroy shared-memory
        std::unique_lock<std::mutex> lock(_lock);

        // Set the value accordingly
        // NOTE: We add in an extra 5 seconds due to randomized ages
        //       in the pending request items themselves
        _ageTimeout = ageTimeout + 5;

        // Mark the return flag as true
        retFlag = true;
    }

    // Return the return flag
    return retFlag;
}

/**
 * Internal function used to handle the resource event loop
 */
void ResourceManager::handleResourceEventLoop()
{

    // Remove proposals which are null, passed, or failed
    // and handle them accordingly
    // Do this in a separate context to leverage RAII for the mutex/lock
    {

        // Lock before we attempt to destroy shared-memory
        std::unique_lock<std::mutex> lock(_lock);

        // Check if any proposals have been approved for resources
        // and apply said resources (manage them) if so
        for (auto it = _pendingRequests.begin(); it != _pendingRequests.end();)
        {

            // Create a flag to track whether to remove the current item
            bool removeItem = false;

            // Extract the proposal from the iterator
            auto proposal = it->second;

            // If the item is not null, attempt to remove it
            // Otherwise, if the item is null, mark it as removable
            if (proposal == nullptr)
                removeItem = true;
            else if ((proposal->getOperation() == ResourceRequest::Operation::MANAGE)
                    && (isInQuorum()) && (proposal->hasMetQuorum())
                    && (proposal->didRequestPass())
                    && (_globalState->claimManagedResourceGroup(_nodeId,
                        proposal->getResourceGroup())))
                removeItem = true;
            else if ((proposal->getOperation() == ResourceRequest::Operation::MANAGE)
                    && (isInQuorum()) && (proposal->hasMetQuorum())
                    && (!proposal->didRequestPass()))
                removeItem = true;

            // If the request is simply too old, we'll also remove it
            if (proposal->getRequestAge() > _ageTimeout)
                removeItem = true;

            // Increment the request's age regardless
            proposal->incrementAge();

            // Remove the item if it is supposed to be removed
            // Otherwise increment the iterator accordingly
            if (removeItem)
                it = _pendingRequests.erase(it);
            else
                it++;
        }

        // Remove and expired voting history items
        for (auto it = _votedOnItems.begin(); it != _votedOnItems.end();)
        {

            // Decrement the individual vote item's countdown expiration
            it->second->decrementVoteTime();

            // If the item is actually expired, remove it here
            if (it->second->isVoteExpired())
                it = _votedOnItems.erase(it);
            else
                it++;
        }
    }

    // Determine if any resources need to be released from management
    // and release them if desired to do so
    // Do this in a separate context to leverage RAII for the mutex/lock
    {

        // Lock before we attempt to destroy shared-memory
        std::unique_lock<std::mutex> lock(_lock);

        // Actually remove the "erased" resources
        _removedResources.erase(
            std::remove_if(
                _removedResources.begin(),
                _removedResources.end(),
                [this](std::shared_ptr<ResourceRequest> itemToRemove)
                {

                    // Create a return flag
                    bool retFlag = false;

                    // If the item is not null, attempt to remove it
                    // Otherwise, if the item is null, mark it as removable
                    if (itemToRemove == nullptr)
                        retFlag = true;
                    else if (_globalState->dropManagedResourceGroup(_nodeId,
                            itemToRemove->getResourceGroup()))
                        retFlag = true;

                    // Return the return flag
                    return retFlag;
                }
            ), _removedResources.end());
    }

    // List the un-managed resource groups available and their requirements
    // Do this in a separate context to leverage RAII for the mutex/lock
    {

        // Lock before we attempt to destroy shared-memory
        std::unique_lock<std::mutex> lock(_lock);

        // Actually list and attempt to claim resources
        auto unmanagedResourceGroups = _globalState->listUnmanagedResourceGroups();
        while (unmanagedResourceGroups->hasMoreItems())
        {

            // Get the current, unmanaged resource group for reference
            auto unmanagedResourceGroupRaw = unmanagedResourceGroups->getNextItem();

            // Determine the resource usages for this particular item
            // TODO - Determine resource usage

            // Determine possible candidates to claim ownership over
            // NOTE: This can only be one resource group per iteration
            // TODO - Determine if we can handle this resource, break if so
        }

        // Submit a proposal to gain ownership over said resource
        //_claimResourceRequests->enqueue(
        //        std::make_shared<std::string>(nodeState.first));
    }

    // Wait an additional 1 second for more consistent age-based behavior
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

/**
 * Internal function used to handle the asynchronous request for a
 * resource manager to request handling/managing a resource group
 * NOTE: This manages out-going resource request from this manager
 *
 * @param resourceGroup String representing the resource group
 */
void ResourceManager::handleClaimResourceRequest(const std::string& resourceGroup)
{

    // Determine if this resource group is not currently pending and is not in
    // the voter history
    // Do this in a separate context to leverage RAII for the mutex/lock
    bool hasLocalHistory = false;
    {

        // Lock before we attempt to destroy shared-memory
        std::unique_lock<std::mutex> lock(_lock);

        // Actually perform the check itself and setup the results
        if (_pendingRequests.find(resourceGroup) != _pendingRequests.end()
                || ((_votedOnItems.find(resourceGroup) != _votedOnItems.end()
                        && (_votedOnItems[resourceGroup]->getVoteValue() == VotingHistory::Vote::YAY))))
            hasLocalHistory = true;
    }


    // Only continue if this request is not currently pending and/or is
    // not in the voter history as well as a resource trying to be claimed
    if (hasLocalHistory)
    {

        // Get a list of all connected master nodes
        auto connectedNodes = getConnectedMasters();

        // Create a new pending item for the resource request
        // Do this in a separate context to leverage RAII for the mutex/lock
        {

            // Lock before we attempt to destroy shared-memory
            std::unique_lock<std::mutex> lock(_lock);

            // Perform the actual resource request creation
            _pendingRequests[resourceGroup] = std::make_shared<ResourceRequest>(
                    _nodeId, ResourceRequest::Operation::MANAGE, resourceGroup,
                    (((int)(connectedNodes.size() / 2) + 1)));
        }

        // Make a claim (REST API request) for the desired resource
        // to all other, known/connected resource managers
        for (const auto& masterNode : connectedNodes)
        {

            // Only continue if we are not making a request to ourselves
            if (masterNode != _nodeId)
            {

                // Make the REST API request to the other nodes
                auto masterUrl = getUrlForConnectedMasterNode(masterNode);
                auto response = Requests::makeRequest(
                            Servable::HttpMethod::POST, masterUrl + "/internal/master/resources",
                            {{"ResourceManagerId", _nodeId},
                            {"ResourceGroup", resourceGroup},
                            {"ResourceOperation", "MANAGE"}});

                // Add successful responses to the current proposal
                // Do this in a separate context to leverage RAII for the mutex/lock
                if (response.code < 300)
                {

                    // Lock before we attempt to destroy shared-memory
                    std::unique_lock<std::mutex> lock(_lock);

                    // Determine the vote from the response
                    auto vote = ResourceRequest::Vote::NAY;
                    if (response.body["Vote"] == "YAY")
                        vote = ResourceRequest::Vote::YAY;

                    // Search for and update the current pending proposal
                    // for the affected resource group
                    if (_pendingRequests.find(resourceGroup) != _pendingRequests.end())
                        if (_pendingRequests[resourceGroup]->getResourceGroup() == resourceGroup)
                            _pendingRequests[resourceGroup]->vote(masterNode, vote);
                }
            }
        }
    }
}

/**
 * Internal handler function used to receive resource handler requests
 *
 * @param headers Unordered-map of Strings representing header values
 * @param body Unordered-map of Strings representing request body values
 * @param routeArg String representing additional routing arguments
 * @return ResponseObj representing the response to the request handled
 */
ResourceManager::ResponseObj ResourceManager::handlerGetInternalMasterResources(
    std::unordered_map<std::string, std::string>& headers,
    std::unordered_map<std::string, std::string>& body,
    const std::string& routeArg)
{

    // Create a return object
    ResponseObj retObj;

    // Extract the items from the request body
    // TODO: Add some kind of token for later authentication purposes
    auto managerId = body["ResourceManagerId"];
    auto resourceGroup = body["ResourceGroup"];
    auto operation = body["ResourceOperation"];

    // Only vote "yay" for this proposal if we are not pending the same request
    // Do this in a separate context to leverage RAII for the mutex/lock
    {

        // Lock before we attempt to destroy shared-memory
        std::unique_lock<std::mutex> lock(_lock);

        // Check if we are going to vote "yay" or "nay" on the proposal
        // and setup the return object accordingly
        if ((_pendingRequests.find(resourceGroup) == _pendingRequests.end())
                && (_votedOnItems.find(resourceGroup) == _votedOnItems.end()))
            retObj = ResponseObj{200, {{"Vote", "YAY"},
                                       {"ResourceManagerId", managerId},
                                       {"ResourceGroup", resourceGroup},
                                       {"ResourceOperation", operation}}};
        else if ((_pendingRequests.find(resourceGroup) == _pendingRequests.end())
                && (_votedOnItems.find(resourceGroup) != _votedOnItems.end())
                && (_votedOnItems[resourceGroup]->getVoteValue() == VotingHistory::Vote::YAY)
                && (_votedOnItems[resourceGroup]->getResourceManagerId() == managerId))
            retObj = ResponseObj{200, {{"Vote", "YAY"},
                                       {"ResourceManagerId", managerId},
                                       {"ResourceGroup", resourceGroup},
                                       {"ResourceOperation", operation}}};
        else
            retObj = ResponseObj{202, {{"Vote", "NAY"},
                                       {"ResourceManagerId", managerId},
                                       {"ResourceGroup", resourceGroup},
                                       {"ResourceOperation", operation}}};

        // If there is no voting history, and this node actually voted in
        // favor of another node, we need to keep track of this, otherwise
        // we do not really need to keep track of this
        if (retObj.body["VOTE"] == "YAY")
            if (_votedOnItems.find(resourceGroup) == _votedOnItems.end())
                _votedOnItems[resourceGroup] = std::make_shared<VotingHistory>(
                        managerId, resourceGroup, VotingHistory::Vote::YAY,
                        (_ageTimeout + (rand() % ((int)(_ageTimeout / 2)))));
    }

    // Return the response object
    return retObj;
}

/**
 * Destructor used to cleanup the instance
 */
ResourceManager::~ResourceManager()
{

    // Stop the event loop
    _resourceEventLoop = nullptr;

    // Stop the background thread pool
    _claimResourceRequests->flushQueue();
    _claimResourceRequests = nullptr;

    // Lock before we attempt to destroy shared-memory
    std::unique_lock<std::mutex> lock(_lock);
}
