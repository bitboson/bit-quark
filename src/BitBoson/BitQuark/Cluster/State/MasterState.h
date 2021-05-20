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

#ifndef BITQUARK_MASTERSTATE_H
#define BITQUARK_MASTERSTATE_H

#include <memory>
#include <unordered_map>
#include <BitBoson/StandardModel/Storage/DataStore.h>
#include <BitBoson/StandardModel/Primitives/Generator.hpp>

using namespace BitBoson;
namespace BitBoson::BitQuark
{

    class MasterState
    {

        // Private member variables
        private:
            std::shared_ptr<StandardModel::DataStore> _dataStore;
            std::unordered_map<std::string, long> _workerRef;

        // Public member functions
        public:

            /**
             * Constructor used to setup the instance
             */
            MasterState();

            /**
             * Function used to add a worker to the master-state instance
             *
             * @param workerId String representing the worker Id to add
             * @return Boolean indicating whether the worker was added
             */
            bool addWorker(const std::string& workerId);

            /**
             * Function used to remove a worker from the master-state instance
             *
             * @param workerId String representing the worker Id to remove
             * @return Boolean indicating whether the worker was removed
             */
            bool removeWorker(const std::string& workerId);

            /**
             * Function used to list the existing workers in the state
             *
             * @return Generator of Strings representing the worker Ids
             */
            std::shared_ptr<StandardModel::Generator<std::string>> listWorkers() const;

            /**
             * Function used to add an association for a worker and resource group
             *
             * @param workerId String representing the worker Id to use in the association
             * @param resourceGroup String representing the resource group to associate
             * @return Boolean indicating whether the association was successfully made
             */
            bool addAssociation(const std::string& workerId,
                    const std::string& resourceGroup);

            /**
             * Function used to remove an association for a worker and resource group
             *
             * @param workerId String representing the worker Id to use in the removal
             * @param resourceGroup String representing the resource group to dissociate
             * @return Boolean indicating whether the association was successfully removed
             */
            bool removeAssociation(const std::string& workerId,
                    const std::string& resourceGroup);

            /**
             * Function used to get the workers for the given resource group
             *
             * @param resourceGroup String representing the group to get the worker for
             * @return Generator of Strings representing the associated worker Ids
             */
            std::shared_ptr<StandardModel::Generator<std::string>> getWorkersForResourceGroup(
                    const std::string& resourceGroup) const;

            /**
             * Function used to get the resource groups for the given worker
             *
             * @param workerId String representing the worker to get the groups for
             * @return Generator of Strings representing the associated groups
             */
            std::shared_ptr<StandardModel::Generator<std::string>> getResourceGroupsForWorker(
                    const std::string& workerId) const;

            /**
             * Destructor used to cleanup the instance
             */
            virtual ~MasterState();

        // Private member functions
        private:

            /**
             * Function used to check if the given worker exists or noe
             *
             * @param workerId String representing the worker Id to check
             * @return Boolean indicating whether the worker exists or not
             */
            bool doesWorkerExistLocally(const std::string& workerId) const;
    };
}

#endif //BITQUARK_MASTERSTATE_H