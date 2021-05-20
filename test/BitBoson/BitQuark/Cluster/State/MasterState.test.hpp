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

#ifndef BITQUARK_MASTERSTATE_TEST_HPP
#define BITQUARK_MASTERSTATE_TEST_HPP

#include <BitBoson/BitQuark/Cluster/State/MasterState.h>

using namespace BitBoson::BitQuark;

TEST_CASE ("Add/Remove Workers to Master State Test", "[MasterStateTest]")
{

    // Create a Master State object for testing
    auto masterState = std::make_shared<MasterState>();

    // Verify no workers exist yet
    auto workers = masterState->listWorkers();
    REQUIRE (!workers->hasMoreItems());

    // Add some workers to the state
    REQUIRE (masterState->addWorker("Worker1"));
    REQUIRE (masterState->addWorker("Worker2"));
    REQUIRE (masterState->addWorker("Worker3"));

    // Verify that the workers exist
    workers = masterState->listWorkers();
    REQUIRE (workers->hasMoreItems());
    REQUIRE (workers->getNextItem() == "Worker1");
    REQUIRE (workers->hasMoreItems());
    REQUIRE (workers->getNextItem() == "Worker3");
    REQUIRE (workers->hasMoreItems());
    REQUIRE (workers->getNextItem() == "Worker2");
    REQUIRE (!workers->hasMoreItems());

    // Remove a single worker and validate
    // the other two still exist
    REQUIRE (masterState->removeWorker("Worker2"));
    workers = masterState->listWorkers();
    REQUIRE (workers->hasMoreItems());
    REQUIRE (workers->getNextItem() == "Worker1");
    REQUIRE (workers->hasMoreItems());
    REQUIRE (workers->getNextItem() == "Worker3");
    REQUIRE (!workers->hasMoreItems());
}

TEST_CASE ("Add/Remove Resource Associations to Master State Test", "[MasterStateTest]")
{

    // Create a Master State object for testing
    auto masterState = std::make_shared<MasterState>();

    // Verify no workers exist yet
    auto workers = masterState->listWorkers();
    REQUIRE (!workers->hasMoreItems());

    // Add some workers to the state
    REQUIRE (masterState->addWorker("Worker1"));
    REQUIRE (masterState->addWorker("Worker2"));
    REQUIRE (masterState->addWorker("Worker3"));

    // Add some associations to the first worker
    REQUIRE (masterState->addAssociation("Worker1", "Resource1"));
    REQUIRE (masterState->addAssociation("Worker1", "Resource2"));
    REQUIRE (masterState->addAssociation("Worker1", "Resource3"));

    // Now add some resources to the second worker
    REQUIRE (masterState->addAssociation("Worker2", "Resource1"));
    REQUIRE (masterState->addAssociation("Worker2", "Resource2"));
    REQUIRE (masterState->addAssociation("Worker2", "Resource4"));
    REQUIRE (masterState->addAssociation("Worker2", "Resource5"));

    // Finally, add a resource to the third worker
    REQUIRE (masterState->addAssociation("Worker3", "Resource2"));

    // Verify that the all three workers cannot be deleted
    // since they have active resources associated with them
    REQUIRE (!masterState->removeWorker("Worker1"));
    REQUIRE (!masterState->removeWorker("Worker2"));
    REQUIRE (!masterState->removeWorker("Worker3"));
    workers = masterState->listWorkers();
    REQUIRE (workers->hasMoreItems());
    REQUIRE (workers->getNextItem() == "Worker1");
    REQUIRE (workers->hasMoreItems());
    REQUIRE (workers->getNextItem() == "Worker3");
    REQUIRE (workers->hasMoreItems());
    REQUIRE (workers->getNextItem() == "Worker2");
    REQUIRE (!workers->hasMoreItems());

    // Verify that the first worker has the associated resources
    auto resources = masterState->getResourceGroupsForWorker("Worker1");
    REQUIRE (resources->hasMoreItems());
    REQUIRE (resources->getNextItem() == "Resource1");
    REQUIRE (resources->hasMoreItems());
    REQUIRE (resources->getNextItem() == "Resource2");
    REQUIRE (resources->hasMoreItems());
    REQUIRE (resources->getNextItem() == "Resource3");
    REQUIRE (!resources->hasMoreItems());

    // Verify that the second worker has the associated resources
    resources = masterState->getResourceGroupsForWorker("Worker2");
    REQUIRE (resources->hasMoreItems());
    REQUIRE (resources->getNextItem() == "Resource1");
    REQUIRE (resources->hasMoreItems());
    REQUIRE (resources->getNextItem() == "Resource2");
    REQUIRE (resources->hasMoreItems());
    REQUIRE (resources->getNextItem() == "Resource4");
    REQUIRE (resources->hasMoreItems());
    REQUIRE (resources->getNextItem() == "Resource5");
    REQUIRE (!resources->hasMoreItems());

    // Verify that the third worker has the associated resource
    resources = masterState->getResourceGroupsForWorker("Worker3");
    REQUIRE (resources->hasMoreItems());
    REQUIRE (resources->getNextItem() == "Resource2");
    REQUIRE (!resources->hasMoreItems());

    // Verify the first resource is associated with the correct workers
    workers = masterState->getWorkersForResourceGroup("Resource1");
    REQUIRE (workers->hasMoreItems());
    REQUIRE (workers->getNextItem() == "Worker1");
    REQUIRE (workers->hasMoreItems());
    REQUIRE (workers->getNextItem() == "Worker2");
    REQUIRE (!workers->hasMoreItems());

    // Verify the second resource is associated with the correct workers
    workers = masterState->getWorkersForResourceGroup("Resource2");
    REQUIRE (workers->hasMoreItems());
    REQUIRE (workers->getNextItem() == "Worker1");
    REQUIRE (workers->hasMoreItems());
    REQUIRE (workers->getNextItem() == "Worker2");
    REQUIRE (workers->hasMoreItems());
    REQUIRE (workers->getNextItem() == "Worker3");
    REQUIRE (!workers->hasMoreItems());

    // Verify the third resource is associated with the correct worker
    workers = masterState->getWorkersForResourceGroup("Resource3");
    REQUIRE (workers->hasMoreItems());
    REQUIRE (workers->getNextItem() == "Worker1");
    REQUIRE (!workers->hasMoreItems());

    // Verify the fourth resource is associated with the correct worker
    workers = masterState->getWorkersForResourceGroup("Resource4");
    REQUIRE (workers->hasMoreItems());
    REQUIRE (workers->getNextItem() == "Worker2");
    REQUIRE (!workers->hasMoreItems());

    // Verify the fifth resource is associated with the correct worker
    workers = masterState->getWorkersForResourceGroup("Resource5");
    REQUIRE (workers->hasMoreItems());
    REQUIRE (workers->getNextItem() == "Worker2");
    REQUIRE (!workers->hasMoreItems());

    // Attempt to add various associations which already exist
    REQUIRE (!masterState->addAssociation("Worker1", "Resource1"));
    REQUIRE (!masterState->addAssociation("Worker1", "Resource2"));
    REQUIRE (!masterState->addAssociation("Worker1", "Resource3"));
    REQUIRE (!masterState->addAssociation("Worker2", "Resource1"));
    REQUIRE (!masterState->addAssociation("Worker2", "Resource2"));
    REQUIRE (!masterState->addAssociation("Worker2", "Resource4"));
    REQUIRE (!masterState->addAssociation("Worker2", "Resource5"));
    REQUIRE (!masterState->addAssociation("Worker3", "Resource2"));

    // Now, attempt to remove various associations which don't exist
    REQUIRE (!masterState->removeAssociation("Worker4", "Resource2"));
    REQUIRE (!masterState->removeAssociation("Worker1", "Resource6"));
    REQUIRE (!masterState->removeAssociation("Worker1", "Resource4"));
    REQUIRE (!masterState->removeAssociation("Worker3", "Resource1"));

    // Remove all associations for worker three and then remove the worker
    REQUIRE (masterState->removeAssociation("Worker3", "Resource2"));
    REQUIRE (masterState->removeWorker("Worker3"));
    resources = masterState->getResourceGroupsForWorker("Worker3");
    REQUIRE (!resources->hasMoreItems());

    // Verify the second resource is associated with the correct workers
    workers = masterState->getWorkersForResourceGroup("Resource2");
    REQUIRE (workers->hasMoreItems());
    REQUIRE (workers->getNextItem() == "Worker1");
    REQUIRE (workers->hasMoreItems());
    REQUIRE (workers->getNextItem() == "Worker2");
    REQUIRE (!workers->hasMoreItems());

    // Remove all associations for worker two and then remove the worker
    REQUIRE (masterState->removeAssociation("Worker2", "Resource1"));
    REQUIRE (masterState->removeAssociation("Worker2", "Resource2"));
    REQUIRE (masterState->removeAssociation("Worker2", "Resource4"));
    REQUIRE (masterState->removeAssociation("Worker2", "Resource5"));
    REQUIRE (masterState->removeWorker("Worker2"));
    resources = masterState->getResourceGroupsForWorker("Worker2");
    REQUIRE (!resources->hasMoreItems());

    // Verify the second resource is associated with the correct workers
    workers = masterState->getWorkersForResourceGroup("Resource2");
    REQUIRE (workers->hasMoreItems());
    REQUIRE (workers->getNextItem() == "Worker1");
    REQUIRE (!workers->hasMoreItems());

    // Verify the fourth resource is associated with nothing
    workers = masterState->getWorkersForResourceGroup("Resource4");
    REQUIRE (!workers->hasMoreItems());

    // Remove all associations for worker two and then remove the worker
    REQUIRE (masterState->removeAssociation("Worker1", "Resource1"));
    REQUIRE (masterState->removeAssociation("Worker1", "Resource2"));
    REQUIRE (masterState->removeAssociation("Worker1", "Resource3"));
    REQUIRE (masterState->removeWorker("Worker1"));
    resources = masterState->getResourceGroupsForWorker("Worker1");
    REQUIRE (!resources->hasMoreItems());

    // Verify the first resource is associated with nothing
    workers = masterState->getWorkersForResourceGroup("Resource1");
    REQUIRE (!workers->hasMoreItems());

    // Verify the second resource is associated with nothing
    workers = masterState->getWorkersForResourceGroup("Resource2");
    REQUIRE (!workers->hasMoreItems());

    // Verify the third resource is associated with nothing
    workers = masterState->getWorkersForResourceGroup("Resource3");
    REQUIRE (!workers->hasMoreItems());
}

#endif //BITQUARK_MASTERSTATE_TEST_HPP
