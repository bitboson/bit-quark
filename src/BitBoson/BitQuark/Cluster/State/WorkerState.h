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

#ifndef BITQUARK_WORKERSTATE_H
#define BITQUARK_WORKERSTATE_H

#include <memory>
#include <BitBoson/StandardModel/Storage/DataStore.h>

using namespace BitBoson;
namespace BitBoson::BitQuark
{

    class WorkerState
    {

        // Private member variables
        private:
            std::shared_ptr<StandardModel::DataStore> _dataStore;

        // Public member functions
        public:

            /**
             * Constructor used to setup the instance
             */
            WorkerState();

            /**
             * Function used to add a resource to the instance state
             *
             * @param resourceGroup String representing the resource group
             * @param resourceId String representing the resource Id
             * @param resourceData String representing the resource data
             * @return Boolean indicating if the resource was added or not
             */
            bool addResource(const std::string& resourceGroup,
                    const std::string& resourceId, const std::string& resourceData);

            /**
             * Function used to get a resource from the instance state
             *
             * @param resourceGroup String representing the resource group
             * @param resourceId String representing the resource Id
             * @return String representing the resource data
             */
            std::string getResource(const std::string& resourceGroup,
                    const std::string& resourceId) const;

            /**
             * Function used to remove a resource from the instance state
             *
             * @param resourceGroup String representing the resource group
             * @param resourceId String representing the resource Id
             * @return Boolean indicating if the resource was removed or not
             */
            bool removeResource(const std::string& resourceGroup,
                    const std::string& resourceId);

            /**
             * Destructor used to cleanup the instance
             */
            virtual ~WorkerState();
    };
}

#endif //BITQUARK_WORKERSTATE_H