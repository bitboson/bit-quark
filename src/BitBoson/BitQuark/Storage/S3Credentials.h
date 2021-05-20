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

#ifndef BITQUARK_S3CREDENTIALS_H
#define BITQUARK_S3CREDENTIALS_H

#include <string>
#include <BitBoson/StandardModel/DataStructures/Cacheable.hpp>

using namespace BitBoson;
namespace BitBoson::BitQuark
{

    class S3Credentials : public StandardModel::Cacheable
    {

        // Private member variables
        private:
            std::string _accessKey;
            std::string _secretKey;
            std::string _bucket;
            std::string _endpoint;
            std::string _dirPrefix;

        // Public member functions
        public:

            /**
             * Constructor used to initialize the credential object's instance
             *
             * @param s3Endpoint String representing the complete S3-Endpoint
             * @param bucket String representing the bucket name to use
             * @param directoryPrefix String representing the directory prefix to use
             * @param accessKey String representing the access key for the bucket
             * @param secretKey String representing the secret key for the bucket
             */
            S3Credentials(const std::string& s3Endpoint, const std::string& bucket,
                          const std::string& directoryPrefix="",
                          const std::string& accessKey="", const std::string& secretKey="");

            /**
             * Constructor used to setup the credential object from the given file-string
             *
             * @param fileString String representing the file-string to create the instance from
             */
            explicit S3Credentials(const std::string& fileString);

            /**
             * Function used to get the S3-Endpoint key from the object
             *
             * @return String representing the S3-Endpoint
             */
            std::string getS3Endpoint();

            /**
             * Function used to get the S3-Bucket key from the object
             *
             * @return String representing the S3-Bucket
             */
            std::string getBucket();

            /**
             * Function used to get the S3-Bucket directory/key prefix from the object
             *
             * @return String representing the directory/key prefix
             */
            std::string getDirectoryPrefix();

            /**
             * Function used to get the access key from the object
             *
             * @return String representing the access key
             */
            std::string getAccessKey();

            /**
             * Function used to get the secret key from the object
             *
             * @return String representing the secret key
             */
            std::string getSecretKey();

            /**
             * Overridden function used to get the file-string-representation of the object
             *
             * @return String representing the cache-able data for the object
             */
            std::string getFileString() const override;

            /**
             * Overridden function used to setup the object based on the given file-string-representation
             *
             * @param fileString String representing the cache-able data for the object to create from
             * @return Boolean indicating whether the given string was parsed and processed properly
             */
            bool setFileString(const std::string& fileString) override;

            /**
             * Overridden function used to get the unique hash (SHA256) of the underlying object
             *
             * @return String representing the unique (SHA256) hash of the underlying object
             */
            std::string getUniqueHash() const override;

            /**
             * Constructor used to cleanup the instance
             */
            virtual ~S3Credentials() = default;

        // Private member functions
        private:

            /**
             * Internal function used to setup default values for the instance
             */
            void setupDefaults();
    };
}

#endif //BITQUARK_S3CREDENTIALS_H
