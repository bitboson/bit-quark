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

#ifndef BITQUARK_S3DATASTORE_TEST_HPP
#define BITQUARK_S3DATASTORE_TEST_HPP

#include <catch.hpp>
#include <iostream>
#include <BitBoson/BitQuark/Storage/S3DataStore.h>
#include <BitBoson/BitQuark/Storage/S3Credentials.h>

using namespace BitBoson::BitQuark;

/**
 * Test function used to get the testing S3-Credentials
 *
 * @param dirPrefix String representing the directory/key prefix for the bucket
 * @param badKeys Boolean indicating whether the s3 access keys should be invalid
 * @return S3 Credentials reference to use for testing
 */
std::shared_ptr<S3Credentials> getTestS3Credentials(const std::string& dirPrefix="",
        bool badKeys=false)
{
    std::string secretKey = (badKeys ? "ThisIsABadSecretKey" : "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY");
    return std::make_shared<S3Credentials>("localhost:9000", "test-bucket", dirPrefix,
            "AKIAIOSFODNN7EXAMPLE", secretKey);
}

TEST_CASE ("General S3-Data-Store with Existing Directory Test", "[S3DataStoreTest]")
{

    // Create a s3 data-store with the given setup
    auto s3Credentials = getTestS3Credentials("S3DataStoreTest");
    auto dataStore = S3DataStore(s3Credentials);

    // Cleanup s3-data-store instance
    REQUIRE(dataStore.deleteEntireDataStore(true));

    // Insert some data in the data-store
    REQUIRE(dataStore.addItem("Key1", "Value1"));
    REQUIRE(dataStore.addItem("Key2", "Value2"));
    REQUIRE(dataStore.addItem("Key3", "Value3"));

    // Verify the size of the stored data
    REQUIRE(dataStore.getSize() == 18);

    // Retrieve the data from the data-store
    REQUIRE(dataStore.getItem("Key1") == "Value1");
    REQUIRE(dataStore.getItem("Key2") == "Value2");
    REQUIRE(dataStore.getItem("Key3") == "Value3");

    // List the items in the s3-data-store
    int index = 0;
    std::string itemsListing[] = {"Key1", "Key2", "Key3"};
    auto itemsGenerator = dataStore.listItems();
    while (itemsGenerator->hasMoreItems())
        REQUIRE(itemsGenerator->getNextItem() == itemsListing[index++]);

    // Cleanup s3-data-store instance
    REQUIRE(dataStore.deleteEntireDataStore(true));
}

TEST_CASE ("Add and Delete Items S3-Data-Store Test", "[S3DataStoreTest]")
{

    // Create a s3 data-store with the given setup
    auto s3Credentials = getTestS3Credentials("S3DataStoreTest");
    auto dataStore = S3DataStore(s3Credentials);

    // Cleanup s3-data-store instance
    REQUIRE(dataStore.deleteEntireDataStore(true));

    // Insert some data in the data-store
    REQUIRE(dataStore.addItem("Key1", "Value1"));
    REQUIRE(dataStore.addItem("Key2", "Value2"));
    REQUIRE(dataStore.addItem("Key3", "Value3"));

    // Verify the size of the stored data
    REQUIRE(dataStore.getSize() == 18);

    // Retrieve the data from the data-store
    REQUIRE(dataStore.getItem("Key1") == "Value1");
    REQUIRE(dataStore.getItem("Key2") == "Value2");
    REQUIRE(dataStore.getItem("Key3") == "Value3");

    // Delete a few items
    REQUIRE(dataStore.deleteItem("Key1"));
    REQUIRE(dataStore.deleteItem("Key2"));

    // Verify the size of the stored data
    REQUIRE(dataStore.getSize() == 6);

    // Verify that the items are deleted
    REQUIRE(dataStore.getItem("Key1").empty());
    REQUIRE(dataStore.getItem("Key2").empty());
    REQUIRE(dataStore.getItem("Key3") == "Value3");

    // Cleanup s3-data-store instance
    REQUIRE(dataStore.deleteEntireDataStore(true));
}

TEST_CASE ("Flush Cache on Destruction S3-Data-Store Test", "[S3DataStoreTest]")
{

    // Create a s3 data-store with the given setup
    auto s3Credentials = getTestS3Credentials("S3DataStoreTest");
    auto dataStore = S3DataStore(s3Credentials);

    // Cleanup s3-data-store instance
    REQUIRE(dataStore.deleteEntireDataStore(true));

    // Insert some data in the data-store
    REQUIRE(dataStore.addItem("Key1", "Value1"));
    REQUIRE(dataStore.addItem("Key2", "Value2"));
    REQUIRE(dataStore.addItem("Key3", "Value3"));

    // Verify the size of the stored data
    REQUIRE(dataStore.getSize() == 18);

    // Destruct the current s3 data-store object
    // to force a cache-flush and re-create it
    dataStore = S3DataStore(s3Credentials);

    // Retrieve the data from the data-store
    REQUIRE(dataStore.getItem("Key1") == "Value1");
    REQUIRE(dataStore.getItem("Key2") == "Value2");
    REQUIRE(dataStore.getItem("Key3") == "Value3");

    // Delete a few items
    REQUIRE(dataStore.deleteItem("Key1"));
    REQUIRE(dataStore.deleteItem("Key2"));

    // Verify the size of the stored data
    REQUIRE(dataStore.getSize() == 6);

    // Verify that the items are deleted
    REQUIRE(dataStore.getItem("Key1").empty());
    REQUIRE(dataStore.getItem("Key2").empty());
    REQUIRE(dataStore.getItem("Key3") == "Value3");

    // Cleanup s3-data-store instance
    REQUIRE(dataStore.deleteEntireDataStore(true));
}

TEST_CASE ("Add and List 2000 Items S3-Data-Store Test", "[S3DataStoreTest]")
{

    // Create a s3 data-store with the given setup
    auto s3Credentials = getTestS3Credentials("S3DataStoreTest");
    auto dataStore = S3DataStore(s3Credentials);

    // Cleanup s3-data-store instance
    REQUIRE(dataStore.deleteEntireDataStore(true));

    // Insert some data in the data-store
    // Also add it to an unordered map for reference
    std::unordered_map<std::string, std::string> refMap;
    for (int ii = 0; ii < 2000; ii++)
    {
        REQUIRE(dataStore.addItem(
            std::string("Key") + std::to_string(ii),
            std::string("Value") + std::to_string(ii)));
        refMap[std::string("Key") + std::to_string(ii)] =
                std::string(std::string("Key") + std::to_string(ii));
    }
    REQUIRE(refMap.size() == 2000);

    // List all 2000 items in the datastore
    auto itemsGenerator = dataStore.listItems();
    while (itemsGenerator->hasMoreItems())
    {
        auto currKey = itemsGenerator->getNextItem();
        REQUIRE(refMap[currKey] == currKey);
        refMap.erase(currKey);
    }
    REQUIRE(refMap.empty());

    // Cleanup s3-data-store instance
    REQUIRE(dataStore.deleteEntireDataStore(true));
}

TEST_CASE ("Terminate 2000 Item Listing Early S3-Data-Store Test", "[S3DataStoreTest]")
{

    // Create a s3 data-store with the given setup
    auto s3Credentials = getTestS3Credentials("S3DataStoreTest");
    auto dataStore = S3DataStore(s3Credentials);

    // Cleanup s3-data-store instance
    REQUIRE(dataStore.deleteEntireDataStore(true));

    // Insert some data in the data-store
    // Also add it to an unordered map for reference
    for (int ii = 0; ii < 2000; ii++)
        REQUIRE(dataStore.addItem(
            std::string("Key") + std::to_string(ii),
            std::string("Value") + std::to_string(ii)));

    // List all 2000 items in the datastore but terminate at the
    // 999th item to trigger a terminate on the yielder
    int counter = 0;
    auto itemsGenerator = dataStore.listItems();
    while (itemsGenerator->hasMoreItems())
    {
        auto currKey = itemsGenerator->getNextItem();
        if (++counter == 999)
            itemsGenerator->quitRemainingItems();
    }

    // List all 2000 items in the datastore but terminate at the
    // 1000th item to trigger a terminate on the yielder
    counter = 0;
    itemsGenerator = dataStore.listItems();
    while (itemsGenerator->hasMoreItems())
    {
        auto currKey = itemsGenerator->getNextItem();
        if (++counter == 1000)
            itemsGenerator->quitRemainingItems();
    }

    // Cleanup s3-data-store instance
    REQUIRE(dataStore.deleteEntireDataStore(true));
}

TEST_CASE ("Add and Delete Entire Data-Store S3-Data-Store Test", "[S3DataStoreTest]")
{

    // Create a s3 data-store with the given setup
    auto s3Credentials = getTestS3Credentials("S3DataStoreTest");
    auto dataStore = S3DataStore(s3Credentials);

    // Cleanup s3-data-store instance
    REQUIRE(dataStore.deleteEntireDataStore(true));

    // Insert some data in the data-store
    REQUIRE(dataStore.addItem("Key1", "Value1"));
    REQUIRE(dataStore.addItem("Key2", "Value2"));
    REQUIRE(dataStore.addItem("Key3", "Value3"));

    // Verify the size of the stored data
    REQUIRE(dataStore.getSize() == 18);

    // Retrieve the data from the data-store
    REQUIRE(dataStore.getItem("Key1") == "Value1");
    REQUIRE(dataStore.getItem("Key2") == "Value2");
    REQUIRE(dataStore.getItem("Key3") == "Value3");

    // Cleanup s3-data-store instance
    // using the multi-delete method
    REQUIRE(dataStore.deleteEntireDataStore(true));

    // Verify the size of the stored data
    REQUIRE(dataStore.getSize() == 0);

    // Verify that the items are deleted
    REQUIRE(dataStore.getItem("Key1").empty());
    REQUIRE(dataStore.getItem("Key2").empty());
    REQUIRE(dataStore.getItem("Key3").empty());

    // Insert some data in the data-store
    REQUIRE(dataStore.addItem("Key1", "Value1"));
    REQUIRE(dataStore.addItem("Key2", "Value2"));
    REQUIRE(dataStore.addItem("Key3", "Value3"));

    // Verify the size of the stored data
    REQUIRE(dataStore.getSize() == 18);

    // Retrieve the data from the data-store
    REQUIRE(dataStore.getItem("Key1") == "Value1");
    REQUIRE(dataStore.getItem("Key2") == "Value2");
    REQUIRE(dataStore.getItem("Key3") == "Value3");

    // Cleanup s3-data-store instance
    // using the multi-delete method
    REQUIRE(dataStore.deleteEntireDataStore(false));

    // Verify the size of the stored data
    REQUIRE(dataStore.getSize() == 0);

    // Verify that the items are deleted
    REQUIRE(dataStore.getItem("Key1").empty());
    REQUIRE(dataStore.getItem("Key2").empty());
    REQUIRE(dataStore.getItem("Key3").empty());

    // Cleanup s3-data-store instance
    REQUIRE(dataStore.deleteEntireDataStore(true));
}

TEST_CASE ("S3 General/Misc. Metadata Test Test", "[S3DataStoreTest]")
{

    // Create a s3 data-store with the given setup
    auto s3Credentials = getTestS3Credentials("S3DataStoreTest");
    auto dataStore = S3DataStore(s3Credentials);

    // Cleanup s3-data-store instance
    REQUIRE(dataStore.deleteEntireDataStore(true));

    // Insert some data in the data-store
    REQUIRE(dataStore.addItem("Key1", "Value1"));
    REQUIRE(dataStore.addItem("Key2", "Value2"));
    REQUIRE(dataStore.addItem("Key3", "Value3"));

    // Verify the size of the stored data
    REQUIRE(dataStore.getSize() == 18);

    // Retrieve the data from the data-store
    REQUIRE(dataStore.getItem("Key1") == "Value1");
    REQUIRE(dataStore.getItem("Key2") == "Value2");
    REQUIRE(dataStore.getItem("Key3") == "Value3");

    // Add in a few misc. metadata items
    dataStore.setMiscMetadataValue("MdKey1", "MdValue1");
    dataStore.setMiscMetadataValue("MdKey2", "MdValue2");
    dataStore.setMiscMetadataValue("MdKey3", "MdValue3");
    dataStore.setMiscMetadataValue("MdKey4", "MdValue4");

    // Get the misc. metadata items
    REQUIRE(dataStore.getMiscMetadataValue("MdKey0").empty());
    REQUIRE(dataStore.getMiscMetadataValue("MdKey1") == "MdValue1");
    REQUIRE(dataStore.getMiscMetadataValue("MdKey2") == "MdValue2");
    REQUIRE(dataStore.getMiscMetadataValue("MdKey3") == "MdValue3");
    REQUIRE(dataStore.getMiscMetadataValue("MdKey4") == "MdValue4");
    REQUIRE(dataStore.getMiscMetadataValue("MdKey5", "Default") == "Default");

    // Create a second s3-data-store instance off of the first
    auto dataStore2 = S3DataStore(s3Credentials);

    // Get the misc. metadata items
    REQUIRE(dataStore2.getMiscMetadataValue("MdKey0").empty());
    REQUIRE(dataStore2.getMiscMetadataValue("MdKey1") == "MdValue1");
    REQUIRE(dataStore2.getMiscMetadataValue("MdKey2") == "MdValue2");
    REQUIRE(dataStore2.getMiscMetadataValue("MdKey3") == "MdValue3");
    REQUIRE(dataStore2.getMiscMetadataValue("MdKey4") == "MdValue4");
    REQUIRE(dataStore2.getMiscMetadataValue("MdKey5", "Default") == "Default");

    // Cleanup s3-data-store instance
    REQUIRE(dataStore.deleteEntireDataStore(true));
}

TEST_CASE ("List Items with Invalid Keys S3-Data-Store Test", "[S3DataStoreTest]")
{

    // Create a s3 data-store with the given setup
    auto s3Credentials = getTestS3Credentials("S3DataStoreTest");
    auto dataStore = S3DataStore(s3Credentials);

    // Cleanup s3-data-store instance
    REQUIRE(dataStore.deleteEntireDataStore(true));

    // Insert some data in the data-store
    for (int ii = 0; ii < 100; ii++)
        REQUIRE(dataStore.addItem(
            std::string("Key") + std::to_string(ii),
            std::string("Value") + std::to_string(ii)));

    // Get bad keys for the s3 data-store
    auto s3CredentialsBad = getTestS3Credentials("S3DataStoreTest", true);
    auto dataStoreBad = S3DataStore(s3CredentialsBad);

    // Attempt to list against the s3 data-store with bad keys
    auto itemsGenerator = dataStoreBad.listItems();
    REQUIRE (!itemsGenerator->hasMoreItems());

    // Cleanup s3-data-store instance
    REQUIRE(dataStore.deleteEntireDataStore(true));
}

TEST_CASE ("Delete Data-Store with Invalid Keys S3-Data-Store Test", "[S3DataStoreTest]")
{

    // Create a s3 data-store with the given setup
    auto s3Credentials = getTestS3Credentials("S3DataStoreTest");
    auto dataStore = S3DataStore(s3Credentials);

    // Cleanup s3-data-store instance
    REQUIRE(dataStore.deleteEntireDataStore(true));

    // Insert some data in the data-store
    for (int ii = 0; ii < 100; ii++)
        REQUIRE(dataStore.addItem(
            std::string("Key") + std::to_string(ii),
            std::string("Value") + std::to_string(ii)));

    // Get bad keys for the s3 data-store
    auto s3CredentialsBad = getTestS3Credentials("S3DataStoreTest", true);
    auto dataStoreBad = S3DataStore(s3CredentialsBad);

    // Attempt to delete the entire s3 data-store with bad keys
    REQUIRE(!dataStoreBad.deleteEntireDataStore(true));

    // Cleanup s3-data-store instance
    REQUIRE(dataStore.deleteEntireDataStore(true));
}

#endif //BITQUARK_S3DATASTORE_TEST_HPP
