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
#include <BitBoson/StandardModel/Crypto/Crypto.h>
#include <BitBoson/BitQuark/Cluster/State/Resource.h>

using namespace BitBoson;
using namespace BitBoson::BitQuark;

/**
 * Constructor used to setup the instance
 */
Resource::ResourceCost::ResourceCost()
{

    // Setup the default member variable values
    _resourceSize = 0;
    _memorySize = 0;
    _threadCount = 0;
}

/**
 * Constructor used to setup the instance with values
 *
 * @param resourceSize Long representing the resource size
 * @param memorySize Long representing the resource size
 * @param threadCount Integer representing the resource size
 */
Resource::ResourceCost::ResourceCost(long resourceSize,
        long memorySize, int threadCount)
{

    // Setup the given member variable values
    _resourceSize = resourceSize;
    _memorySize = memorySize;
    _threadCount = threadCount;
}

/**
 * Function used to set the resource's size (in bytes)
 *
 * @param size Long representing the size of the resource
 */
void Resource::ResourceCost::setResourceSize(long size)
{

    // Set the appropriate member variable
    _resourceSize = size;
}

/**
 * Function used to get the resource's size (in bytes)
 *
 * @return long representing the size of the resource
 */
long Resource::ResourceCost::getResourceSize() const
{

    // Return the appropriate member variable
    return _resourceSize;
}

/**
 * Function used to set the resource's memory requirements (in bytes)
 *
 * @param size Long representing the size of the resource's memory requirements
 */
void Resource::ResourceCost::setMemoryRequirements(long size)
{

    // Set the appropriate member variable
    _memorySize = size;
}

/**
 * Function used to get the resource's memory requirements (in bytes)
 *
 * @return long representing the size of the resource's memory requirements
 */
long Resource::ResourceCost::getMemoryRequirements() const
{

    // Return the appropriate member variable
    return _memorySize;
}

/**
 * Function used to set the resource's thread-count requirements
 *
 * @param count Integer representing the number of threads required by the resource
 */
void Resource::ResourceCost::setResourceThreads(int count)
{

    // Set the appropriate member variable
    _threadCount = count;
}

/**
 * Function used to get the resource's thread-count requirements
 *
 * @return integer representing the number of threads required by the resource
 */
int Resource::ResourceCost::getResourceThreads() const
{

    // Return the appropriate member variable
    return _threadCount;
}

/**
 * Overridden function used to get the file-string-representation of the object
 *
 * @return String representing the cache-able data for the object
 */
std::string Resource::getFileString() const
{

    // Simply get the file-string for the packed vector of the resource instance
    return StandardModel::Utils::getFileString(getPackedVector());
}

/**
 * Overridden function used to setup the object based on the given file-string-representation
 *
 * @param fileString String representing the cache-able data for the object to create from
 * @return Boolean indicating whether the given string was parsed and processed properly
 */
bool Resource::setFileString(const std::string& fileString)
{

    // Create a return flag
    bool retFlag = false;

    // Simply setup the resource instance using the packed-vector form of the file-string
    auto parsedFileString = StandardModel::Utils::parseFileString(fileString);
    if (parsedFileString != nullptr)
        retFlag = setPackedVector(parsedFileString->rawVect);

    // Return the return flag
    return retFlag;
}

/**
 * Overridden function used to get the unique hash (SHA256) of the underlying object
 *
 * @return String representing the unique (SHA256) hash of the underlying object
 */
std::string Resource::getUniqueHash() const
{

    // Simply return the unique hash as the hash of the packed-vector (in order)
    std::string currHashVal;
    for (const auto& item : getPackedVector())
        currHashVal = StandardModel::Crypto::sha256(currHashVal + item);
    return currHashVal;
}
