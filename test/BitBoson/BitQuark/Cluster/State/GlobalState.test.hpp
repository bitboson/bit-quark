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

#ifndef BITQUARK_GLOBALSTATE_TEST_HPP
#define BITQUARK_GLOBALSTATE_TEST_HPP

#include <BitBoson/BitQuark/Cluster/State/GlobalState.h>

using namespace BitBoson::BitQuark;

class DummyStringResource : public Resource
{
    private:
        std::string _data;
    public:
        DummyStringResource(std::string data="") { _data = data; }
        ResourceCost getResourceCost() const override {
            auto cost = ResourceCost();
            cost.setResourceSize(getFileString().size());
            cost.setMemoryRequirements(_data.size());
            cost.setResourceThreads(1); return cost; }
        std::vector<std::string> getPackedVector() const override { return {_data}; }
        bool setPackedVector(const std::vector<std::string>& pV) override {
            _data = pV[0]; return true; }
        std::string getDataValue() const { return _data; }
        DummyStringResource* setFileStringHelper(const std::string& fileString)
            { setFileString(fileString); return this; }
        virtual ~DummyStringResource() = default;
};

/**
 * Test function used to get the testing S3-Credentials
 *
 * @param dirPrefix String representing the directory/key prefix for the bucket
 * @param badKeys Boolean indicating whether the s3 access keys should be invalid
 * @return S3 Credentials reference to use for testing
 */
std::shared_ptr<S3Credentials> getTestGlobalStateCredentials(const std::string& dirPrefix="",
        bool badKeys=false)
{
    std::string secretKey = (badKeys ? "ThisIsABadSecretKey" : "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY");
    return std::make_shared<S3Credentials>("localhost:9000", "test-bucket", dirPrefix,
            "AKIAIOSFODNN7EXAMPLE", secretKey);
}

TEST_CASE ("Add and List Resource Groups Global State Test", "[GlobalStateTest]")
{

    // Create a global state object to use for testing
    auto credentials = getTestGlobalStateCredentials("GlobalStateTest");
    auto globalState = std::make_shared<GlobalState>(credentials, GlobalState::Mode::READ_WRITE);

    // Ensure that the global state is empty
    REQUIRE (globalState->clearEntireState());

    // List the current resource groups to ensure there are none
    auto resourceGroups = globalState->listResourceGroups();
    REQUIRE (!resourceGroups->hasMoreItems());

    // Add a single resource group and ensure that it exists
    REQUIRE (globalState->addResourceGroup("def456"));
    resourceGroups = globalState->listResourceGroups();
    REQUIRE (resourceGroups->hasMoreItems());
    REQUIRE (resourceGroups->getNextItem() == "def456");
    REQUIRE (!resourceGroups->hasMoreItems());

    // Add a few more and ensure they exist in alphabetical order
    REQUIRE (globalState->addResourceGroup("abc123"));
    REQUIRE (globalState->addResourceGroup("ghi789"));
    resourceGroups = globalState->listResourceGroups();
    REQUIRE (resourceGroups->hasMoreItems());
    REQUIRE (resourceGroups->getNextItem() == "abc123");
    REQUIRE (resourceGroups->hasMoreItems());
    REQUIRE (resourceGroups->getNextItem() == "def456");
    REQUIRE (resourceGroups->hasMoreItems());
    REQUIRE (resourceGroups->getNextItem() == "ghi789");
    REQUIRE (!resourceGroups->hasMoreItems());

    // Cleanup the global-state when we are finished
    REQUIRE (globalState->clearEntireState());
}

TEST_CASE ("Add/Get/Remove Items to/from a Resource Group Global State Test", "[GlobalStateTest]")
{

    // Create a global state object to use for testing
    auto credentials = getTestGlobalStateCredentials("GlobalStateTest");
    auto globalState = std::make_shared<GlobalState>(credentials, GlobalState::Mode::READ_WRITE);

    // Ensure that the global state is empty
    REQUIRE (globalState->clearEntireState());

    // Add some resource groups and ensure they're all empty
    REQUIRE (globalState->addResourceGroup("abc123"));
    REQUIRE (globalState->addResourceGroup("def456"));
    REQUIRE (globalState->addResourceGroup("ghi789"));
    auto resourceGroupItems = globalState->listResourcesInGroup("abc123");
    REQUIRE (!resourceGroupItems->hasMoreItems());
    resourceGroupItems = globalState->listResourcesInGroup("def456");
    REQUIRE (!resourceGroupItems->hasMoreItems());
    resourceGroupItems = globalState->listResourcesInGroup("ghi789");
    REQUIRE (!resourceGroupItems->hasMoreItems());

    // Validate that all resource groups have no cost
    REQUIRE (globalState->getResourceGroupCost("abc123").getResourceSize() == 0);
    REQUIRE (globalState->getResourceGroupCost("abc123").getMemoryRequirements() == 0);
    REQUIRE (globalState->getResourceGroupCost("abc123").getResourceThreads() == 0);
    REQUIRE (globalState->getResourceGroupCost("def456").getResourceSize() == 0);
    REQUIRE (globalState->getResourceGroupCost("def456").getMemoryRequirements() == 0);
    REQUIRE (globalState->getResourceGroupCost("def456").getResourceThreads() == 0);
    REQUIRE (globalState->getResourceGroupCost("ghi789").getResourceSize() == 0);
    REQUIRE (globalState->getResourceGroupCost("ghi789").getMemoryRequirements() == 0);
    REQUIRE (globalState->getResourceGroupCost("ghi789").getResourceThreads() == 0);

    // Add an item into one of the resource groups
    REQUIRE (globalState->setResourceInGroup("def456", "eeeeeee", std::make_shared<DummyStringResource>("Howdy Y'all!")));

    // Verify that the resource is in the group
    resourceGroupItems = globalState->listResourcesInGroup("def456");
    REQUIRE (resourceGroupItems->hasMoreItems());
    REQUIRE (resourceGroupItems->getNextItem() == "eeeeeee");
    REQUIRE (!resourceGroupItems->hasMoreItems());

    // Ensure that the other groups are empty
    resourceGroupItems = globalState->listResourcesInGroup("abc123");
    REQUIRE (!resourceGroupItems->hasMoreItems());
    resourceGroupItems = globalState->listResourcesInGroup("ghi789");
    REQUIRE (!resourceGroupItems->hasMoreItems());

    // Validate the new expected per-group costs for the global state
    REQUIRE (globalState->getResourceGroupCost("abc123").getResourceSize() == 0);
    REQUIRE (globalState->getResourceGroupCost("abc123").getMemoryRequirements() == 0);
    REQUIRE (globalState->getResourceGroupCost("abc123").getResourceThreads() == 0);
    REQUIRE (globalState->getResourceGroupCost("def456").getResourceSize() == 28);
    REQUIRE (globalState->getResourceGroupCost("def456").getMemoryRequirements() == 12);
    REQUIRE (globalState->getResourceGroupCost("def456").getResourceThreads() == 1);
    REQUIRE (globalState->getResourceGroupCost("ghi789").getResourceSize() == 0);
    REQUIRE (globalState->getResourceGroupCost("ghi789").getMemoryRequirements() == 0);
    REQUIRE (globalState->getResourceGroupCost("ghi789").getResourceThreads() == 0);

    // Validate the size of the known resources
    REQUIRE (globalState->getResourceInGroupCost("def456", "eeeeeee").getResourceSize() == 28);
    REQUIRE (globalState->getResourceInGroupCost("def456", "eeeeeee").getMemoryRequirements() == 12);
    REQUIRE (globalState->getResourceInGroupCost("def456", "eeeeeee").getResourceThreads() == 1);

    // Add a few items into another resource group in non-alphabetical order
    REQUIRE (globalState->setResourceInGroup("abc123", "eeeeeee", std::make_shared<DummyStringResource>("Ope!")));
    REQUIRE (globalState->setResourceInGroup("abc123", "zzzzzzz", std::make_shared<DummyStringResource>("Let me squeeze on by ya")));
    REQUIRE (globalState->setResourceInGroup("abc123", "aaaaaaa", std::make_shared<DummyStringResource>("You're Fine")));

    // Validate that the first resource group we added to still has its data
    resourceGroupItems = globalState->listResourcesInGroup("def456");
    REQUIRE (resourceGroupItems->hasMoreItems());
    REQUIRE (resourceGroupItems->getNextItem() == "eeeeeee");
    REQUIRE (!resourceGroupItems->hasMoreItems());

    // Validate that the new items we added are in their resource group
    // and that the items are in alphabetical order now too
    resourceGroupItems = globalState->listResourcesInGroup("abc123");
    REQUIRE (resourceGroupItems->hasMoreItems());
    REQUIRE (resourceGroupItems->getNextItem() == "aaaaaaa");
    REQUIRE (resourceGroupItems->hasMoreItems());
    REQUIRE (resourceGroupItems->getNextItem() == "eeeeeee");
    REQUIRE (resourceGroupItems->hasMoreItems());
    REQUIRE (resourceGroupItems->getNextItem() == "zzzzzzz");
    REQUIRE (!resourceGroupItems->hasMoreItems());

    // Validate that the other resource group is still empty
    resourceGroupItems = globalState->listResourcesInGroup("ghi789");
    REQUIRE (!resourceGroupItems->hasMoreItems());

    // Validate the new expected per-group costs for the global state
    REQUIRE (globalState->getResourceGroupCost("abc123").getResourceSize() == 86);
    REQUIRE (globalState->getResourceGroupCost("abc123").getMemoryRequirements() == 38);
    REQUIRE (globalState->getResourceGroupCost("abc123").getResourceThreads() == 3);
    REQUIRE (globalState->getResourceGroupCost("def456").getResourceSize() == 28);
    REQUIRE (globalState->getResourceGroupCost("def456").getMemoryRequirements() == 12);
    REQUIRE (globalState->getResourceGroupCost("def456").getResourceThreads() == 1);
    REQUIRE (globalState->getResourceGroupCost("ghi789").getResourceSize() == 0);
    REQUIRE (globalState->getResourceGroupCost("ghi789").getMemoryRequirements() == 0);
    REQUIRE (globalState->getResourceGroupCost("ghi789").getResourceThreads() == 0);

    // Validate the size of the known resources
    REQUIRE (globalState->getResourceInGroupCost("abc123", "aaaaaaa").getResourceSize() == 27);
    REQUIRE (globalState->getResourceInGroupCost("abc123", "aaaaaaa").getMemoryRequirements() == 11);
    REQUIRE (globalState->getResourceInGroupCost("abc123", "aaaaaaa").getResourceThreads() == 1);
    REQUIRE (globalState->getResourceInGroupCost("abc123", "eeeeeee").getResourceSize() == 20);
    REQUIRE (globalState->getResourceInGroupCost("abc123", "eeeeeee").getMemoryRequirements() == 4);
    REQUIRE (globalState->getResourceInGroupCost("abc123", "eeeeeee").getResourceThreads() == 1);
    REQUIRE (globalState->getResourceInGroupCost("abc123", "zzzzzzz").getResourceSize() == 39);
    REQUIRE (globalState->getResourceInGroupCost("abc123", "zzzzzzz").getMemoryRequirements() == 23);
    REQUIRE (globalState->getResourceInGroupCost("abc123", "zzzzzzz").getResourceThreads() == 1);
    REQUIRE (globalState->getResourceInGroupCost("def456", "eeeeeee").getResourceSize() == 28);
    REQUIRE (globalState->getResourceInGroupCost("def456", "eeeeeee").getMemoryRequirements() == 12);
    REQUIRE (globalState->getResourceInGroupCost("def456", "eeeeeee").getResourceThreads() == 1);

    // Get all of the known items in the different resource groups
    REQUIRE (DummyStringResource().setFileStringHelper(globalState->getResourceInGroup("def456", "eeeeeee"))->getDataValue() == "Howdy Y'all!");
    REQUIRE (DummyStringResource().setFileStringHelper(globalState->getResourceInGroup("abc123", "eeeeeee"))->getDataValue() == "Ope!");
    REQUIRE (DummyStringResource().setFileStringHelper(globalState->getResourceInGroup("abc123", "zzzzzzz"))->getDataValue() == "Let me squeeze on by ya");
    REQUIRE (DummyStringResource().setFileStringHelper(globalState->getResourceInGroup("abc123", "aaaaaaa"))->getDataValue() == "You're Fine");

    // Overwrite one of the values in one of the resource groups
    REQUIRE (globalState->setResourceInGroup("abc123", "aaaaaaa", std::make_shared<DummyStringResource>("You becha!")));

    // Validate the values in the resource group are as expected
    REQUIRE (DummyStringResource().setFileStringHelper(globalState->getResourceInGroup("def456", "eeeeeee"))->getDataValue() == "Howdy Y'all!");
    REQUIRE (DummyStringResource().setFileStringHelper(globalState->getResourceInGroup("abc123", "eeeeeee"))->getDataValue() == "Ope!");
    REQUIRE (DummyStringResource().setFileStringHelper(globalState->getResourceInGroup("abc123", "zzzzzzz"))->getDataValue() == "Let me squeeze on by ya");
    REQUIRE (DummyStringResource().setFileStringHelper(globalState->getResourceInGroup("abc123", "aaaaaaa"))->getDataValue() == "You becha!");

    // Validate the new expected per-group costs for the global state
    REQUIRE (globalState->getResourceGroupCost("abc123").getResourceSize() == 85);
    REQUIRE (globalState->getResourceGroupCost("abc123").getMemoryRequirements() == 37);
    REQUIRE (globalState->getResourceGroupCost("abc123").getResourceThreads() == 3);
    REQUIRE (globalState->getResourceGroupCost("def456").getResourceSize() == 28);
    REQUIRE (globalState->getResourceGroupCost("def456").getMemoryRequirements() == 12);
    REQUIRE (globalState->getResourceGroupCost("def456").getResourceThreads() == 1);
    REQUIRE (globalState->getResourceGroupCost("ghi789").getResourceSize() == 0);
    REQUIRE (globalState->getResourceGroupCost("ghi789").getMemoryRequirements() == 0);
    REQUIRE (globalState->getResourceGroupCost("ghi789").getResourceThreads() == 0);

    // Validate the size of the known resources
    REQUIRE (globalState->getResourceInGroupCost("abc123", "aaaaaaa").getResourceSize() == 26);
    REQUIRE (globalState->getResourceInGroupCost("abc123", "aaaaaaa").getMemoryRequirements() == 10);
    REQUIRE (globalState->getResourceInGroupCost("abc123", "aaaaaaa").getResourceThreads() == 1);
    REQUIRE (globalState->getResourceInGroupCost("abc123", "eeeeeee").getResourceSize() == 20);
    REQUIRE (globalState->getResourceInGroupCost("abc123", "eeeeeee").getMemoryRequirements() == 4);
    REQUIRE (globalState->getResourceInGroupCost("abc123", "eeeeeee").getResourceThreads() == 1);
    REQUIRE (globalState->getResourceInGroupCost("abc123", "zzzzzzz").getResourceSize() == 39);
    REQUIRE (globalState->getResourceInGroupCost("abc123", "zzzzzzz").getMemoryRequirements() == 23);
    REQUIRE (globalState->getResourceInGroupCost("abc123", "zzzzzzz").getResourceThreads() == 1);
    REQUIRE (globalState->getResourceInGroupCost("def456", "eeeeeee").getResourceSize() == 28);
    REQUIRE (globalState->getResourceInGroupCost("def456", "eeeeeee").getMemoryRequirements() == 12);
    REQUIRE (globalState->getResourceInGroupCost("def456", "eeeeeee").getResourceThreads() == 1);

    // Remove a resource from one of the groups
    REQUIRE (globalState->removeResourceInGroup("abc123", "eeeeeee"));

    // Validate the new expected per-group costs for the global state
    REQUIRE (globalState->getResourceGroupCost("abc123").getResourceSize() == 65);
    REQUIRE (globalState->getResourceGroupCost("abc123").getMemoryRequirements() == 33);
    REQUIRE (globalState->getResourceGroupCost("abc123").getResourceThreads() == 2);
    REQUIRE (globalState->getResourceGroupCost("def456").getResourceSize() == 28);
    REQUIRE (globalState->getResourceGroupCost("def456").getMemoryRequirements() == 12);
    REQUIRE (globalState->getResourceGroupCost("def456").getResourceThreads() == 1);
    REQUIRE (globalState->getResourceGroupCost("ghi789").getResourceSize() == 0);
    REQUIRE (globalState->getResourceGroupCost("ghi789").getMemoryRequirements() == 0);
    REQUIRE (globalState->getResourceGroupCost("ghi789").getResourceThreads() == 0);

    // Validate the values in the resource group are as expected
    REQUIRE (DummyStringResource().setFileStringHelper(globalState->getResourceInGroup("def456", "eeeeeee"))->getDataValue() == "Howdy Y'all!");
    REQUIRE (DummyStringResource().setFileStringHelper(globalState->getResourceInGroup("abc123", "zzzzzzz"))->getDataValue() == "Let me squeeze on by ya");
    REQUIRE (DummyStringResource().setFileStringHelper(globalState->getResourceInGroup("abc123", "aaaaaaa"))->getDataValue() == "You becha!");

    // Validate that the removed item is empty
    REQUIRE (DummyStringResource().setFileStringHelper(globalState->getResourceInGroup("abc123", "eeeeeee"))->getDataValue().empty());

    // Attempt to add a item to a non-existent resource group
    REQUIRE (!globalState->setResourceInGroup("xyz000", "eeeeeee", std::make_shared<DummyStringResource>("Ope!")));
    REQUIRE (DummyStringResource().setFileStringHelper(globalState->getResourceInGroup("xyz000", "eeeeeee"))->getDataValue().empty());

    // Validate the new expected per-group costs for the global state
    REQUIRE (globalState->getResourceGroupCost("abc123").getResourceSize() == 65);
    REQUIRE (globalState->getResourceGroupCost("abc123").getMemoryRequirements() == 33);
    REQUIRE (globalState->getResourceGroupCost("abc123").getResourceThreads() == 2);
    REQUIRE (globalState->getResourceGroupCost("def456").getResourceSize() == 28);
    REQUIRE (globalState->getResourceGroupCost("def456").getMemoryRequirements() == 12);
    REQUIRE (globalState->getResourceGroupCost("def456").getResourceThreads() == 1);
    REQUIRE (globalState->getResourceGroupCost("ghi789").getResourceSize() == 0);
    REQUIRE (globalState->getResourceGroupCost("ghi789").getMemoryRequirements() == 0);
    REQUIRE (globalState->getResourceGroupCost("ghi789").getResourceThreads() == 0);

    // Cleanup the global-state when we are finished
    REQUIRE (globalState->clearEntireState());
}

TEST_CASE ("Add and Remove a Resource Group Global State Test", "[GlobalStateTest]")
{

    // Create a global state object to use for testing
    auto credentials = getTestGlobalStateCredentials("GlobalStateTest");
    auto globalState = std::make_shared<GlobalState>(credentials, GlobalState::Mode::READ_WRITE);

    // Ensure that the global state is empty
    REQUIRE (globalState->clearEntireState());

    // Add some resource groups and ensure they're all empty
    REQUIRE (globalState->addResourceGroup("abc123"));
    REQUIRE (globalState->addResourceGroup("def456"));
    REQUIRE (globalState->addResourceGroup("ghi789"));
    auto resourceGroupItems = globalState->listResourcesInGroup("abc123");
    REQUIRE (!resourceGroupItems->hasMoreItems());
    resourceGroupItems = globalState->listResourcesInGroup("def456");
    REQUIRE (!resourceGroupItems->hasMoreItems());
    resourceGroupItems = globalState->listResourcesInGroup("ghi789");
    REQUIRE (!resourceGroupItems->hasMoreItems());

    // Add some data to some of the resource groups
    REQUIRE (globalState->setResourceInGroup("def456", "eeeeeee", std::make_shared<DummyStringResource>("Howdy Y'all!")));
    REQUIRE (globalState->setResourceInGroup("abc123", "eeeeeee", std::make_shared<DummyStringResource>("Ope!")));
    REQUIRE (globalState->setResourceInGroup("abc123", "zzzzzzz", std::make_shared<DummyStringResource>("Let me squeeze on by ya")));
    REQUIRE (globalState->setResourceInGroup("abc123", "aaaaaaa", std::make_shared<DummyStringResource>("You're Fine")));

    // Attempt to remove each resource group ensuring
    // Only the empty one should be removed
    REQUIRE (!globalState->removeResourceGroup("abc123"));
    REQUIRE (!globalState->removeResourceGroup("def456"));
    REQUIRE (globalState->removeResourceGroup("ghi789"));

    // Attempt to add a duplicate resource group
    REQUIRE (!globalState->addResourceGroup("abc123"));

    // Validate that the resource group was unaffected
    REQUIRE (DummyStringResource().setFileStringHelper(globalState->getResourceInGroup("abc123", "eeeeeee"))->getDataValue() == "Ope!");
    REQUIRE (DummyStringResource().setFileStringHelper(globalState->getResourceInGroup("abc123", "zzzzzzz"))->getDataValue() == "Let me squeeze on by ya");
    REQUIRE (DummyStringResource().setFileStringHelper(globalState->getResourceInGroup("abc123", "aaaaaaa"))->getDataValue() == "You're Fine");

    // Validate that one of the resource groups was deleted
    auto resourceGroups = globalState->listResourceGroups();
    REQUIRE (resourceGroups->hasMoreItems());
    REQUIRE (resourceGroups->getNextItem() == "abc123");
    REQUIRE (resourceGroups->hasMoreItems());
    REQUIRE (resourceGroups->getNextItem() == "def456");
    REQUIRE (!resourceGroups->hasMoreItems());

    // Remove all of the items in the first resource group
    REQUIRE (globalState->removeResourceInGroup("abc123", "eeeeeee"));
    REQUIRE (globalState->removeResourceInGroup("abc123", "zzzzzzz"));
    REQUIRE (globalState->removeResourceInGroup("abc123", "aaaaaaa"));

    // Attempt to remove each resource group ensuring
    // Only the empty one should be removed
    REQUIRE (globalState->removeResourceGroup("abc123"));
    REQUIRE (!globalState->removeResourceGroup("def456"));

    // Validate that one of the resource groups was deleted
    resourceGroups = globalState->listResourceGroups();
    REQUIRE (resourceGroups->hasMoreItems());
    REQUIRE (resourceGroups->getNextItem() == "def456");
    REQUIRE (!resourceGroups->hasMoreItems());

    // Validate that the content from the first resource group has been removed
    resourceGroupItems = globalState->listResourcesInGroup("abc123");
    REQUIRE (!resourceGroupItems->hasMoreItems());
    REQUIRE (DummyStringResource().setFileStringHelper(globalState->getResourceInGroup("abc123", "eeeeeee"))->getDataValue().empty());

    // Cleanup the global-state when we are finished
    REQUIRE (globalState->clearEntireState());
}

TEST_CASE ("Add Groups/Resourced to Read-Only Global State Test", "[GlobalStateTest]")
{

    // Create a global state object to use for reference and one for testing
    auto credentials = getTestGlobalStateCredentials("GlobalStateTest");
    auto globalStateW = std::make_shared<GlobalState>(credentials, GlobalState::Mode::READ_WRITE);
    auto globalStateR = std::make_shared<GlobalState>(credentials, GlobalState::Mode::READ_ONLY);

    // Ensure that the global state is empty
    REQUIRE (globalStateW->clearEntireState());

    // Attempt to add a resource group to the read-only instance
    // and verify it doesn't exist
    REQUIRE (!globalStateR->addResourceGroup("abc123"));
    auto resourceGroupItems = globalStateR->listResourceGroups();
    REQUIRE (!resourceGroupItems->hasMoreItems());
    resourceGroupItems = globalStateW->listResourceGroups();
    REQUIRE (!resourceGroupItems->hasMoreItems());

    // Add a resource group to the reference instance
    // and verify it exists now
    REQUIRE (globalStateW->addResourceGroup("abc123"));
    resourceGroupItems = globalStateR->listResourceGroups();
    REQUIRE (resourceGroupItems->hasMoreItems());
    REQUIRE (resourceGroupItems->getNextItem() == "abc123");
    resourceGroupItems = globalStateW->listResourceGroups();
    REQUIRE (resourceGroupItems->hasMoreItems());
    REQUIRE (resourceGroupItems->getNextItem() == "abc123");

    // Attempt to add  a resource group to the read-only instance
    // and verify it doesn't exist
    REQUIRE (!globalStateR->setResourceInGroup("abc123", "eeeeeee", std::make_shared<DummyStringResource>("Ope!")));
    REQUIRE (DummyStringResource().setFileStringHelper(globalStateR->getResourceInGroup("abc123", "eeeeeee"))->getDataValue().empty());
    REQUIRE (DummyStringResource().setFileStringHelper(globalStateW->getResourceInGroup("abc123", "eeeeeee"))->getDataValue().empty());

    // Add a resource group to the reference instance
    // and verify it exists now
    REQUIRE (globalStateW->setResourceInGroup("abc123", "eeeeeee", std::make_shared<DummyStringResource>("Ope!")));
    REQUIRE (DummyStringResource().setFileStringHelper(globalStateR->getResourceInGroup("abc123", "eeeeeee"))->getDataValue() == "Ope!");
    REQUIRE (DummyStringResource().setFileStringHelper(globalStateW->getResourceInGroup("abc123", "eeeeeee"))->getDataValue() == "Ope!");

    // Attempt to clear-out all of the state in the read-only instance
    // and verify that this hasn't happened
    REQUIRE (!globalStateR->clearEntireState());
    REQUIRE (DummyStringResource().setFileStringHelper(globalStateR->getResourceInGroup("abc123", "eeeeeee"))->getDataValue() == "Ope!");
    REQUIRE (DummyStringResource().setFileStringHelper(globalStateW->getResourceInGroup("abc123", "eeeeeee"))->getDataValue() == "Ope!");

    // Attempt to remove a resource from a group in the read-only instance
    // and verify it hasn't been removed
    REQUIRE (!globalStateR->removeResourceInGroup("abc123", "eeeeeee"));
    REQUIRE (DummyStringResource().setFileStringHelper(globalStateR->getResourceInGroup("abc123", "eeeeeee"))->getDataValue() == "Ope!");
    REQUIRE (DummyStringResource().setFileStringHelper(globalStateW->getResourceInGroup("abc123", "eeeeeee"))->getDataValue() == "Ope!");

    // Remove a resource from a group in the reference instance
    // and verify it has been removed
    REQUIRE (globalStateW->removeResourceInGroup("abc123", "eeeeeee"));
    REQUIRE (DummyStringResource().setFileStringHelper(globalStateR->getResourceInGroup("abc123", "eeeeeee"))->getDataValue().empty());
    REQUIRE (DummyStringResource().setFileStringHelper(globalStateW->getResourceInGroup("abc123", "eeeeeee"))->getDataValue().empty());

    // Attempt to remove a resource group in the read-only instance
    // and verify it hasn't been removed
    REQUIRE (!globalStateR->removeResourceGroup("abc123"));
    resourceGroupItems = globalStateR->listResourceGroups();
    REQUIRE (resourceGroupItems->hasMoreItems());
    REQUIRE (resourceGroupItems->getNextItem() == "abc123");
    resourceGroupItems = globalStateW->listResourceGroups();
    REQUIRE (resourceGroupItems->hasMoreItems());
    REQUIRE (resourceGroupItems->getNextItem() == "abc123");

    // Remove a resource group in the reference instance
    // and verify it has been removed
    REQUIRE (globalStateW->removeResourceGroup("abc123"));
    resourceGroupItems = globalStateR->listResourceGroups();
    REQUIRE (!resourceGroupItems->hasMoreItems());
    resourceGroupItems = globalStateW->listResourceGroups();
    REQUIRE (!resourceGroupItems->hasMoreItems());

    // Cleanup the global-state when we are finished
    REQUIRE (globalStateW->clearEntireState());
}

TEST_CASE ("Assign and Un-Assign Resource Groups Global State Test", "[GlobalStateTest]")
{

    // Create a global state object to use for testing
    auto credentials = getTestGlobalStateCredentials("GlobalStateTest");
    auto globalState = std::make_shared<GlobalState>(credentials, GlobalState::Mode::READ_WRITE);

    // Ensure that the global state is empty
    REQUIRE (globalState->clearEntireState());

    // List the current resource groups to ensure there are none
    auto resourceGroups = globalState->listResourceGroups();
    REQUIRE (!resourceGroups->hasMoreItems());

    // Add three resource groups to the global state
    REQUIRE (globalState->addResourceGroup("def456"));
    REQUIRE (globalState->addResourceGroup("abc123"));
    REQUIRE (globalState->addResourceGroup("ghi789"));

    // Verify that the resource groups exist in the global state
    resourceGroups = globalState->listResourceGroups();
    REQUIRE (resourceGroups->hasMoreItems());
    REQUIRE (resourceGroups->getNextItem() == "abc123");
    REQUIRE (resourceGroups->hasMoreItems());
    REQUIRE (resourceGroups->getNextItem() == "def456");
    REQUIRE (resourceGroups->hasMoreItems());
    REQUIRE (resourceGroups->getNextItem() == "ghi789");
    REQUIRE (!resourceGroups->hasMoreItems());

    // Verify that all three resource groups are unassigned
    auto unassignedResourceGroups = globalState->listUnmanagedResourceGroups();
    REQUIRE (unassignedResourceGroups->hasMoreItems());
    REQUIRE (unassignedResourceGroups->getNextItem() == "abc123");
    REQUIRE (unassignedResourceGroups->hasMoreItems());
    REQUIRE (unassignedResourceGroups->getNextItem() == "def456");
    REQUIRE (unassignedResourceGroups->hasMoreItems());
    REQUIRE (unassignedResourceGroups->getNextItem() == "ghi789");
    REQUIRE (!unassignedResourceGroups->hasMoreItems());

    // Add one of the resource to a resource manager id
    REQUIRE (globalState->claimManagedResourceGroup("ResourceId1", "abc123"));

    // Verify that only two of the resource groups are unassigned
    unassignedResourceGroups = globalState->listUnmanagedResourceGroups();
    REQUIRE (unassignedResourceGroups->hasMoreItems());
    REQUIRE (unassignedResourceGroups->getNextItem() == "def456");
    REQUIRE (unassignedResourceGroups->hasMoreItems());
    REQUIRE (unassignedResourceGroups->getNextItem() == "ghi789");
    REQUIRE (!unassignedResourceGroups->hasMoreItems());

    // Verify that the one resource group is assigned accordingly
    auto assignedResourceGroups = globalState->listManagedResourceGroups("ResourceId1");
    REQUIRE (assignedResourceGroups->hasMoreItems());
    REQUIRE (assignedResourceGroups->getNextItem() == "abc123");
    REQUIRE (!assignedResourceGroups->hasMoreItems());

    // Assign the teo remaining resource groups to a different resource id
    REQUIRE (globalState->claimManagedResourceGroup("ResourceId2", "def456"));
    REQUIRE (globalState->claimManagedResourceGroup("ResourceId2", "ghi789"));

    // Verify that no resource groups are unassigned now
    unassignedResourceGroups = globalState->listUnmanagedResourceGroups();
    REQUIRE (!unassignedResourceGroups->hasMoreItems());

    // Verify the first resource id still has its item
    assignedResourceGroups = globalState->listManagedResourceGroups("ResourceId1");
    REQUIRE (assignedResourceGroups->hasMoreItems());
    REQUIRE (assignedResourceGroups->getNextItem() == "abc123");
    REQUIRE (!assignedResourceGroups->hasMoreItems());

    // Verify that the other resource id has the other items
    assignedResourceGroups = globalState->listManagedResourceGroups("ResourceId2");
    REQUIRE (assignedResourceGroups->hasMoreItems());
    REQUIRE (assignedResourceGroups->getNextItem() == "def456");
    REQUIRE (assignedResourceGroups->hasMoreItems());
    REQUIRE (assignedResourceGroups->getNextItem() == "ghi789");
    REQUIRE (!assignedResourceGroups->hasMoreItems());

    // Validate that we cannot add a resource group already accounted for to
    // another resource id
    REQUIRE (!globalState->claimManagedResourceGroup("ResourceId1", "ghi789"));

    // Validate that the state is still the same as before
    unassignedResourceGroups = globalState->listUnmanagedResourceGroups();
    REQUIRE (!unassignedResourceGroups->hasMoreItems());
    assignedResourceGroups = globalState->listManagedResourceGroups("ResourceId1");
    REQUIRE (assignedResourceGroups->hasMoreItems());
    REQUIRE (assignedResourceGroups->getNextItem() == "abc123");
    REQUIRE (!assignedResourceGroups->hasMoreItems());
    assignedResourceGroups = globalState->listManagedResourceGroups("ResourceId2");
    REQUIRE (assignedResourceGroups->hasMoreItems());
    REQUIRE (assignedResourceGroups->getNextItem() == "def456");
    REQUIRE (assignedResourceGroups->hasMoreItems());
    REQUIRE (assignedResourceGroups->getNextItem() == "ghi789");
    REQUIRE (!assignedResourceGroups->hasMoreItems());

    // Have the second resource id remove ownership of one resource group
    REQUIRE (globalState->dropManagedResourceGroup("ResourceId2", "ghi789"));

    // Validate that the resource group has been dropped and unassigned
    assignedResourceGroups = globalState->listManagedResourceGroups("ResourceId2");
    REQUIRE (assignedResourceGroups->hasMoreItems());
    REQUIRE (assignedResourceGroups->getNextItem() == "def456");
    REQUIRE (!assignedResourceGroups->hasMoreItems());
    unassignedResourceGroups = globalState->listUnmanagedResourceGroups();
    REQUIRE (unassignedResourceGroups->hasMoreItems());
    REQUIRE (unassignedResourceGroups->getNextItem() == "ghi789");
    REQUIRE (!unassignedResourceGroups->hasMoreItems());

    // Validate that we can now assign the dropped-resource group to another
    REQUIRE (globalState->claimManagedResourceGroup("ResourceId1", "ghi789"));
    assignedResourceGroups = globalState->listManagedResourceGroups("ResourceId1");
    REQUIRE (assignedResourceGroups->hasMoreItems());
    REQUIRE (assignedResourceGroups->getNextItem() == "abc123");
    REQUIRE (assignedResourceGroups->hasMoreItems());
    REQUIRE (assignedResourceGroups->getNextItem() == "ghi789");
    REQUIRE (!assignedResourceGroups->hasMoreItems());

    // Verify that dropping an un-owned resource group fails
    REQUIRE (!globalState->dropManagedResourceGroup("ResourceId2", "ghi789"));

    // Validate that the state is still the same as before
    unassignedResourceGroups = globalState->listUnmanagedResourceGroups();
    REQUIRE (!unassignedResourceGroups->hasMoreItems());
    assignedResourceGroups = globalState->listManagedResourceGroups("ResourceId1");
    REQUIRE (assignedResourceGroups->hasMoreItems());
    REQUIRE (assignedResourceGroups->getNextItem() == "abc123");
    REQUIRE (assignedResourceGroups->hasMoreItems());
    REQUIRE (assignedResourceGroups->getNextItem() == "ghi789");
    REQUIRE (!assignedResourceGroups->hasMoreItems());
    assignedResourceGroups = globalState->listManagedResourceGroups("ResourceId2");
    REQUIRE (assignedResourceGroups->hasMoreItems());
    REQUIRE (assignedResourceGroups->getNextItem() == "def456");
    REQUIRE (!assignedResourceGroups->hasMoreItems());

    // Attempt to remove all resource groups which are currently assigned
    REQUIRE (!globalState->removeResourceGroup("abc123"));
    REQUIRE (!globalState->removeResourceGroup("def456"));
    REQUIRE (!globalState->removeResourceGroup("ghi789"));

    // Validate that the state is still the same as before
    unassignedResourceGroups = globalState->listUnmanagedResourceGroups();
    REQUIRE (!unassignedResourceGroups->hasMoreItems());
    assignedResourceGroups = globalState->listManagedResourceGroups("ResourceId1");
    REQUIRE (assignedResourceGroups->hasMoreItems());
    REQUIRE (assignedResourceGroups->getNextItem() == "abc123");
    REQUIRE (assignedResourceGroups->hasMoreItems());
    REQUIRE (assignedResourceGroups->getNextItem() == "ghi789");
    REQUIRE (!assignedResourceGroups->hasMoreItems());
    assignedResourceGroups = globalState->listManagedResourceGroups("ResourceId2");
    REQUIRE (assignedResourceGroups->hasMoreItems());
    REQUIRE (assignedResourceGroups->getNextItem() == "def456");
    REQUIRE (!assignedResourceGroups->hasMoreItems());

    // Drop all resource groups from their resource ids
    REQUIRE (globalState->dropManagedResourceGroup("ResourceId1", "abc123"));
    REQUIRE (globalState->dropManagedResourceGroup("ResourceId1", "ghi789"));
    REQUIRE (globalState->dropManagedResourceGroup("ResourceId2", "def456"));

    // Verify that both resource ids have no resources
    assignedResourceGroups = globalState->listManagedResourceGroups("ResourceId1");
    REQUIRE (!assignedResourceGroups->hasMoreItems());
    assignedResourceGroups = globalState->listManagedResourceGroups("ResourceId2");
    REQUIRE (!assignedResourceGroups->hasMoreItems());

    // Verify that all three resource groups are unmanaged
    unassignedResourceGroups = globalState->listUnmanagedResourceGroups();
    REQUIRE (unassignedResourceGroups->hasMoreItems());
    REQUIRE (unassignedResourceGroups->getNextItem() == "abc123");
    REQUIRE (unassignedResourceGroups->hasMoreItems());
    REQUIRE (unassignedResourceGroups->getNextItem() == "def456");
    REQUIRE (unassignedResourceGroups->hasMoreItems());
    REQUIRE (unassignedResourceGroups->getNextItem() == "ghi789");
    REQUIRE (!unassignedResourceGroups->hasMoreItems());

    // Actually remove the resource groups
    REQUIRE (globalState->removeResourceGroup("abc123"));
    REQUIRE (globalState->removeResourceGroup("def456"));
    REQUIRE (globalState->removeResourceGroup("ghi789"));

    // Verify that no more unassigned resource groups exist
    unassignedResourceGroups = globalState->listUnmanagedResourceGroups();
    REQUIRE (!unassignedResourceGroups->hasMoreItems());

    // Cleanup the global-state when we are finished
    REQUIRE (globalState->clearEntireState());
}

#endif //BITQUARK_GLOBALSTATE_TEST_HPP
