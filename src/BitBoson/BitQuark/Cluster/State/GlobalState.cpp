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

#include <BitBoson/StandardModel/Utils/Utils.h>
#include <BitBoson/BitQuark/Cluster/State/GlobalState.h>

using namespace BitBoson;
using namespace BitBoson::BitQuark;

/**
 * Constructor used to setup the instance with a reference value
 *
 * @param resource Resource representing the reference value to use
 */
GlobalState::SimpleResourceWrapper::SimpleResourceWrapper(
        std::shared_ptr<Resource> resource)
{

    // Only try to setup the values if the resource is valid
    if (resource != nullptr)
    {

        // Setup the member variables accordingly
        _resourceCost = resource->getResourceCost();
        _originalFileString = resource->getFileString();
    }
}

/**
 * Constructor used to setup the instance with a file-string
 *
 * @param fileString String representing the file-string to set
 */
GlobalState::SimpleResourceWrapper::SimpleResourceWrapper(
        const std::string& fileString)
{

    // Simply setup the packed vector for the instance
    auto parsedFileString = StandardModel::Utils::parseFileString(fileString);
    if (parsedFileString != nullptr)
        setPackedVector(parsedFileString->rawVect);
}

/**
 * Function used to get the original resource's file-string
 *
 * @return String representing the original resource file-string
 */
std::string GlobalState::SimpleResourceWrapper::getOriginalResourceFileString() const
{

    // Simply return the original resource's file-string
    return _originalFileString;
}

/**
 * Overridden function used to get the resource cost for the resource instance
 *
 * @return ResourceCost object representing the instance's resource cost
 */
Resource::ResourceCost GlobalState::SimpleResourceWrapper::getResourceCost() const
{

    // Create and return a copy of the resource cost object
    return ResourceCost(_resourceCost.getResourceSize(),
            _resourceCost.getMemoryRequirements(), _resourceCost.getResourceThreads());
}

/**
 * Overridden function used to get the packed-vector for the resource instance
 *
 * @return Vector of Strings representing the instance's packed vector
 */
std::vector<std::string> GlobalState::SimpleResourceWrapper::getPackedVector() const
{

    // Create a return vector
    std::vector<std::string> retVect;

    // Push all internal values into the packed vector
    retVect.push_back(std::to_string(_resourceCost.getResourceSize()));
    retVect.push_back(std::to_string(_resourceCost.getMemoryRequirements()));
    retVect.push_back(std::to_string(_resourceCost.getResourceThreads()));
    retVect.push_back(_originalFileString);

    // Return the return vector
    return retVect;
}

/**
 * Overridden function used to set the packed-vector for the resource instance
 *
 * @param packedVect Vector of Strings representing the instance's packed vector
 */
bool GlobalState::SimpleResourceWrapper::setPackedVector(
        const std::vector<std::string>& packedVect)
{

    // Create a return flag
    bool retFlag = false;

    // Only continue if the packed vector has enough items
    if (packedVect.size() >= 4)
    {

        // Extract the cost values from the parsed file-string
        // TODO - Include regex checking before conversions
        _resourceCost = ResourceCost();
        _resourceCost.setResourceSize(std::stol(packedVect[0]));
        _resourceCost.setMemoryRequirements(std::stol(packedVect[1]));
        _resourceCost.setResourceThreads(std::stoi(packedVect[2]));

        // Extract the original, internal resource's file-string
        _originalFileString = packedVect[3];

        // Setup the return flag as true
        retFlag = true;
    }

    // Return the return flag
    return retFlag;
}

/**
 * Constructor used to setup the instance on a bucket
 *
 * @param credentials S3Credentials to setup the instance on/using
 * @param mode Mode enumeration representing the access mode being used
 */
GlobalState::GlobalState(std::shared_ptr<S3Credentials> credentials,
        Mode mode)
{

    // Setup the instance using the provided values
    _accessMode = mode;
    _dataStore = std::make_shared<S3DataStore>(credentials);
}

/**
 * Function used to claim a resource group with a provided
 * resource-manager Id
 *
 * @param resourceManagerId String representing the manager's Id
 * @param groupId String representing the group Id to claim/manage
 * @return Boolean indicating whether the group was claimed or not
 */
bool GlobalState::claimManagedResourceGroup(
        const std::string& resourceManagerId, const std::string& groupId)
{

    // Create a return flag
    bool retFlag = false;

    // Only continue if the resource group is not already accounted for
    // Basically check if the group id is in the unassigned directory
    auto unassignedResource = getUnassignedPrefixedKey(groupId);
    if (_dataStore->getItem(unassignedResource) == "UNASSIGNED")
    {

        // If we get here, it means the resource is up-for-grabs and
        // thus is not assigned, so we'll add it to the assigned resources
        auto assignedResource = getAssignedPrefixedKey(
                resourceManagerId, groupId);
        if (_dataStore->addItem(assignedResource, "ASSIGNED"))
        {

            // Once we get here, it means we have successfully assigned
            // the resource to ourselves, so we'll delete it from the
            // unassigned area (and assign the return flag accordingly)
            retFlag = _dataStore->deleteItem(unassignedResource);
        }
    }

    // Return the return flag
    return retFlag;
}

/**
 * Function used to drop a resource group from the provided
 * resource-manager Id
 *
 * @param resourceManagerId String representing the manager's Id
 * @param groupId String representing the group Id to drop
 * @return Boolean indicating whether the group was dropped or not
 */
bool GlobalState::dropManagedResourceGroup(
        const std::string& resourceManagerId, const std::string& groupId)
{

    // Create a return flag
    bool retFlag = false;

    // In order to drop a resource, it has to be claimed by the
    // given resource manager, so only proceed if that's the case
    auto assignedResource = getAssignedPrefixedKey(
            resourceManagerId, groupId);
    if (_dataStore->getItem(assignedResource) == "ASSIGNED")
    {

        // If we get here, it means that the item is assigned, so
        // we'll add it to the unassigned area before proceeding
        auto unassignedResource = getUnassignedPrefixedKey(groupId);
        if (_dataStore->addItem(unassignedResource, "UNASSIGNED"))
        {

            // Finally, remove the assigned resource and assign
            // the return flag accordingly
            retFlag = _dataStore->deleteItem(assignedResource);
        }
    }

    // Return the return flag
    return retFlag;
}

/**
 * Function used to list the managed resoure groups for the
 * resource-manager Id
 *
 * @param resourceManagerId String representing the manager Id
 * @return Generator of Strings representing the resource groups
 */
std::shared_ptr<StandardModel::Generator<std::string>> GlobalState::listManagedResourceGroups(
        const std::string& resourceManagerId) const
{

    // List all of the keys under the assigned prefixed directory
    auto prefix = getAssignedPrefixedKey(resourceManagerId);
    auto listedItems = _dataStore->listItems(prefix);

    // Create and return a generator for the listing under the assigned prefix
    return std::make_shared<StandardModel::Generator<std::string>>(
            [listedItems, prefix](std::shared_ptr<StandardModel::Yieldable<std::string>> yielder)
    {

        // Loop through all of the listed keys in the assigned directory
        while (listedItems->hasMoreItems())
        {

            // Remove the prefix from each returned key and yield it
            auto resourceKey = listedItems->getNextItem();
            resourceKey.erase(0, prefix.size());
            yielder->yield(resourceKey);
        }

        // Complete the yielder to indicate we are finished
        yielder->complete();
    });
}

/**
 * Function used to list the unmanaged resoure groups for the instance
 *
 * @return Generator of Strings representing the resource groups
 */
std::shared_ptr<StandardModel::Generator<std::string>>
GlobalState::listUnmanagedResourceGroups() const
{

    // List all of the keys under the unassigned prefixed directory
    auto prefix = getUnassignedPrefixedKey();
    auto listedItems = _dataStore->listItems(prefix);

    // Create and return a generator for the listing under the unassigned prefix
    return std::make_shared<StandardModel::Generator<std::string>>(
            [listedItems, prefix](std::shared_ptr<StandardModel::Yieldable<std::string>> yielder)
    {

        // Loop through all of the listed keys in the unassigned directory
        while (listedItems->hasMoreItems())
        {

            // Remove the prefix from each returned key and yield it
            auto resourceKey = listedItems->getNextItem();
            resourceKey.erase(0, prefix.size());
            yielder->yield(resourceKey);
        }

        // Complete the yielder to indicate we are finished
        yielder->complete();
    });
}

/**
 * Function used to add a resource group to the global state
 *
 * @param groupId String representing the group Id to add
 * @return Boolean indicating whether the group was added or not
 */
bool GlobalState::addResourceGroup(const std::string& groupId)
{

    // Create a return flag
    bool retFlag = false;

    // Only continue if we are setup to make writes
    if (_accessMode == Mode::READ_WRITE)
    {

        // Only continue if the group doesn't already exist
        auto groupPrefixedkey = getResourceGroupPrefixedKey(groupId);
        if (!groupId.empty() && _dataStore->getItem(groupPrefixedkey).empty())
        {

            // If we get here, it means the group doesn't exist
            // so we'll add it with a value of (zero - the size)
            // The value of the group-id represents the size of the group
            retFlag = _dataStore->addItem(groupPrefixedkey,
                    StandardModel::Utils::getFileString({"0", "0", "0", "0"}));

            // If the resource group addition was successful, then
            // we'll proceed to add-in the unassigned placeholder for
            // it so that it can be assigned later
            if (retFlag)
                retFlag = _dataStore->addItem(
                        getUnassignedPrefixedKey(groupId), "UNASSIGNED");
        }
    }

    // Return the return flag
    return retFlag;
}

/**
 * Function used to remove a resource group to the global state
 *
 * @param groupId String representing the group Id to remove
 * @return Boolean indicating whether the group was removed or not
 */
bool GlobalState::removeResourceGroup(const std::string& groupId)
{

    // Create a return flag
    bool retFlag = false;

    // Only continue if we are setup to make writes
    if (_accessMode == Mode::READ_WRITE)
    {

        // Only continue if the group given is valid and is unassigned
        auto unassignedKey = getUnassignedPrefixedKey(groupId);
        if (!groupId.empty() &&
                (_dataStore->getItem(unassignedKey) == "UNASSIGNED"))
        {

            // Only continue if the group already exists and has no items
            // TODO - Add Regex for number validation
            auto groupPrefixedkey = getResourceGroupPrefixedKey(groupId);
            std::vector<std::string> currDetailsVect;
            auto currDetailsVectRaw = StandardModel::Utils::parseFileString(_dataStore->getItem(groupPrefixedkey));
            if (currDetailsVectRaw != nullptr)
                currDetailsVect = currDetailsVectRaw->rawVect;
            if ((currDetailsVect.size() >= 4) && (std::stoi(currDetailsVect[3]) <= 0))
            {

                // If we get here, it means the group exists and is
                // empty, so we'll remove it from the data-store
                retFlag = _dataStore->deleteItem(groupPrefixedkey);
                retFlag &= _dataStore->deleteItem(unassignedKey);
            }
        }
    }

    // Return the return flag
    return retFlag;
}

/**
 * Function used to get a resource group's cost from the global state
 *
 * @param groupId String representing the group Id to get the cost for
 * @return ResourceCost representing the cost of the entire resource group
 */
Resource::ResourceCost GlobalState::getResourceGroupCost(const std::string& groupId) const
{

    // Create a return resource cost object
    Resource::ResourceCost costObj;

    // Get the current details of the resource group cost
    auto groupPrefixedkey = getResourceGroupPrefixedKey(groupId);
    std::vector<std::string> currDetailsVect;
    auto currDetailsVectRaw = StandardModel::Utils::parseFileString(_dataStore->getItem(groupPrefixedkey));
    if (currDetailsVectRaw != nullptr)
        currDetailsVect = currDetailsVectRaw->rawVect;

    // Only continue if the group's vector is valid
    if (currDetailsVect.size() >= 4)
    {

        // Extract the cost information an put it in the return object
        costObj = Resource::ResourceCost(std::stol(currDetailsVect[0]),
                std::stol(currDetailsVect[1]), std::stoi(currDetailsVect[2]));
    }

    // Return the return resource cost object
    return costObj;
}

/**
 * Function used to list the existing resource groups in the state
 *
 * @return Generator of Strings representing the resource groups
 */
std::shared_ptr<StandardModel::Generator<std::string>> GlobalState::listResourceGroups() const
{

    // List all of the keys under the resource-group prefixed directory
    auto prefix = getResourceGroupPrefixedKey();
    auto listedItems = _dataStore->listItems(prefix);

    // Create and return a generator for the listing under the group-prefix
    return std::make_shared<StandardModel::Generator<std::string>>(
            [listedItems, prefix](std::shared_ptr<StandardModel::Yieldable<std::string>> yielder)
    {

        // Loop through all of the listed keys in the resource-group directory
        while (listedItems->hasMoreItems())
        {

            // Remove the prefix from each returned key and yield it
            auto resourceKey = listedItems->getNextItem();
            resourceKey.erase(0, prefix.size());
            yielder->yield(resourceKey);
        }

        // Complete the yielder to indicate we are finished
        yielder->complete();
    });
}

/**
 * Function used to set/add the resource data in the resource/group pair
 *
 * @param groupId String representing the group Id to use
 * @param resourceId String representing the resource Id we are setting
 * @param resource Resource representing the data to use in the process
 * @return Boolean indicating whether the resource was added/set or not
 */
bool GlobalState::setResourceInGroup(const std::string& groupId,
        const std::string& resourceId, std::shared_ptr<Resource> resource)
{

    // Create a return flag
    bool retFlag = false;

    // Only continue if we are setup to make writes and if the
    // resource itself is not NULL
    if ((_accessMode == Mode::READ_WRITE) && (resource != nullptr))
    {

        // Extract the resource data from the provided resource
        auto resourceData = SimpleResourceWrapper(resource).getFileString();

        // Only continue if the resource, group, and data are valid (non-empty)
        if (!groupId.empty() && !resourceId.empty() && !resourceData.empty())
        {

            // Get the current details of the resource group cost
            auto groupPrefixedkey = getResourceGroupPrefixedKey(groupId);
            std::vector<std::string> currDetailsVect;
            auto currDetailsVectRaw = StandardModel::Utils::parseFileString(_dataStore->getItem(groupPrefixedkey));
            if (currDetailsVectRaw != nullptr)
                currDetailsVect = currDetailsVectRaw->rawVect;

            // Get the current cost details of the existing resource itself (if present)
            auto resourceItemCost = getResourceInGroupCost(groupId, resourceId);

            // Only continue if the resource group already exists
            if (currDetailsVect.size() >= 4)
            {

                // Get the current cost and count details from the packed vector
                // TODO - Add regex validation before parsing
                auto currCost = Resource::ResourceCost(std::stol(currDetailsVect[0]),
                        std::stol(currDetailsVect[1]), std::stoi(currDetailsVect[2]));
                auto currCount = std::stoi(currDetailsVect[3]);

                // Simply add the resource to the resource group
                auto resourcePrefixedKey = getResourcePrefixedKey(groupId, resourceId);
                auto addFlag = _dataStore->addItem(resourcePrefixedKey, resourceData);

                // Only continue if the add resource data operation was successful
                if (addFlag)
                {

                    // Next, we will update the count and cost for the resource group
                    auto newCost = resource->getResourceCost();
                    auto packedVect = {std::to_string(currCost.getResourceSize() + newCost.getResourceSize() - resourceItemCost.getResourceSize()),
                            std::to_string(currCost.getMemoryRequirements() + newCost.getMemoryRequirements() - resourceItemCost.getMemoryRequirements()),
                            std::to_string(currCost.getResourceThreads() + newCost.getResourceThreads() - resourceItemCost.getResourceThreads()),
                            std::to_string(currCount + 1)};
                    retFlag = _dataStore->addItem(groupPrefixedkey, StandardModel::Utils::getFileString(packedVect));
                }
            }
        }
    }

    // Return the return flag
    return retFlag;
}

/**
 * Function used to get the resource data in the resource/group pair
 *
 * @param groupId String representing the group Id to use
 * @param resourceId String representing the resource Id we are setting
 * @return String representing the file-string used in the resource/group pair
 */
std::string GlobalState::getResourceInGroup(const std::string& groupId,
        const std::string& resourceId) const
{

    // Simply try to read and return the desired value
    auto resourcePrefixedKey = getResourcePrefixedKey(groupId, resourceId);
    return SimpleResourceWrapper(_dataStore->getItem(resourcePrefixedKey)).getOriginalResourceFileString();
}

/**
 * Function used to remove the resource data in the resource/group pair
 *
 * @param groupId String representing the group Id to use
 * @param resourceId String representing the resource Id we are removing
 * @return Boolean indicating whether the resource was removed or not
 */
bool GlobalState::removeResourceInGroup(const std::string& groupId,
        const std::string& resourceId)
{

    // Create a return flag
    bool retFlag = false;

    // Only continue if we are setup to make writes
    if (_accessMode == Mode::READ_WRITE)
    {

        // Only continue if the resource and group are valid (non-empty)
        if (!groupId.empty() && !resourceId.empty())
        {

            // Get the current details of the resource group cost
            auto groupPrefixedkey = getResourceGroupPrefixedKey(groupId);
            std::vector<std::string> currDetailsVect;
            auto currDetailsVectRaw = StandardModel::Utils::parseFileString(_dataStore->getItem(groupPrefixedkey));
            if (currDetailsVectRaw != nullptr)
                currDetailsVect = currDetailsVectRaw->rawVect;

            // Only continue if the resource group already exists
            if (currDetailsVect.size() >= 4)
            {

                // Get the current cost and count details from the packed vector
                // TODO - Add regex validation before parsing
                auto currCost = Resource::ResourceCost(std::stol(currDetailsVect[0]),
                        std::stol(currDetailsVect[1]), std::stoi(currDetailsVect[2]));
                auto currCount = std::stoi(currDetailsVect[3]);

                // Simply remove the resource from the resource group
                auto resourcePrefixedKey = getResourcePrefixedKey(groupId, resourceId);
                auto resourceToRemove = _dataStore->getItem(resourcePrefixedKey);
                auto removeFlag = _dataStore->deleteItem(resourcePrefixedKey);

                // Only continue if the remove resource data operation was successful
                if (removeFlag && !resourceToRemove.empty())
                {

                    // Setup the old resource cost object for use in updating group cost
                    auto oldCost = SimpleResourceWrapper(resourceToRemove).getResourceCost();

                    // Next, we will update the count for the resource group
                    auto packedVect = {std::to_string(currCost.getResourceSize() - oldCost.getResourceSize()),
                            std::to_string(currCost.getMemoryRequirements() - oldCost.getMemoryRequirements()),
                            std::to_string(currCost.getResourceThreads() - oldCost.getResourceThreads()),
                            std::to_string(currCount - 1)};
                    retFlag = _dataStore->addItem(groupPrefixedkey,
                            StandardModel::Utils::getFileString(packedVect));
                }
            }
        }
    }

    // Return the return flag
    return retFlag;
}

/**
 * Function used to get the resource's cost in the resource/group pair
 *
 * @param groupId String representing the group Id to use
 * @param resourceId String representing the resource Id we are getting
 * @return ResourceCost representing the cost of the entire resource item
 */
Resource::ResourceCost GlobalState::getResourceInGroupCost(const std::string& groupId,
        const std::string& resourceId) const
{

    // Get the current details of the resource item itself and return it
    auto resourcePrefixedKey = getResourcePrefixedKey(groupId, resourceId);
    return SimpleResourceWrapper(_dataStore->getItem(resourcePrefixedKey)).getResourceCost();
}

/**
 * Function used to list the existing resources in a resource group
 *
 * @param groupId String representing the group Id to list out
 * @return Generator of Strings representing the resources in the group
 */
std::shared_ptr<StandardModel::Generator<std::string>> GlobalState::listResourcesInGroup(
        const std::string& groupId) const
{

    // List all of the keys under the resource-group prefixed directory
    auto prefix = getResourcePrefixedKey(groupId);
    auto listedItems = _dataStore->listItems(prefix);

    // Create and return a generator for the listing under the resource-prefix
    return std::make_shared<StandardModel::Generator<std::string>>(
            [listedItems, prefix](std::shared_ptr<StandardModel::Yieldable<std::string>> yielder)
    {

        // Loop through all of the listed keys in the resource directory
        while (listedItems->hasMoreItems())
        {

            // Remove the prefix from each returned key and yield it
            auto resourceKey = listedItems->getNextItem();
            resourceKey.erase(0, prefix.size());
            yielder->yield(resourceKey);
        }

        // Complete the yielder to indicate we are finished
        yielder->complete();
    });
}

/**
 * Function used to clear-out the entire Global State
 *
 * @return Boolean indicating whether the operation was successful
 */
bool GlobalState::clearEntireState()
{

    // Create a return flag
    bool retFlag = false;

    // Only continue if we are setup to make writes
    if (_accessMode == Mode::READ_WRITE)
    {

        // Simply clear-out the data-store and save the results
        retFlag = _dataStore->deleteEntireDataStore();
    }

    // Return the return flag
    return retFlag;
}

/**
 * Internal function used to get a key prefixed with "ResourceGroups"
 *
 * @param groupId String representing the group Id to use
 * @return String representing the resource-group prefixed-key
 */
std::string GlobalState::getResourceGroupPrefixedKey(const std::string& groupId) const
{

    // Simply return the prefixed-key
    return std::string("ResourceGroups/") + groupId;
}

/**
 * Internal function used to get a key prefixed with "Resources"
 *
 * @param groupId String representing the group Id to use
 * @param resourceId String representing the resource Id to use
 * @return String representing the resource and group prefixed-key
 */
std::string GlobalState::getResourcePrefixedKey(const std::string& groupId,
        const std::string& resourceId) const
{

    // Simply return the prefixed-key
    return std::string("Resources/") + groupId + std::string("/") + resourceId;
}

/**
 * Internal function used to get a key prefixed with "Assignments/Unassigned"
 *
 * @param groupId String representing the group Id to use
 * @return String representing the unassigned prefixed-key
 */
std::string GlobalState::getUnassignedPrefixedKey(const std::string& groupId) const
{

    // Simply return the prefixed-key
    return std::string("Assignments/Unassigned/") + groupId;
}

/**
 * Internal function used to get a key prefixed with "Assignments/Assigned"
 *
 * @param resourceManagerId String representing the manager Id to use
 * @param groupId String representing the group Id to use
 * @return String representing the assigned prefixed-key
 */
std::string GlobalState::getAssignedPrefixedKey(const std::string& resourceManagerId,
        const std::string& groupId) const
{

    // Simply return the prefixed-key
    return std::string("Assignments/Assigned/")
            + resourceManagerId + std::string("/") + groupId;
}
