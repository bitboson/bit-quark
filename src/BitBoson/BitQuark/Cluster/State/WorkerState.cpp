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

#include <BitBoson/StandardModel/FileSystem/FileSystem.h>
#include <BitBoson/BitQuark/Cluster/State/WorkerState.h>

using namespace BitBoson;
using namespace BitBoson::BitQuark;

/**
 * Constructor used to setup the instance
 */
WorkerState::WorkerState()
{

    // Setup the data-store on a temporary directory
    _dataStore = std::make_shared<StandardModel::DataStore>(
            StandardModel::FileSystem::getTemporaryDir("BitQuark_WorkerState").getFullPath());
}

/**
 * Function used to add a resource to the instance state
 *
 * @param resourceGroup String representing the resource group
 * @param resourceId String representing the resource Id
 * @param resourceData String representing the resource data
 * @return Boolean indicating if the resource was added or not
 */
bool WorkerState::addResource(const std::string& resourceGroup,
        const std::string& resourceId, const std::string& resourceData)
{

    // Create a return flag
    bool retFlag = false;

    // Only continue if the resource group, id, and data are all valid
    if (!resourceGroup.empty() && !resourceId.empty() && !resourceData.empty())
    {

        // Simply add the data to the data-store and get the results
        retFlag = _dataStore->addItem(resourceGroup + "/" + resourceId,
                resourceData, true);
    }

    // Return the return flag
    return retFlag;
}

/**
 * Function used to get a resource from the instance state
 *
 * @param resourceGroup String representing the resource group
 * @param resourceId String representing the resource Id
 * @return String representing the resource data
 */
std::string WorkerState::getResource(const std::string& resourceGroup,
        const std::string& resourceId) const
{

    // Simply attempt to get and return the desired resource
    return _dataStore->getItem(resourceGroup + "/" + resourceId);
}

/**
 * Function used to remove a resource from the instance state
 *
 * @param resourceGroup String representing the resource group
 * @param resourceId String representing the resource Id
 * @return Boolean indicating if the resource was removed or not
 */
bool WorkerState::removeResource(const std::string& resourceGroup,
        const std::string& resourceId)
{

    // Create a return flag
    bool retFlag = false;

    // Only continue if the resource group and id are both valid
    if (!resourceGroup.empty() && !resourceId.empty())
    {

        // Simply remove the data from the data-store and get the results
        retFlag = _dataStore->deleteItem(resourceGroup + "/" + resourceId);
    }

    // Return the return flag
    return retFlag;
}

/**
 * Destructor used to cleanup the instance
 */
WorkerState::~WorkerState()
{

    // Clear-out the data-store and directory
    _dataStore->deleteEntireDataStore(false);
    StandardModel::FileSystem(_dataStore->getDataStoreDirectory()).removeDir();
}
