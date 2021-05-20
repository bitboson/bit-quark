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

#ifndef BITQUARK_GLOBALSTATE_H
#define BITQUARK_GLOBALSTATE_H

#include <memory>
#include <BitBoson/StandardModel/Primitives/Generator.hpp>
#include <BitBoson/BitQuark/Storage/S3DataStore.h>
#include <BitBoson/BitQuark/Storage/S3Credentials.h>
#include <BitBoson/BitQuark/Cluster/State/Resource.h>

using namespace BitBoson;
namespace BitBoson::BitQuark
{

    class GlobalState
    {

        // Public member enumerations
        public:
            enum Mode {
                READ_ONLY,
                READ_WRITE
            };

        // Private member classes
        private:

            class SimpleResourceWrapper : public Resource
            {

                // Private member variables
                private:
                    ResourceCost _resourceCost;
                    std::string _originalFileString;

                // Public member functions
                public:

                    /**
                     * Constructor used to setup the instance
                     */
                    SimpleResourceWrapper() = default;

                    /**
                     * Constructor used to setup the instance with a reference value
                     *
                     * @param resource Resource representing the reference value to use
                     */
                    explicit SimpleResourceWrapper(std::shared_ptr<Resource> resource);

                    /**
                     * Constructor used to setup the instance with a file-string
                     *
                     * @param fileString String representing the file-string to set
                     */
                    explicit SimpleResourceWrapper(const std::string& fileString);

                    /**
                     * Function used to get the original resource's file-string
                     *
                     * @return String representing the original resource file-string
                     */
                    std::string getOriginalResourceFileString() const;

                    /**
                     * Overridden function used to get the resource cost for the resource instance
                     *
                     * @return ResourceCost object representing the instance's resource cost
                     */
                    ResourceCost getResourceCost() const override;

                    /**
                     * Overridden function used to get the packed-vector for the resource instance
                     *
                     * @return Vector of Strings representing the instance's packed vector
                     */
                    std::vector<std::string> getPackedVector() const override;

                    /**
                     * Overridden function used to set the packed-vector for the resource instance
                     *
                     * @param packedVect Vector of Strings representing the instance's packed vector
                     */
                    bool setPackedVector(const std::vector<std::string>& packedVect) override;

                    /**
                     * Destructor used to cleanup the instance
                     */
                    virtual ~SimpleResourceWrapper() = default;
            };

        // Private member variables
        private:
            Mode _accessMode;
            std::shared_ptr<S3DataStore> _dataStore;

        // Public member functions
        public:

            /**
             * Constructor used to setup the instance on a bucket
             *
             * @param credentials S3Credentials to setup the instance on/using
             * @param mode Mode enumeration representing the access mode being used
             */
            explicit GlobalState(std::shared_ptr<S3Credentials> credentials,
                    Mode mode = Mode::READ_ONLY);

            /**
             * Function used to claim a resource group with a provided
             * resource-manager Id
             *
             * @param resourceManagerId String representing the manager's Id
             * @param groupId String representing the group Id to claim/manage
             * @return Boolean indicating whether the group was claimed or not
             */
            bool claimManagedResourceGroup(const std::string& resourceManagerId,
                    const std::string& groupId);

            /**
             * Function used to drop a resource group from the provided
             * resource-manager Id
             *
             * @param resourceManagerId String representing the manager's Id
             * @param groupId String representing the group Id to drop
             * @return Boolean indicating whether the group was dropped or not
             */
            bool dropManagedResourceGroup(const std::string& resourceManagerId,
                    const std::string& groupId);

            /**
             * Function used to list the managed resoure groups for the
             * resource-manager Id
             *
             * @param resourceManagerId String representing the manager Id
             * @return Generator of Strings representing the resource groups
             */
            std::shared_ptr<StandardModel::Generator<std::string>> listManagedResourceGroups(
                    const std::string& resourceManagerId) const;

            /**
             * Function used to list the unmanaged resoure groups for the instance
             *
             * @return Generator of Strings representing the resource groups
             */
            std::shared_ptr<StandardModel::Generator<std::string>> listUnmanagedResourceGroups() const;

            /**
             * Function used to add a resource group to the global state
             *
             * @param groupId String representing the group Id to add
             * @return Boolean indicating whether the group was added or not
             */
            bool addResourceGroup(const std::string& groupId);

            /**
             * Function used to remove a resource group to the global state
             *
             * @param groupId String representing the group Id to remove
             * @return Boolean indicating whether the group was removed or not
             */
            bool removeResourceGroup(const std::string& groupId);

            /**
             * Function used to get a resource group's cost from the global state
             *
             * @param groupId String representing the group Id to get the cost for
             * @return ResourceCost representing the cost of the entire resource group
             */
            Resource::ResourceCost getResourceGroupCost(const std::string& groupId) const;

            /**
             * Function used to list the existing resource groups in the state
             *
             * @return Generator of Strings representing the resource groups
             */
            std::shared_ptr<StandardModel::Generator<std::string>> listResourceGroups() const;

            /**
             * Function used to set/add the resource data in the resource/group pair
             *
             * @param groupId String representing the group Id to use
             * @param resourceId String representing the resource Id we are setting
             * @param resource Resource representing the data to use in the process
             * @return Boolean indicating whether the resource was added/set or not
             */
            bool setResourceInGroup(const std::string& groupId,
                    const std::string& resourceId, std::shared_ptr<Resource> resource);

            /**
             * Function used to get the resource data in the resource/group pair
             *
             * @param groupId String representing the group Id to use
             * @param resourceId String representing the resource Id we are setting
             * @return String representing the file-string used in the resource/group pair
             */
            std::string getResourceInGroup(const std::string& groupId,
                    const std::string& resourceId) const;

            /**
             * Function used to remove the resource data in the resource/group pair
             *
             * @param groupId String representing the group Id to use
             * @param resourceId String representing the resource Id we are removing
             * @return Boolean indicating whether the resource was removed or not
             */
            bool removeResourceInGroup(const std::string& groupId,
                    const std::string& resourceId);

            /**
             * Function used to get the resource's cost in the resource/group pair
             *
             * @param groupId String representing the group Id to use
             * @param resourceId String representing the resource Id we are getting
             * @return ResourceCost representing the cost of the entire resource item
             */
            Resource::ResourceCost getResourceInGroupCost(const std::string& groupId,
                    const std::string& resourceId) const;

            /**
             * Function used to list the existing resources in a resource group
             *
             * @param groupId String representing the group Id to list out
             * @return Generator of Strings representing the resources in the group
             */
            std::shared_ptr<StandardModel::Generator<std::string>> listResourcesInGroup(
                    const std::string& groupId) const;

            /**
             * Function used to clear-out the entire Global State
             *
             * @return Boolean indicating whether the operation was successful
             */
            bool clearEntireState();

            /**
             * Destructor used to cleanup the instance
             */
            virtual ~GlobalState() = default;

        // Private member functions
        private:

            /**
             * Internal function used to get a key prefixed with "ResourceGroups"
             *
             * @param groupId String representing the group Id to use
             * @return String representing the resource-group prefixed-key
             */
            std::string getResourceGroupPrefixedKey(const std::string& groupId = "") const;

            /**
             * Internal function used to get a key prefixed with "Resources"
             *
             * @param groupId String representing the group Id to use
             * @param resourceId String representing the resource Id to use
             * @return String representing the resource and group prefixed-key
             */
            std::string getResourcePrefixedKey(const std::string& groupId,
                    const std::string& resourceId = "") const;

            /**
             * Internal function used to get a key prefixed with "Assignments/Unassigned"
             *
             * @param groupId String representing the group Id to use
             * @return String representing the unassigned prefixed-key
             */
            std::string getUnassignedPrefixedKey(const std::string& groupId="") const;

            /**
             * Internal function used to get a key prefixed with "Assignments/Assigned"
             *
             * @param resourceManagerId String representing the manager Id to use
             * @param groupId String representing the group Id to use
             * @return String representing the assigned prefixed-key
             */
            std::string getAssignedPrefixedKey(const std::string& resourceManagerId,
                    const std::string& groupId="") const;
    };
}

#endif //BITQUARK_GLOBALSTATE_H