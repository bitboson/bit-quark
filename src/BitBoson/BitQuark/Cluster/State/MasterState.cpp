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
#include <BitBoson/StandardModel/FileSystem/FileSystem.h>
#include <BitBoson/BitQuark/Cluster/State/MasterState.h>

using namespace BitBoson;
using namespace BitBoson::BitQuark;

/**
 * Constructor used to setup the instance
 */
MasterState::MasterState()
{

    // Setup the data-store on a temporary directory
    _dataStore = std::make_shared<StandardModel::DataStore>(
            StandardModel::FileSystem::getTemporaryDir("BitQuark_MasterState").getFullPath());
}

/**
 * Function used to add a worker to the master-state instance
 *
 * @param workerId String representing the worker Id to add
 * @return Boolean indicating whether the worker was added
 */
bool MasterState::addWorker(const std::string& workerId)
{

    // Create a return flag
    bool retFlag = false;

    // Only continue if the worker Id is valid and we don't already
    // have this worker in our state
    if (!workerId.empty() && !doesWorkerExistLocally(workerId))
    {

        // Add the new worker to the worker reference
        _workerRef[workerId] = 0;
        retFlag = true;
    }

    // Return the return flag
    return retFlag;
}

/**
 * Function used to remove a worker from the master-state instance
 *
 * @param workerId String representing the worker Id to remove
 * @return Boolean indicating whether the worker was removed
 */
bool MasterState::removeWorker(const std::string& workerId)
{

    // Create a return flag
    bool retFlag = false;

    // Only continue if the worker Id is valid and no associations exist
    // but also requiring that the worker also exist in the state
    if (!workerId.empty() && doesWorkerExistLocally(workerId)
            && (_workerRef[workerId] <= 0))
    {

        // Remove the worker from the local worker reference
        _workerRef.erase(workerId);
        retFlag = true;
    }

    // Return the return flag
    return retFlag;
}

/**
 * Function used to list the existing workers in the state
 *
 * @return Generator of Strings representing the worker Ids
 */
std::shared_ptr<StandardModel::Generator<std::string>> MasterState::listWorkers() const
{

    // Create a generator to yield the items in the worker reference
    auto workerRefs = _workerRef;
    return std::make_shared<StandardModel::Generator<std::string>>(
            [workerRefs](std::shared_ptr<StandardModel::Yieldable<std::string>> yielder)
    {

        // Loop through and yield the workers in the worker reference
        for (const auto& workerRef : workerRefs)
            yielder->yield(workerRef.first);

        // Complete the yielder to indicate we are finished
        yielder->complete();
    });
}

/**
 * Function used to add an association for a worker and resource group
 *
 * @param workerId String representing the worker Id to use in the association
 * @param resourceGroup String representing the resource group to associate
 * @return Boolean indicating whether the association was successfully made
 */
bool MasterState::addAssociation(const std::string& workerId,
        const std::string& resourceGroup)
{

    // Create a return flag
    bool retFlag = false;

    // Only continue if the worker Id and resource group are valid
    // and the worker actually exists locally
    if (!workerId.empty() && !resourceGroup.empty()
            && doesWorkerExistLocally(workerId))
    {

        // Only continue if the worker doesn't already have the
        // resource group we are trying to associate/add
        std::vector<std::string> workerResources;
        auto workerResourcesRaw = StandardModel::Utils::parseFileString(
                _dataStore->getItem(workerId));
        if (workerResourcesRaw != nullptr)
            workerResources = workerResourcesRaw->rawVect;
        if (std::find(workerResources.begin(), workerResources.end(),
                resourceGroup) == workerResources.end())
        {

            // Get the reverse-association (resource-groups to workers)
            // and only continue if that association doesn't already have
            // the worker we are associating here as well
            std::vector<std::string> resourceWorkers;
            auto resourceWorkersRaw = StandardModel::Utils::parseFileString(
                    _dataStore->getItem(resourceGroup));
            if (resourceWorkersRaw != nullptr)
                resourceWorkers = resourceWorkersRaw->rawVect;
            if (std::find(resourceWorkers.begin(), resourceWorkers.end(),
                    workerId) == resourceWorkers.end())
            {

                // Append the new resource to the worker's resource list
                // and then re-add it back to the data-store, keeping
                // track of the results in the return flag
                workerResources.push_back(resourceGroup);
                retFlag = _dataStore->addItem(workerId,
                        StandardModel::Utils::getFileString(workerResources), true);

                // Append the new worker to the resources's worker list
                // and then re-add it back to the data-store, keeping
                // track of the results in the return flag as well
                resourceWorkers.push_back(workerId);
                if (retFlag)
                    retFlag &= _dataStore->addItem(resourceGroup,
                            StandardModel::Utils::getFileString(resourceWorkers), true);

                // If the add-association was successful, increment
                // the worker's association count
                if (retFlag)
                    _workerRef[workerId] = _workerRef[workerId] + 1;
            }
        }
    }

    // Return the return flag
    return retFlag;
}

/**
 * Function used to remove an association for a worker and resource group
 *
 * @param workerId String representing the worker Id to use in the removal
 * @param resourceGroup String representing the resource group to dissociate
 * @return Boolean indicating whether the association was successfully removed
 */
bool MasterState::removeAssociation(const std::string& workerId,
        const std::string& resourceGroup)
{

    // Create a return flag
    bool retFlag = false;

    // Only continue if the worker Id and resource group are valid
    // and the worker actually exists locally
    if (!workerId.empty() && !resourceGroup.empty()
            && doesWorkerExistLocally(workerId))
    {

        // Only continue here if the worker has the association already
        bool retFlagWorker = false;
        std::vector<std::string> workerResources;
        auto workerResourcesRaw = StandardModel::Utils::parseFileString(
                _dataStore->getItem(workerId));
        if (workerResourcesRaw != nullptr)
            workerResources = workerResourcesRaw->rawVect;
        auto foundItemIter = std::find(workerResources.begin(),
                workerResources.end(), resourceGroup);
        if (foundItemIter != workerResources.end())
        {

            // Remove the worker's association from its resource list
            // and then re-add it back to the data-store, keeping
            // track of the results in the return flag part
            workerResources.erase(foundItemIter);
            retFlagWorker = _dataStore->addItem(workerId,
                    StandardModel::Utils::getFileString(workerResources), true);
        }

        // Only continue here if the group has the association already
        bool retFlagGroup = false;
        std::vector<std::string> resourceWorkers;
        auto resourceWorkersRaw = StandardModel::Utils::parseFileString(
                _dataStore->getItem(resourceGroup));
        if (resourceWorkersRaw != nullptr)
            resourceWorkers = resourceWorkersRaw->rawVect;
        foundItemIter = std::find(resourceWorkers.begin(),
                resourceWorkers.end(), workerId);
        if (foundItemIter != resourceWorkers.end())
        {

            // Remove the groups's association from its worker list
            // and then re-add it back to the data-store, keeping
            // track of the results in the return flag part
            resourceWorkers.erase(foundItemIter);
            retFlagGroup = _dataStore->addItem(resourceGroup,
                    StandardModel::Utils::getFileString(resourceWorkers), true);
        }

        // Only return true if both return flags were true
        if (retFlagWorker && retFlagGroup)
            retFlag = true;

        // If the remove-association was successful, decrement
        // the worker's association count
        if (retFlag)
            _workerRef[workerId] = _workerRef[workerId] - 1;
    }

    // Return the return flag
    return retFlag;
}

/**
 * Function used to get the workers for the given resource group
 *
 * @param resourceGroup String representing the group to get the worker for
 * @return Generator of Strings representing the associated worker Ids
 */
std::shared_ptr<StandardModel::Generator<std::string>> MasterState::getWorkersForResourceGroup(
        const std::string& resourceGroup) const
{

    // Create a generator to yield the worker items for the resource group
    std::vector<std::string> resourceWorkers;
    auto resourceWorkersRaw = StandardModel::Utils::parseFileString(
            _dataStore->getItem(resourceGroup));
    if (resourceWorkersRaw != nullptr)
        resourceWorkers = resourceWorkersRaw->rawVect;
    return std::make_shared<StandardModel::Generator<std::string>>(
            [resourceWorkers](std::shared_ptr<StandardModel::Yieldable<std::string>> yielder)
    {

        // Loop through and yield the workers in the resource list
        for (const auto& workerId : resourceWorkers)
            yielder->yield(workerId);

        // Complete the yielder to indicate we are finished
        yielder->complete();
    });
}

/**
 * Function used to get the resource groups for the given worker
 *
 * @param workerId String representing the worker to get the groups for
 * @return Generator of Strings representing the associated groups
 */
std::shared_ptr<StandardModel::Generator<std::string>> MasterState::getResourceGroupsForWorker(
        const std::string& workerId) const
{

    // Create a generator to yield the resource group items for the worker
    std::vector<std::string> workerResources;
    auto workerResourcesRaw = StandardModel::Utils::parseFileString(
            _dataStore->getItem(workerId));
    if (workerResourcesRaw != nullptr)
        workerResources = workerResourcesRaw->rawVect;
    return std::make_shared<StandardModel::Generator<std::string>>(
            [workerResources](std::shared_ptr<StandardModel::Yieldable<std::string>> yielder)
    {

        // Loop through and yield the resource in the worker list
        for (const auto& resourceGroup : workerResources)
            yielder->yield(resourceGroup);

        // Complete the yielder to indicate we are finished
        yielder->complete();
    });
}

/**
 * Function used to check if the given worker exists or noe
 *
 * @param workerId String representing the worker Id to check
 * @return Boolean indicating whether the worker exists or not
 */
bool MasterState::doesWorkerExistLocally(const std::string& workerId) const
{

    // Create a return flag
    bool retFlag = false;

    // Only continue if the worker Id is valid
    if (!workerId.empty())
    {

        // Attempt to find a worker entry in the worker reference map
        for (const auto& workerRefItem : _workerRef)
            if (workerRefItem.first == workerId)
                retFlag = true;
    }

    // Return the return flag
    return retFlag;
}

/**
 * Destructor used to cleanup the instance
 */
MasterState::~MasterState()
{

    // Clear-out the data-store and directory
    _dataStore->deleteEntireDataStore(false);
    StandardModel::FileSystem(_dataStore->getDataStoreDirectory()).removeDir();
}
