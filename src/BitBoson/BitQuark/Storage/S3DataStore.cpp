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

#include <aws/core/Aws.h>
#include <aws/s3/model/Delete.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/DeleteObjectsRequest.h>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <BitBoson/StandardModel/Utils/Utils.h>
#include <BitBoson/StandardModel/Crypto/Crypto.h>
#include <BitBoson/BitQuark/Storage/S3DataStore.h>

using namespace BitBoson;
using namespace BitBoson::BitQuark;

/**
 * Constructor used to setup the s3-data-store instance
 *
 * @param s3Credentials S3 Credentials used to setup the S3 connection with
 */
S3DataStore::S3DataStore(std::shared_ptr<S3Credentials> s3Credentials)
{

    // Setup member variables
    _bucket = s3Credentials->getBucket();
    _directory = s3Credentials->getDirectoryPrefix();
    auto usePathStyleAddressing = true; // TODO: Implement in S3Credentials

    // Obtain the AWS SDK Options (force Singleton Instance)
    _awsOptions = AwsOptionsSingleton::getAwsOptions();

    // Setup the Client Configuration (to use configured HTTP(S) protocol)
    Aws::Client::ClientConfiguration config;
    config.endpointOverride = s3Credentials->getS3Endpoint();
    config.scheme = Aws::Http::Scheme::HTTP;

    // Setup the S3 Credentials
    Aws::Auth::AWSCredentials credentials{Aws::String(s3Credentials->getAccessKey()),
            Aws::String(s3Credentials->getSecretKey())};

    // Setup the S3 Client based on the provided keys and configuration information
    _s3Client = std::make_shared<Aws::S3::S3Client>(credentials, config,
            Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Always, !usePathStyleAddressing);

    // Load the S3-Meta-Data from the S3-Data-Store directly
    _internalMd = getMetaData();
}

/**
 * Function used to add an item to the s3-data-store
 *
 * @param key String representing the key for the item to add
 * @param item String item to add to the data store
 * @return Boolean indicating whether the item was added or not
 */
bool S3DataStore::addItem(const std::string& key, const std::string& item)
{

    // Create a return flag
    bool wasAdded = false;

    // Only process if the key isn't empty
    // and doesn't start with a '.'
    if (!key.empty() && (key[0] != '.'))
    {

        // Start by getting the size of the size of the object
        auto currSize = getObjectSize(key);

        // Next, add the item to the s3-bucket
        wasAdded = addItemHelper(key, item);

        // If the operation was successful, update the metadata
        if (wasAdded)
        {

            // Update the total size in the metadata
            auto newSize = item.size() - currSize;
            _internalMd.dataSize += newSize;

            // Add the current object's size to the memoization map
            _memoizationMap[key] = item.size();

            // Push out the updated internal metadata
            setMetaData(_internalMd);
        }
    }

    // Return the return flag
    return wasAdded;
}

/**
 * Function used to get the value for the given key
 *
 * @param key String representing the key for the item to get
 * @return String representing the value for the given key
 */
std::string S3DataStore::getItem(const std::string& key)
{

    // Create the return string/value
    std::string retValue;

    // Only process if the key isn't empty
    if (!key.empty())
    {

        // Create the Get Object request
        Aws::S3::Model::GetObjectRequest getObjectRequest;
        getObjectRequest.WithBucket(_bucket).WithKey(_directory + "/" + Aws::String(key));

        // Actually perform the request on the given client
        auto getObjectOutcome = _s3Client->GetObject(getObjectRequest);

        // Only attempt to write the file if the request was successful
        if (getObjectOutcome.IsSuccess())
        {

            // Extract the object data from the response and save it in memory
            std::ostringstream localStream;
            localStream << getObjectOutcome.GetResult().GetBody().rdbuf();
            retValue = localStream.str();
        }
    }

    // Return the return value
    return retValue;
}

/**
 * Function used to get the given object's size
 *
 * @param key String representing the key for the item to get
 * @return Long Long Integer representing the object's size in bytes
 */
long long int S3DataStore::getObjectSize(const std::string& key)
{

    // Create the return value
    long long int retValue = 0;

    // Only process if the key isn't empty
    if (!key.empty())
    {

        // Attempt to get the value from the memoization map
        long currMapVal = -1;
        if (_memoizationMap.find(key) != _memoizationMap.end())
            currMapVal = _memoizationMap[key];

        // Create the Head Object request
        Aws::S3::Model::HeadObjectRequest headObjectRequest;
        headObjectRequest.WithBucket(_bucket).WithKey(_directory + "/" + Aws::String(key));

        // Actually perform the request on the given client
        auto headObjectOutcome = _s3Client->HeadObject(headObjectRequest);

        // Only attempt to write the file if the request was successful
        if (headObjectOutcome.IsSuccess())
        {

            // Extract the object's size from the head-object response
            retValue = headObjectOutcome.GetResult().GetContentLength();
        }

        // If the memoization map's value is the same as the cloud
        // value, and they are non-negative, then remove the value
        // from the memoization map since things are synchronized
        if ((currMapVal == retValue) && (currMapVal >= 0))
            _memoizationMap.erase(key);

        // We always trust the current map value so return it
        // as long as it exists, otherwise we'll leave the cloud
        // value in place (because the map doesn't have the item)
        if (currMapVal >= 0)
            retValue = currMapVal;
    }

    // Return the return value
    return retValue;
}

/**
 * Function used to list all of the items in the S3 Data-store
 * NOTE: This will effectively translate to S3-list operation(s)
 *
 * @param prefix String representing the object-key prefix to use (if any)
 * @return Generator of Strings representing the keys in the S3 data-store
 */
std::shared_ptr<StandardModel::Generator<std::string>> S3DataStore::listItems(
        const std::string& prefix)
{

    // Create and return a generator for getting the S3 Data-Store elements
    auto bucket = _bucket;
    auto directory = _directory;
    auto s3Client = _s3Client;
    return std::make_shared<StandardModel::Generator<std::string>>(
            [bucket, directory, s3Client, prefix]
            (std::shared_ptr<StandardModel::Yieldable<std::string>> yielder)
        {

            // Run in a loop to list/delete everything
            bool keepListing = true;
            bool wasTruncated = false;
            Aws::String previousMarker;
            while (keepListing)
            {

                // Exit the loop early if the generator terminated
                if (yielder->isTerminated())
                    break;

                // Construct the list-objects request
                Aws::S3::Model::ListObjectsRequest listObjectsRequest;
                listObjectsRequest.WithBucket(bucket).WithPrefix(directory + Aws::String("/" + prefix));

                // Add in the marker from the previous listing (if applicable)
                if (wasTruncated)
                    listObjectsRequest.WithMarker(previousMarker);

                // Actually perform the request
                auto objectListing = s3Client->ListObjects(listObjectsRequest);

                // Only continue if the operation was successful
                if (objectListing.IsSuccess())
                {

                    // Loop through all of the results and extract all object keys
                    // and yield them to the input yielder
                    auto objectList = objectListing.GetResult().GetContents();
                    for (const auto& s3Object : objectList)
                    {
                        Aws::String keyString = s3Object.GetKey();
                        keyString.erase(0, directory.size() + 1);

                        // Exit the loop early if the generator terminated
                        if (yielder->isTerminated())
                            break;

                        // Only yield items that don't start with a '.'
                        if ((keyString[0] != '.') && (!keyString.empty()))
                            yielder->yield(keyString.c_str());
                    }

                    // Determine if we need to keep looping (i.e. if the response was truncated)
                    wasTruncated = objectListing.GetResult().GetIsTruncated();
                    previousMarker = objectListing.GetResult().GetNextMarker();
                    keepListing = wasTruncated;
                }

                // Handle the case where the object listing failed (return false)
                else
                {

                    // Indicate to stop listing (and exit the loop) if an error occurred
                    keepListing = false;
                }
            }

            // Complete the yielder
            yielder->complete();
        });
}

/**
 * Function used to get the S3-Data-Store size
 * NOTE: This only accounts for object raw data
 *
 * @return Long representing the size in bytes
 */
long S3DataStore::getSize()
{
    // Get and return the internally tracked size
    return _internalMd.dataSize;
}

/**
 * Function used to delete the given item from the key-value s3-data-store
 *
 * @param key String representing the key for the item to delete
 * @return Boolean indicating whether the item was deleted or not
 */
bool S3DataStore::deleteItem(const std::string& key)
{

    // Create a return flag
    bool wasDeleted = false;

    // Only process if the key isn't empty
    if (!key.empty())
    {

        // Get the object's original size (in the bucket)
        auto origSize = getObjectSize(key);

        // Create the Delete Object Request
        Aws::S3::Model::DeleteObjectRequest deleteObjectResult;
        deleteObjectResult.WithBucket(_bucket).WithKey(_directory + "/" + Aws::String(key));

        // Delete the object from the bucket and verify the results
        wasDeleted = _s3Client->DeleteObject(deleteObjectResult).IsSuccess();

        // If the operation was successful, update the metadata
        // NOTE: Do not do this if the key deleted begins with a '.'
        if (wasDeleted && (key[0] != '.'))
        {

            // Update the total size in the metadata
            _internalMd.dataSize -= origSize;

            // Remove the current object's size from the memoization map
            _memoizationMap.erase(key);

            // Push out the updated internal metadata
            setMetaData(_internalMd);
        }
    }

    // Return the return flag
    return wasDeleted;
}

/**
 * Function used to delete the entire s3-data-store bucket directory
 *
 * @param supportsMultiDelete Boolean indicating whether the back-end
 *                            cloud provider supports multi-item S3 delete
 * @return Boolean indicating if all of the items were deleted or no
 */
bool S3DataStore::deleteEntireDataStore(bool supportsMultiDelete)
{

    // Create a return flag
    bool retFlag = true;

    // Run in a loop to list/delete everything
    bool keepListing = true;
    bool wasTruncated = false;
    Aws::String previousMarker;
    while (keepListing)
    {

        // Construct the list-objects request
        Aws::S3::Model::ListObjectsRequest listObjectsRequest;
        listObjectsRequest.WithBucket(_bucket).WithPrefix(_directory);

        // Add in the marker from the previous listing (if applicable)
        if (wasTruncated)
            listObjectsRequest.WithMarker(previousMarker);

        // Actually perform the request
        auto objectListing = _s3Client->ListObjects(listObjectsRequest);

        // Only continue if the operation was successful
        if (objectListing.IsSuccess())
        {

            // Loop through all of the results and extract all object keys
            Aws::Vector<Aws::S3::Model::ObjectIdentifier> deleteVect;
            auto objectList = objectListing.GetResult().GetContents();
            for (const auto& s3Object : objectList)
                deleteVect.push_back(Aws::S3::Model::ObjectIdentifier().WithKey(s3Object.GetKey()));

            // If the backend supports multi-item deletion, delete all keys
            // at the same time using a single request
            if (supportsMultiDelete && !deleteVect.empty())
            {

                // Create the multi-item delete request
                auto deleteItems = Aws::S3::Model::Delete().WithObjects(deleteVect);
                Aws::S3::Model::DeleteObjectsRequest deleteObjectsRequest;
                deleteObjectsRequest.WithBucket(_bucket).WithDelete(deleteItems);

                // Actually perform the object deletion
                retFlag &= _s3Client->DeleteObjects(deleteObjectsRequest).IsSuccess();
            }

            // If the backend does not support multi-item delete, then delete
            // the listed object keys one-by-one
            else
            {

                // Loop through all of the items and delete them individually
                for (const auto& item : deleteVect)
                {

                    // Obtain the key from the object
                    Aws::String keyString = item.GetKey();

                    // Strip-off the directory and delete the object by key
                    keyString.erase(0, _directory.size() + 1);
                    retFlag &= deleteItem(keyString.c_str());
                }
            }

            // Determine if we need to keep looping (i.e. if the response was truncated)
            wasTruncated = objectListing.GetResult().GetIsTruncated();
            keepListing = wasTruncated;
        }

        // Handle the case where the object listing failed (return false)
        else
        {

            // Setup the return value and exit the loop
            keepListing = false;
            retFlag = false;
        }
    }

    // Handle any local and cloud metadata changes on success
    if (retFlag)
    {

        // Make sure we also completely dump the memoization cache
        _memoizationMap.clear();

        // Update the metadata accordingly
        _internalMd.dataSize = 0;
    }

    // Return the return flag
    return retFlag;
}

/**
 * Function used to add a misc. metadata key-value pair to the S3-data-store
 *
 * @param key String representing the key for the metadata item
 * @param value String representing the value for the metadata item
 */
void S3DataStore::setMiscMetadataValue(const std::string& key, const std::string& value)
{

    // Add-in the key-value pair/item for the metadata value
    _internalMd.miscMetadata[key] = value;

    // Sync/push the metadata back up to the cloud
    setMetaData(_internalMd);
}

/**
 * Function used to get a misc. metadata value for the given key in the S3-data-store
 *
 * @param key String representing the key for the metadata item
 * @param defaultVal String representing the default value if the item doesn't exist
 * @return String representing the value for the metadata item
 */
std::string S3DataStore::getMiscMetadataValue(const std::string& key, const std::string& defaultVal)
{

    // Create a return string
    std::string retString = defaultVal;

    // Get the metadata value if it exists locally
    auto mdIterator = _internalMd.miscMetadata.find(key);
    if (mdIterator != _internalMd.miscMetadata.end())
        retString = mdIterator->second;

    // Return the return string
    return retString;
}

/**
 * Internal helper function used to add an item to the s3-data-store
 *
 * @param key String representing the key for the item to add
 * @param item String item to add to the data store
 * @return Boolean indicating whether the item was added or not
 */
bool S3DataStore::addItemHelper(const std::string& key, const std::string& item)
{

    // Create a return flag
    bool wasAdded = false;

    // Only process if the key isn't empty
    if (!key.empty())
    {

        // Create the Put Object Request
        Aws::S3::Model::PutObjectRequest putObjectRequest;
        putObjectRequest.WithBucket(_bucket).WithKey(_directory + "/" + Aws::String(key));

        // Create the input stream (IOStream) from the input string item
        typedef boost::iostreams::basic_array_source<char> StringStream;
        boost::iostreams::stream_buffer<StringStream> inputDataRaw(item.c_str(), item.size());
        auto inputData = std::make_shared<std::iostream>(&inputDataRaw);
        putObjectRequest.SetBody(std::shared_ptr<Aws::IOStream>(inputData));
        putObjectRequest.SetContentLength(item.size());

        // Put the object in the bucket and verify the results
        // NOTE: Will succeed if the item already exists
        wasAdded = _s3Client->PutObject(putObjectRequest).IsSuccess();
    }

    // Return the return flag
    return wasAdded;
}

/**
 * Function used to flush (or remove) no-longer needed cache values
 *
 * @param ensureConsistent Boolean indicating to ensure the local
 *                         state is consistent with the cloud state
 */
void S3DataStore::flushCacheIfPossible(bool ensureConsistent)
{

    // Continuously call the cache-flush process until we
    // have met any specified consistency requirements
    // Ensure it is done at least once regardless
    do
    {

        // Extract all of the keys from the memoization map
        std::vector<std::string> memoizedKeys;
        for (const auto& cacheItem : _memoizationMap)
            memoizedKeys.push_back(cacheItem.first);

        // Loop through every item in the memoization map
        // performing a "getObjectSize" operation, which
        // will also handle flushing the cache out
        for (const auto& cacheKey : memoizedKeys)
            getObjectSize(cacheKey);

    // Ensure we retry if required
    } while (ensureConsistent && !_memoizationMap.empty());
}

/**
 * Function used to get the metadata structure for the instance
 *
 * @return S3-Meta-Data structure representing the instance's metadata
 */
S3DataStore::S3MetaData S3DataStore::getMetaData()
{

    // Create a return S3-Meta-Data structure
    auto retStruct = S3MetaData();

    // Get the file-string associated with the metadata
    // from the KVI instance itself
    auto kviMetaDataString = getItem(".s3datastore/metadata");
    if (!kviMetaDataString.empty())
    {

        // Extract the packed-vector associated with the metadata
        auto packedVect = StandardModel::Utils::parseFileString(kviMetaDataString);
        if ((packedVect != nullptr) && (packedVect->size >= 2))
        {

            // Build-up the metadata structure
            retStruct.dataSize = std::stoll(StandardModel::Utils::getNextFileStringValue(packedVect));

            // Build-up the misc. metadata values
            auto miscPackedMdVect = StandardModel::Utils::parseFileString(
                    StandardModel::Utils::getNextFileStringValue(packedVect));
            if ((miscPackedMdVect != nullptr) && (miscPackedMdVect->size >= 2) && (miscPackedMdVect->size % 2 == 0))
                for (unsigned long ii = 0; ii < miscPackedMdVect->size; ii+=2)
                    retStruct.miscMetadata[miscPackedMdVect->rawVect[ii]] = miscPackedMdVect->rawVect[ii + 1];
        }
    }

    // Return the return metadata structure
    return retStruct;
}

/**
 * Function used to set the metadata structure for the instance
 *
 * @param s3MetaData S3-Meta-Data structure representing the instance's metadata
 * @return Boolean indicating whether the metadata was updated/set or not
 */
bool S3DataStore::setMetaData(S3MetaData s3MetaData)
{

    // Create a return flag
    bool retFlag = false;

    // Build-up the packed-vector for the metadata
    std::vector<std::string> packedVect;
    packedVect.push_back(std::to_string(s3MetaData.dataSize));

    // Add the misc. metadata values to the packed-vector
    std::vector<std::string> miscMd;
    for (const auto& miscMdItem : s3MetaData.miscMetadata)
    {
        miscMd.push_back(miscMdItem.first);
        miscMd.push_back(miscMdItem.second);
    }
    packedVect.push_back(StandardModel::Utils::getFileString(miscMd));

    // Get the file-string for the packed vector
    auto kviMetaDataString = StandardModel::Utils::getFileString(packedVect);

    // Replace the metadata to the KVI instance directly
    retFlag = addItemHelper(".s3datastore/metadata", kviMetaDataString);

    // Try to keep the cache flushed
    flushCacheIfPossible();

    // Return the return flag
    return retFlag;
}

/**
 * Destructor used to cleanup and sync the S3 instance
 */
S3DataStore::~S3DataStore()
{

    // Wait until all cloud items are consistent
    flushCacheIfPossible(true);

    // Actually clear any internal variables
    _memoizationMap.clear();
}
