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

#ifndef BITQUARK_WORKERSTATE_TEST_HPP
#define BITQUARK_WORKERSTATE_TEST_HPP

#include <BitBoson/BitQuark/Cluster/State/WorkerState.h>

using namespace BitBoson::BitQuark;

TEST_CASE ("Generic Worker State Test", "[WorkerStateTest]")
{

    // Create a Worker State instance to test
    auto workerState = std::make_shared<WorkerState>();

    // Add some resources to the instance
    REQUIRE (workerState->addResource("Group1", "Resource1", "Howdy Y'all!"));
    REQUIRE (workerState->addResource("Group2", "Resource1", "Ope!"));
    REQUIRE (workerState->addResource("Group2", "Resource2", "Let me squeeze on by ya"));
    REQUIRE (workerState->addResource("Group2", "Resource3", "You're Fine"));

    // Verify that the resources exist as expected
    REQUIRE (workerState->getResource("Group1", "Resource1") == "Howdy Y'all!");
    REQUIRE (workerState->getResource("Group2", "Resource1") == "Ope!");
    REQUIRE (workerState->getResource("Group2", "Resource2") == "Let me squeeze on by ya");
    REQUIRE (workerState->getResource("Group2", "Resource3") == "You're Fine");

    // Override one of the values and verify that it has changed
    // and that the others are unaffected
    REQUIRE (workerState->addResource("Group2", "Resource1", "You becha!"));
    REQUIRE (workerState->getResource("Group1", "Resource1") == "Howdy Y'all!");
    REQUIRE (workerState->getResource("Group2", "Resource1") == "You becha!");
    REQUIRE (workerState->getResource("Group2", "Resource2") == "Let me squeeze on by ya");
    REQUIRE (workerState->getResource("Group2", "Resource3") == "You're Fine");

    // Remove a resource and verify that it is gone
    REQUIRE (workerState->removeResource("Group2", "Resource1"));
    REQUIRE (workerState->getResource("Group2", "Resource1").empty());

    // Verify we can't remove a resource which doesn't exist
    // or has already been removed
    REQUIRE (!workerState->removeResource("Group2", "Resource1"));
}

#endif //BITQUARK_WORKERSTATE_TEST_HPP
