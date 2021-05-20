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

#include <vector>
#include <BitBoson/StandardModel/Utils/Utils.h>
#include <BitBoson/StandardModel/Crypto/Crypto.h>
#include <BitBoson/BitQuark/Storage/S3DataStore.h>
#include <BitBoson/BitQuark/Storage/S3Credentials.h>

using namespace BitBoson;
using namespace BitBoson::BitQuark;

/**
 * Constructor used to initialize the credential object's instance
 *
 * @param s3Endpoint String representing the complete S3-Endpoint
 * @param bucket String representing the bucket name to use
 * @param directoryPrefix String representing the directory prefix to use
 * @param accessKey String representing the access key for the bucket
 * @param secretKey String representing the secret key for the bucket
 */
S3Credentials::S3Credentials(const std::string& s3Endpoint, const std::string& bucket,
              const std::string& directoryPrefix, const std::string& accessKey,
              const std::string& secretKey)
{

    // Setup the member variables
    _endpoint = s3Endpoint;
    _bucket = bucket;
    _dirPrefix = directoryPrefix;
    _accessKey = accessKey;
    _secretKey = secretKey;
}

/**
 * Constructor used to setup the credential object from the given file-string
 *
 * @param fileString String representing the file-string to create the instance from
 */
S3Credentials::S3Credentials(const std::string& fileString)
{

    // Setup the defaults for a new instance
    setupDefaults();

    // Attempt to setup the instance from the file-string, reverting on failure
    if (!setFileString(fileString))
        setupDefaults();
}

/**
 * Function used to get the S3-Endpoint key from the object
 *
 * @return String representing the S3-Endpoint
 */
std::string S3Credentials::getS3Endpoint()
{

    // Get and return the endpoint
    return _endpoint;
}

/**
 * Function used to get the S3-Bucket key from the object
 *
 * @return String representing the S3-Bucket
 */
std::string S3Credentials::getBucket()
{

    // Get and return the bucket
    return _bucket;
}

/**
 * Function used to get the S3-Bucket directory/key prefix from the object
 *
 * @return String representing the directory/key prefix
 */
std::string S3Credentials::getDirectoryPrefix()
{

    // Get and return the directory/key prefix
    return _dirPrefix;
}

/**
 * Function used to get the access key from the object
 *
 * @return String representing the access key
 */
std::string S3Credentials::getAccessKey()
{

    // Get and return the access key
    return _accessKey;
}

/**
 * Function used to get the secret key from the object
 *
 * @return String representing the secret key
 */
std::string S3Credentials::getSecretKey()
{

    // Get and return the secret key
    return _secretKey;
}

/**
 * Overridden function used to get the file-string-representation of the object
 *
 * @return String representing the cache-able data for the object
 */
std::string S3Credentials::getFileString() const
{

    // Create the packed vector for the members of the instance
    std::vector<std::string> packedVect;
    packedVect.push_back(_accessKey);
    packedVect.push_back(_secretKey);
    packedVect.push_back(_bucket);
    packedVect.push_back(_endpoint);
    packedVect.push_back(_dirPrefix);

    // Return the file-string for the packed vector
    return StandardModel::Utils::getFileString(packedVect);
}

/**
 * Overridden function used to setup the object based on the given file-string-representation
 *
 * @param fileString String representing the cache-able data for the object to create from
 * @return Boolean indicating whether the given string was parsed and processed properly
 */
bool S3Credentials::setFileString(const std::string& fileString)
{

    // Create a return flag
    bool retFlag = false;

    // Setup the defaults for the instance
    setupDefaults();

    // Only continue if the file-string is not empty
    if (!fileString.empty())
    {

        // Extract the packed vector from the file-string and verify it's size criteria
        auto packedVect = StandardModel::Utils::parseFileString(fileString);
        if ((packedVect != nullptr) && (packedVect->size >= 5))
        {

            // Setup the instance variables from the packed vector
            _accessKey = StandardModel::Utils::getNextFileStringValue(packedVect);
            _secretKey = StandardModel::Utils::getNextFileStringValue(packedVect);
            _bucket = StandardModel::Utils::getNextFileStringValue(packedVect);
            _endpoint = StandardModel::Utils::getNextFileStringValue(packedVect);
            _dirPrefix = StandardModel::Utils::getNextFileStringValue(packedVect);

            // Setup the return flag accordingly
            retFlag = (packedVect->index == packedVect->size);
        }

        // If the operation failed, then reset the instance
        if (!retFlag)
            setupDefaults();
    }

    // Return the return flag
    return retFlag;
}

/**
 * Overridden function used to get the unique hash (SHA256) of the underlying object
 *
 * @return String representing the unique (SHA256) hash of the underlying object
 */
std::string S3Credentials::getUniqueHash() const
{

    // Create a return string
    std::string retString;

    // Add in the values from all members in some way or another
    retString += StandardModel::Crypto::sha256(_accessKey);
    retString += StandardModel::Crypto::sha256(_secretKey);
    retString += StandardModel::Crypto::sha256(_bucket);
    retString += StandardModel::Crypto::sha256(_endpoint);
    retString += StandardModel::Crypto::sha256(_dirPrefix);

    // Ensure it fits the SHA256 format/criteria
    retString = StandardModel::Crypto::sha256(retString);

    // Return the return string
    return retString;
}

/**
 * Internal function used to setup default values for the instance
 */
void S3Credentials::setupDefaults()
{

    // Setup the member/instance defaults
    _accessKey = "";
    _secretKey = "";
    _bucket = "";
    _endpoint = "";
    _dirPrefix = "";
}
