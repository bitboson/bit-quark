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

#ifndef BITQUARK_RESOURCEMANAGER_H
#define BITQUARK_RESOURCEMANAGER_H

#include <memory>
#include <string>
#include <BitBoson/StandardModel/Threading/ThreadPool.hpp>
#include <BitBoson/StandardModel/Threading/AsyncEventLoop.hpp>
#include <BitBoson/BitQuark/Storage/S3Credentials.h>
#include <BitBoson/BitQuark/Cluster/State/Resource.h>
#include <BitBoson/BitQuark/Cluster/State/GlobalState.h>
#include <BitBoson/BitQuark/Cluster/State/MasterState.h>
#include <BitBoson/BitQuark/Cluster/Components/MasterNode.h>

using namespace BitBoson;
namespace BitBoson::BitQuark
{

    class ResourceManager : public MasterNode
    {

        // Private member classes
        private:

            class ResourceRequest
            {

                // Public Enumerations
                public:

                    // Enumeration representing an operation
                    enum Operation
                    {
                        MANAGE,
                        UNMANAGE
                    };

                    // Enumeration representing a vote
                    enum Vote
                    {
                        YAY,
                        NAY
                    };

                // Private member variables
                private:
                    long _age;
                    long _quorum;
                    Operation _operationInQuestion;
                    std::string _resourceManagerId;
                    std::string _resourceGroupInQuestion;
                    std::vector<std::string> _yayVotes;
                    std::vector<std::string> _nayVotes;

                // Public member functions
                public:

                    /**
                     * Constructor used to setup a new request instance for voting
                     *
                     * @param resourceManagerId String representing the resource manager id
                     * @param operation Operation representing the operation to perform
                     * @param resourceGroup String representing the resource group in question
                     * @param quorum Long representing the quorum needed for passage
                     */
                    ResourceRequest(const std::string& resourceManagerId,
                        Operation operation, const std::string& resourceGroup,
                        long quorum);

                    /**
                     * Function used to increment the request's current age
                     */
                    void incrementAge();

                    /**
                     * Function used to get the operation for the request
                     *
                     * @return Operation representing the operation for the request
                     */
                    Operation getOperation() const;

                    /**
                     * Function used to get the resource group for the request
                     *
                     * @return String representing the  resource group  for the request
                     */
                    std::string getResourceGroup() const;

                    /**
                     * Function used to vote for the resource request in question
                     *
                     * @param resourceManagerId String representing the voting manager id
                     * @param vote Vote representing the vote on the matter in question
                     * @return Boolean representing whether the vote was registered or not
                     */
                    bool vote(const std::string& resourceManagerId, Vote vote);

                    /**
                     * Function used to get whether the operation passes or not
                     *
                     * @param passRate Float representing the required percentage to pass
                     * @return Boolean indicating whether the proposal/request passed
                     */
                    bool didRequestPass(float passRate=0.5) const;

                    /**
                     * Function used to get whether the request has met quorum
                     *
                     * @return Boolean indicating whether quorum was met or not
                     */
                    bool hasMetQuorum() const;

                    /**
                     * Function used to get the request's current age
                     *
                     * @return Long representing the request's age
                     */
                    long getRequestAge() const;

                    /**
                     * Destructor used to cleanup the instance
                     */
                    virtual ~ResourceRequest() = default;
            };

            class VotingHistory
            {

                // Public enumerations
                public:

                    // Enumeration representing a vote
                    enum Vote
                    {
                        YAY,
                        NAY
                    };

                // Private member variables
                private:
                    Vote _voteValue;
                    long _voteExpirationTimeout;
                    std::string _resourceManagerId;
                    std::string _resourceGroup;

                // Public member functions
                public:

                    /**
                     * Constructor used to setup the voting history instance
                     *
                     * @param resourceManagerId String representing the resource manager
                     * @param resourceGroup String representing the resource group
                     * @param voteValue Vote representing the vote for the resource group
                     */
                    VotingHistory(const std::string& resourceManagerId,
                            const std::string& resourceGroup, Vote voteValue,
                            long voteExpirationTimeout);

                    /**
                     * Function used to get the resource manager id
                     *
                     * @return String representing the resource manager id
                     */
                    std::string getResourceManagerId() const;

                    /**
                     * Function used to get the resource group
                     *
                     * @return String representing the resource group
                     */
                    std::string getResourceGroup() const;

                    /**
                     * Function used to get the resource vote value
                     *
                     * @return Vote representing the resource vote value
                     */
                    Vote getVoteValue() const;

                    /**
                     * Function used to decrement the timeout for the vote
                     */
                    void decrementVoteTime();

                    /**
                     * Function used to get whether the vote has expired or not
                     *
                     * @return Boolean indicating whether the vote has expired
                     */
                    bool isVoteExpired() const;

                    /**
                     * Destructor used to cleanup the instance
                     */
                    virtual ~VotingHistory() = default;
            };

            class ManagedResources : public Resource::ResourceCost
            {

                // Private member variables
                private:
                    std::unordered_map<std::string, std::shared_ptr<Resource>> _resources;

                // Public member functions
                public:

                    /**
                     * Constructor used to setup the instance
                     */
                    ManagedResources() = default;

                    /**
                     * Function used to add a resource group to the managed resources
                     *
                     * @param resourceGroup String representing the resource group to add
                     * @param resource Resource (pointer) representing the actual resource
                     * @return Boolean indcating whether the resource group was added or not
                     */
                    bool addResourceGroup(const std::string& resourceGroup, std::shared_ptr<Resource> resource);

                    /**
                     * Function used to remove a resource group from the managed resources
                     *
                     * @param resourceGroup String representing the resource group to remove
                     * @return Boolean indcating whether the resource group was removed or not
                     */
                    bool removeResourceGroup(const std::string& resourceGroup);

                    /**
                     * Function used to get/return the unordered-map of managed resource groups
                     *
                     * @return Unordered-Map of String-Resources representing the managed resource groups
                     */
                    std::unordered_map<std::string, std::shared_ptr<Resource>> getManagedResourceGroups();

                    /**
                     * Destructor used to cleanup the instance
                     */
                    virtual ~ManagedResources() = default;
            };

        // Private member variables
        private:
            long _ageTimeout;
            std::mutex _lock;
            std::string _nodeId;
            std::shared_ptr<GlobalState> _globalState;
            std::shared_ptr<MasterState> _masterState;
            std::shared_ptr<StandardModel::AsyncEventLoop> _resourceEventLoop;
            std::shared_ptr<ManagedResources> _currentResources;
            std::vector<std::shared_ptr<ResourceRequest>> _removedResources;
            std::shared_ptr<StandardModel::ThreadPool<std::string>> _claimResourceRequests;
            std::unordered_map<std::string, std::shared_ptr<VotingHistory>> _votedOnItems;
            std::unordered_map<std::string, std::shared_ptr<ResourceRequest>> _pendingRequests;

        // Public member functions
        public:

            /**
             * Constructor used to setup the servable and resource management
             * for a master node (resource manager) in the cluster
             *
             * @param hostname String representing the hostname for the server
             * @param port Integer representing the port to serve/listen on
             * @param credentials S3Credentials to setup the instance on/using
             */
            ResourceManager(const std::string& hostname, int port,
                    std::shared_ptr<S3Credentials> credentials);

            /**
             * Function used to set the maximum request age timeout for new resources
             *
             * @param ageTimeout Long representing the request age timeout to use
             * @return Boolean indcating whether the new value was accepted or not
             */
            bool setRequestAgeTimeout(long ageTimeout);

            /**
             * Destructor used to cleanup the instane
             */
            virtual ~ResourceManager();

        // Private member variables
        private:

            /**
             * Internal function used to handle the resource event loop
             */
            void handleResourceEventLoop();

            /**
             * Internal function used to handle the asynchronous request for a
             * resource manager to request handling/managing a resource group
             * NOTE: This manages out-going resource request from this manager
             *
             * @param resourceGroup String representing the resource group
             */
            void handleClaimResourceRequest(const std::string& resourceGroup);

            /**
             * Internal handler function used to receive resource handler requests
             *
             * @param headers Unordered-map of Strings representing header values
             * @param body Unordered-map of Strings representing request body values
             * @param routeArg String representing additional routing arguments
             * @return ResponseObj representing the response to the request handled
             */
            ResponseObj handlerGetInternalMasterResources(
                std::unordered_map<std::string, std::string>& headers,
                std::unordered_map<std::string, std::string>& body,
                const std::string& routeArg);
    };
}

#endif //BITQUARK_RESOURCEMANAGER_H