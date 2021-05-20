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

#ifndef BITQUARK_RESOURCE_H
#define BITQUARK_RESOURCE_H

#include <string>
#include <vector>
#include <BitBoson/StandardModel/DataStructures/Cacheable.hpp>

using namespace BitBoson;
namespace BitBoson::BitQuark
{

    class Resource : public StandardModel::Cacheable
    {

        // Public member classes
        public:
            class ResourceCost
            {

                // Protected member variables
                protected:
                    long _resourceSize;
                    long _memorySize;
                    int _threadCount;

                // Public member functions
                public:

                    /**
                     * Constructor used to setup the instance
                     */
                    ResourceCost();

                    /**
                     * Constructor used to setup the instance with values
                     *
                     * @param resourceSize Long representing the resource size
                     * @param memorySize Long representing the resource size
                     * @param threadCount Integer representing the resource size
                     */
                    ResourceCost(long resourceSize, long memorySize, int threadCount);

                    /**
                     * Function used to set the resource's size (in bytes)
                     *
                     * @param size Long representing the size of the resource
                     */
                    void setResourceSize(long size);

                    /**
                     * Function used to get the resource's size (in bytes)
                     *
                     * @return long representing the size of the resource
                     */
                    long getResourceSize() const;

                    /**
                     * Function used to set the resource's memory requirements (in bytes)
                     *
                     * @param size Long representing the size of the resource's memory requirements
                     */
                    void setMemoryRequirements(long size);

                    /**
                     * Function used to get the resource's memory requirements (in bytes)
                     *
                     * @return long representing the size of the resource's memory requirements
                     */
                    long getMemoryRequirements() const;

                    /**
                     * Function used to set the resource's thread-count requirements
                     *
                     * @param count Integer representing the number of threads required by the resource
                     */
                    void setResourceThreads(int count);

                    /**
                     * Function used to get the resource's thread-count requirements
                     *
                     * @return integer representing the number of threads required by the resource
                     */
                    int getResourceThreads() const;

                    /**
                     * Destructor used to cleanup the instance
                     */
                    virtual ~ResourceCost() = default;
            };

        // Public member functions
        public:

            /**
             * Virtual function used to get the resource cost for the resource instance
             *
             * @return ResourceCost object representing the instance's resource cost
             */
            virtual ResourceCost getResourceCost() const = 0;

            /**
             * Virtual function used to get the packed-vector for the resource instance
             *
             * @return Vector of Strings representing the instance's packed vector
             */
            virtual std::vector<std::string> getPackedVector() const = 0;

            /**
             * Virtual function used to set the packed-vector for the resource instance
             *
             * @param packedVect Vector of Strings representing the instance's packed vector
             */
            virtual bool setPackedVector(const std::vector<std::string>& packedVect) = 0;

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
    };
}

#endif //BITQUARK_RESOURCE_H