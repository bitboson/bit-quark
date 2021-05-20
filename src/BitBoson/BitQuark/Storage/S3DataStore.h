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

#ifndef BITQUARK_S3DATASTORE_H
#define BITQUARK_S3DATASTORE_H

#include <mutex>
#include <iostream>
#include <istream>
#include <streambuf>
#include <string>
#include <memory>
#include <unordered_map>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <BitBoson/StandardModel/Primitives/Generator.hpp>
#include <BitBoson/BitQuark/Storage/S3Credentials.h>

using namespace BitBoson;
namespace BitBoson::BitQuark
{

    class S3DataStore
    {

        // Private structures
        private:
            struct S3MetaData
            {
                long long int dataSize;
                std::unordered_map<std::string, std::string> miscMetadata;
            };

        // Private internal class
        private:
            class AwsOptionsSingleton
            {

                // Private member variables
                private:
                    Aws::SDKOptions _awsOptions;

                // Public member functions
                public:

                    /**
                     * Static function used to get the underlying
                     * AWS SDK Options from the Singleton Class
                     *
                     * @return AWS SDK Options from the Singleton
                     */
                    //___attribute__((no_sanitize("memory")))
                    static Aws::SDKOptions getAwsOptions()
                    {
                        return getInstance()._awsOptions;
                    }

                    /**
                     * Destructor used to cleanup the Singleton Class
                     */
                    //___attribute__((no_sanitize("memory")))
                    virtual ~AwsOptionsSingleton()
                    {

                        // Properly shutdown the AWS API
                        Aws::ShutdownAPI(_awsOptions);
                    }

                // Private member functions
                private:

                    /**
                     * Internal constructor used to setup the
                     * Singleton Class instance
                     */
                    //___attribute__((no_sanitize("memory")))
                    AwsOptionsSingleton()
                    {

                        // Initialize the AWS API
                        _awsOptions = Aws::SDKOptions();
                        Aws::InitAPI(_awsOptions);
                    }

                    /**
                     * Internal static get-instance function to get the
                     * instance of the Singleton Class
                     *
                     * @return Singleton Class instance
                     */
                    //___attribute__((no_sanitize("memory")))
                    static AwsOptionsSingleton& getInstance()
                    {

                        // Setup the Singleton Instance
                        static AwsOptionsSingleton instance;

                        // Return the Singleton instance
                        return instance;
                    }
            };

        // Private member variables
        private:
            Aws::String _bucket;
            Aws::String _directory;
            S3MetaData _internalMd;
            Aws::SDKOptions _awsOptions;
            std::shared_ptr<Aws::S3::S3Client> _s3Client;
            std::unordered_map<std::string, long> _memoizationMap;

        // Public member functions
        public:

            /**
             * Constructor used to setup the s3-data-store instance
             *
             * @param s3Credentials S3 Credentials used to setup the S3 connection with
             */
            explicit S3DataStore(std::shared_ptr<S3Credentials> s3Credentials);

            /**
             * Function used to add an item to the s3-data-store
             *
             * @param key String representing the key for the item to add
             * @param item String item to add to the data store
             * @return Boolean indicating whether the item was added or not
             */
            bool addItem(const std::string& key, const std::string& item);

            /**
             * Function used to get the value for the given key
             *
             * @param key String representing the key for the item to get
             * @return String representing the value for the given key
             */
            std::string getItem(const std::string& key);

            /**
             * Function used to get the given object's size
             *
             * @param key String representing the key for the item to get
             * @return Long Long Integer representing the object's size in bytes
             */
            long long int getObjectSize(const std::string& key);

            /**
             * Function used to list all of the items in the S3 Data-store
             * NOTE: This will effectively translate to S3-list operation(s)
             *
             * @param prefix String representing the object-key prefix to use (if any)
             * @return Generator of Strings representing the keys in the S3 data-store
             */
            std::shared_ptr<StandardModel::Generator<std::string>> listItems(const std::string& prefix="");

            /**
             * Function used to get the S3-Data-Store size
             * NOTE: This only accounts for object raw data
             *
             * @return Long representing the size in bytes
             */
            long getSize();

            /**
             * Function used to delete the given item from the key-value s3-data-store
             *
             * @param key String representing the key for the item to delete
             * @return Boolean indicating whether the item was deleted or not
             */
            bool deleteItem(const std::string& key);

            /**
             * Function used to delete the entire s3-data-store bucket directory
             *
             * @param supportsMultiDelete Boolean indicating whether the back-end
             *                            cloud provider supports multi-item S3 delete
             * @return Boolean indicating if all of the items were deleted or no
             */
            bool deleteEntireDataStore(bool supportsMultiDelete=true);

            /**
             * Function used to add a misc. metadata key-value pair to the S3-data-store
             *
             * @param key String representing the key for the metadata item
             * @param value String representing the value for the metadata item
             */
            void setMiscMetadataValue(const std::string& key, const std::string& value);

            /**
             * Function used to get a misc. metadata value for the given key in the S3-data-store
             *
             * @param key String representing the key for the metadata item
             * @param defaultVal String representing the default value if the item doesn't exist
             * @return String representing the value for the metadata item
             */
            std::string getMiscMetadataValue(const std::string& key, const std::string& defaultVal="");

            /**
             * Destructor used to cleanup and sync the S3 instance
             */
            virtual ~S3DataStore();

        // Private member functions
        private:

            /**
             * Internal helper function used to add an item to the s3-data-store
             *
             * @param key String representing the key for the item to add
             * @param item String item to add to the data store
             * @return Boolean indicating whether the item was added or not
             */
            bool addItemHelper(const std::string& key, const std::string& item);

            /**
             * Function used to flush (or remove) no-longer needed cache values
             *
             * @param ensureConsistent Boolean indicating to ensure the local
             *                         stat is consistent with the cloud state
             */
            void flushCacheIfPossible(bool ensureConsistent = false);

            /**
             * Function used to get the metadata structure for the instance
             *
             * @return S3-Meta-Data structure representing the instance's metadata
             */
            S3MetaData getMetaData();

            /**
             * Function used to set the metadata structure for the instance
             *
             * @param s3MetaData S3-Meta-Data structure representing the instance's metadata
             * @return Boolean indicating whether the metadata was updated/set or not
             */
            bool setMetaData(S3MetaData s3MetaData);
    };
}

#endif //BITQUARK_S3DATASTORE_H
