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

#ifndef BITQUARK_MASTERNODE_TEST_HPP
#define BITQUARK_MASTERNODE_TEST_HPP

#include <catch.hpp>
#include <memory>
#include <BitBoson/BitQuark/Networking/Requests.h>
#include <BitBoson/BitQuark/Cluster/Components/MasterNode.h>
#include <BitBoson/BitQuark/Cluster/Components/WorkerNode.h>

using namespace BitBoson::BitQuark;

TEST_CASE("Single Random Master-Node Cluster Test", "[MasterNodeTest]")
{

    // Create a single master node
    auto masterNode = std::make_shared<MasterNode>("localhost", 9996);
    masterNode->start();

    // TODO: Need to check id against a regex for sha256

    // Ensure the server has a chance to start its background
    // processes for better thread-testing
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Validate that it has a quorum status
    auto response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9996/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "1/1");

    // Validate that the internal status checks-out
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9996/internal/master/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "1/1");
}

TEST_CASE("Single Named Master-Node Cluster Test", "[MasterNodeTest]")
{

    // Create a single master node
    auto masterNode = std::make_shared<MasterNode>("localhost", 9996, "SpecifiedId");
    masterNode->start();

    // Ensure the server has a chance to start its background
    // processes for better thread-testing
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Validate that it has a quorum status
    auto response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9996/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["SpecifiedId"] == "SelfInstance");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "1/1");

    // Validate that the internal status checks-out
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9996/internal/master/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["SpecifiedId"] == "SelfInstance");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "1/1");
}

TEST_CASE("Multiple Master-Nodes Cluster Test", "[MasterNodeTest]")
{

    // Create three master nodes
    auto masterNode1 = std::make_shared<MasterNode>("localhost", 9996, "SpecifiedId1");
    masterNode1->start();
    auto masterNode2 = std::make_shared<MasterNode>("localhost", 9997, "SpecifiedId2");
    masterNode2->start();
    auto masterNode3 = std::make_shared<MasterNode>("localhost", 9998, "SpecifiedId3");
    masterNode3->start();

    // Ensure the server has a chance to start its background
    // processes for better thread-testing
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Validate that each individual server has quorum status
    auto response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9996/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["SpecifiedId1"] == "SelfInstance");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "1/1");
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9997/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["SpecifiedId2"] == "SelfInstance");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "1/1");
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9998/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["SpecifiedId3"] == "SelfInstance");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "1/1");

    // Verify that the master nodes shows themselves in quorum
    // and an empty list of other connected master nodes
    REQUIRE (masterNode1->isInQuorum());
    auto connectedMasters = masterNode1->getConnectedMasters();
    REQUIRE (connectedMasters.size() == 0);
    REQUIRE (masterNode2->isInQuorum());
    connectedMasters = masterNode2->getConnectedMasters();
    REQUIRE (connectedMasters.size() == 0);
    REQUIRE (masterNode3->isInQuorum());
    connectedMasters = masterNode3->getConnectedMasters();
    REQUIRE (connectedMasters.size() == 0);

    // Verify that no node can find each other's connected master node URLs
    REQUIRE (masterNode1->getUrlForConnectedMasterNode("SpecifiedId2").empty());
    REQUIRE (masterNode1->getUrlForConnectedMasterNode("SpecifiedId3").empty());
    REQUIRE (masterNode2->getUrlForConnectedMasterNode("SpecifiedId1").empty());
    REQUIRE (masterNode2->getUrlForConnectedMasterNode("SpecifiedId3").empty());
    REQUIRE (masterNode3->getUrlForConnectedMasterNode("SpecifiedId1").empty());
    REQUIRE (masterNode3->getUrlForConnectedMasterNode("SpecifiedId2").empty());

    // Add the second two nodes to the first node
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9996/internal/master/join",
            {{"NodeId", "SpecifiedId2"}, {"NodeUrl", "http://localhost:9997"}});
    REQUIRE(response.code == 201);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["AddedNode"] == "True");
    REQUIRE(response.body["NodeId"] == "SpecifiedId2");
    REQUIRE(response.body["NodeUrl"] == "http://localhost:9997");
    REQUIRE(response.body["Message"] == "The node will be added to the cluster");
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9996/internal/master/join",
            {{"NodeId", "SpecifiedId3"}, {"NodeUrl", "http://localhost:9998"}});
    REQUIRE(response.code == 201);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["AddedNode"] == "True");
    REQUIRE(response.body["NodeId"] == "SpecifiedId3");
    REQUIRE(response.body["NodeUrl"] == "http://localhost:9998");
    REQUIRE(response.body["Message"] == "The node will be added to the cluster");

    // Ensure the server has a chance to connect to each other
    std::this_thread::sleep_for(std::chrono::seconds(20));

    // Verify that the master nodes shows themselves in quorum
    // and a list of other connected master nodes which is valid
    REQUIRE (masterNode1->isInQuorum());
    connectedMasters = masterNode1->getConnectedMasters();
    REQUIRE (connectedMasters.size() == 2);
    REQUIRE (connectedMasters[0] == "SpecifiedId3");
    REQUIRE (connectedMasters[1] == "SpecifiedId2");
    REQUIRE (masterNode2->isInQuorum());
    connectedMasters = masterNode2->getConnectedMasters();
    REQUIRE (connectedMasters.size() == 2);
    REQUIRE (connectedMasters[0] == "SpecifiedId3");
    REQUIRE (connectedMasters[1] == "SpecifiedId1");
    REQUIRE (masterNode3->isInQuorum());
    connectedMasters = masterNode3->getConnectedMasters();
    REQUIRE (connectedMasters.size() == 2);
    REQUIRE (connectedMasters[0] == "SpecifiedId2");
    REQUIRE (connectedMasters[1] == "SpecifiedId1");

    // Verify that each master node can get the other master node URLs
    REQUIRE (masterNode1->getUrlForConnectedMasterNode("SpecifiedId2") == "http://localhost:9997");
    REQUIRE (masterNode1->getUrlForConnectedMasterNode("SpecifiedId3") == "http://localhost:9998");
    REQUIRE (masterNode2->getUrlForConnectedMasterNode("SpecifiedId1") == "http://localhost:9996");
    REQUIRE (masterNode2->getUrlForConnectedMasterNode("SpecifiedId3") == "http://localhost:9998");
    REQUIRE (masterNode3->getUrlForConnectedMasterNode("SpecifiedId1") == "http://localhost:9996");
    REQUIRE (masterNode3->getUrlForConnectedMasterNode("SpecifiedId2") == "http://localhost:9997");

    // Validate that all of the nodes know about each other and are reporting
    // the current quorum correctly
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9996/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 5);
    REQUIRE(response.body["SpecifiedId1"] == "SelfInstance");
    REQUIRE(response.body["SpecifiedId2"] == "Connected");
    REQUIRE(response.body["SpecifiedId3"] == "Connected");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "3/3");
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9997/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 5);
    REQUIRE(response.body["SpecifiedId1"] == "Connected");
    REQUIRE(response.body["SpecifiedId2"] == "SelfInstance");
    REQUIRE(response.body["SpecifiedId3"] == "Connected");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "3/3");
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9998/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 5);
    REQUIRE(response.body["SpecifiedId1"] == "Connected");
    REQUIRE(response.body["SpecifiedId2"] == "Connected");
    REQUIRE(response.body["SpecifiedId3"] == "SelfInstance");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "3/3");

    // Drop-out the second node by shutting it down
    masterNode2 = nullptr;

    // Ensure the servers have a chance to timeout node 2
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // Verify that the remaining master nodes shows themselves in quorum
    // and a list of other connected master nodes which is valid
    REQUIRE (masterNode1->isInQuorum());
    connectedMasters = masterNode1->getConnectedMasters();
    REQUIRE (connectedMasters.size() == 1);
    REQUIRE (connectedMasters[0] == "SpecifiedId3");
    REQUIRE (masterNode3->isInQuorum());
    connectedMasters = masterNode3->getConnectedMasters();
    REQUIRE (connectedMasters.size() == 1);
    REQUIRE (connectedMasters[0] == "SpecifiedId1");

    // Validate that the remaining two nodes are returning the correct quorum
    // of 2 out of 3, and that it is still met
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9996/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 5);
    REQUIRE(response.body["SpecifiedId1"] == "SelfInstance");
    REQUIRE(response.body["SpecifiedId2"] == "NotConnected");
    REQUIRE(response.body["SpecifiedId3"] == "Connected");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "2/3");
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9998/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 5);
    REQUIRE(response.body["SpecifiedId1"] == "Connected");
    REQUIRE(response.body["SpecifiedId2"] == "NotConnected");
    REQUIRE(response.body["SpecifiedId3"] == "SelfInstance");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "2/3");

    // Verify that each master node can get the other master node URLs
    // only when they are connected, and not when they are not
    REQUIRE (masterNode1->getUrlForConnectedMasterNode("SpecifiedId2").empty());
    REQUIRE (masterNode1->getUrlForConnectedMasterNode("SpecifiedId3") == "http://localhost:9998");
    REQUIRE (masterNode3->getUrlForConnectedMasterNode("SpecifiedId1") == "http://localhost:9996");
    REQUIRE (masterNode3->getUrlForConnectedMasterNode("SpecifiedId2").empty());

    // Start-up server 2 again and allow it to rejoin the cluster
    masterNode2 = std::make_shared<MasterNode>("localhost", 9997, "SpecifiedId2");
    masterNode2->start();

    // Ensure the servers have a chance to find node 2 again
    std::this_thread::sleep_for(std::chrono::seconds(30));

    // Validate that all of the nodes know about each other and are reporting
    // the current quorum correctly
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9996/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 5);
    REQUIRE(response.body["SpecifiedId1"] == "SelfInstance");
    REQUIRE(response.body["SpecifiedId2"] == "Connected");
    REQUIRE(response.body["SpecifiedId3"] == "Connected");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "3/3");
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9997/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 5);
    REQUIRE(response.body["SpecifiedId1"] == "Connected");
    REQUIRE(response.body["SpecifiedId2"] == "SelfInstance");
    REQUIRE(response.body["SpecifiedId3"] == "Connected");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "3/3");
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9998/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 5);
    REQUIRE(response.body["SpecifiedId1"] == "Connected");
    REQUIRE(response.body["SpecifiedId2"] == "Connected");
    REQUIRE(response.body["SpecifiedId3"] == "SelfInstance");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "3/3");

    // Bring server 2 back down as well as server 3
    masterNode2 = nullptr;
    masterNode3 = nullptr;

    // Ensure the server have a chance to timeout node 2 and node 3
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // Verify that the remaining master node shows themselves not in quorum
    // and a list of other connected master nodes which is empty at this point
    REQUIRE (!masterNode1->isInQuorum());
    connectedMasters = masterNode1->getConnectedMasters();
    REQUIRE (connectedMasters.size() == 0);

    // Verify that server 1 no longer has quorum
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9996/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 5);
    REQUIRE(response.body["SpecifiedId1"] == "SelfInstance");
    REQUIRE(response.body["SpecifiedId2"] == "NotConnected");
    REQUIRE(response.body["SpecifiedId3"] == "NotConnected");
    REQUIRE(response.body["QuorumMet"] == "False");
    REQUIRE(response.body["ClusterSize"] == "1/3");

    // Bring back up server 3 and allow it to rejoin the cluster
    masterNode3 = std::make_shared<MasterNode>("localhost", 9998, "SpecifiedId3");
    masterNode3->start();

    // Ensure the server has a chance to find node 3 again
    std::this_thread::sleep_for(std::chrono::seconds(20));

    // Verify that the remaining master nodes shows themselves in quorum
    // and a list of other connected master nodes which is valid
    REQUIRE (masterNode1->isInQuorum());
    connectedMasters = masterNode1->getConnectedMasters();
    REQUIRE (connectedMasters.size() == 1);
    REQUIRE (connectedMasters[0] == "SpecifiedId3");
    REQUIRE (masterNode3->isInQuorum());
    connectedMasters = masterNode3->getConnectedMasters();
    REQUIRE (connectedMasters.size() == 1);
    REQUIRE (connectedMasters[0] == "SpecifiedId1");

    // Verify that the servers are connected and quorum is re-established
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9996/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 5);
    REQUIRE(response.body["SpecifiedId1"] == "SelfInstance");
    REQUIRE(response.body["SpecifiedId2"] == "NotConnected");
    REQUIRE(response.body["SpecifiedId3"] == "Connected");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "2/3");
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9998/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 5);
    REQUIRE(response.body["SpecifiedId1"] == "Connected");
    REQUIRE(response.body["SpecifiedId2"] == "NotConnected");
    REQUIRE(response.body["SpecifiedId3"] == "SelfInstance");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "2/3");

    // Setup the leave timeout as only 30 seconds to verify self-healing will
    // still take place if we wait too long to complete all leave requests
    REQUIRE(masterNode1->setLeftNodeTimeout(30));
    REQUIRE(masterNode3->setLeftNodeTimeout(30));

    // Have node 2 officially leave the cluster
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9996/internal/master/leave",
            {{"NodeId", "SpecifiedId2"}});
    REQUIRE(response.code == 202);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["RemovedNode"] == "True");
    REQUIRE(response.body["NodeId"] == "SpecifiedId2");
    REQUIRE(response.body["Message"] == "The node will be removed from the cluster");
    std::this_thread::sleep_for(std::chrono::seconds(60));
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9998/internal/master/leave",
            {{"NodeId", "SpecifiedId2"}});
    REQUIRE(response.code == 202);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["RemovedNode"] == "True");
    REQUIRE(response.body["NodeId"] == "SpecifiedId2");
    REQUIRE(response.body["Message"] == "The node will be removed from the cluster");

    // Wait for things to sync-up
    std::this_thread::sleep_for(std::chrono::seconds(60));

    // Verify that the servers were able to heal the state despite node 2 leaving
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9996/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 5);
    REQUIRE(response.body["SpecifiedId1"] == "SelfInstance");
    REQUIRE(response.body["SpecifiedId2"] == "NotConnected");
    REQUIRE(response.body["SpecifiedId3"] == "Connected");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "2/3");
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9998/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 5);
    REQUIRE(response.body["SpecifiedId1"] == "Connected");
    REQUIRE(response.body["SpecifiedId2"] == "NotConnected");
    REQUIRE(response.body["SpecifiedId3"] == "SelfInstance");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "2/3");

    // Setup the leave timeout as the default 300 seconds to verify self-healing will
    // not take place if we wait a similar amount of time to complete all leave requests
    REQUIRE(masterNode1->setLeftNodeTimeout(300));
    REQUIRE(masterNode3->setLeftNodeTimeout(300));

    // Have node 2 officially leave the cluster
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9996/internal/master/leave",
            {{"NodeId", "SpecifiedId2"}});
    REQUIRE(response.code == 202);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["RemovedNode"] == "True");
    REQUIRE(response.body["NodeId"] == "SpecifiedId2");
    REQUIRE(response.body["Message"] == "The node will be removed from the cluster");
    std::this_thread::sleep_for(std::chrono::seconds(60));
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9998/internal/master/leave",
            {{"NodeId", "SpecifiedId2"}});
    REQUIRE(response.code == 202);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["RemovedNode"] == "True");
    REQUIRE(response.body["NodeId"] == "SpecifiedId2");
    REQUIRE(response.body["Message"] == "The node will be removed from the cluster");

    // Verify that the servers are connected and quorum is at max capacity of 2
    // Also verify that 2 is no longer a part of the cluster
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9996/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["SpecifiedId1"] == "SelfInstance");
    REQUIRE(response.body["SpecifiedId3"] == "Connected");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "2/2");
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9998/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["SpecifiedId1"] == "Connected");
    REQUIRE(response.body["SpecifiedId3"] == "SelfInstance");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "2/2");

    // Start-up server 2 again so we can verify it doesn't try to re-join
    masterNode2 = std::make_shared<MasterNode>("localhost", 9997, "SpecifiedId2");
    masterNode2->start();

    // WAit for things to possibly sync-up (though nothing should)
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // Verify that nodes 1 and 3 did not add node 2 and vice-versa
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9996/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["SpecifiedId1"] == "SelfInstance");
    REQUIRE(response.body["SpecifiedId3"] == "Connected");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "2/2");
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9997/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["SpecifiedId2"] == "SelfInstance");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "1/1");
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9998/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["SpecifiedId1"] == "Connected");
    REQUIRE(response.body["SpecifiedId3"] == "SelfInstance");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "2/2");
}

TEST_CASE("Post a Bad Master Join Cluster Test", "[MasterNodeTest]")
{

    // Create three master nodes
    auto masterNode1 = std::make_shared<MasterNode>("localhost", 9996, "SpecifiedId1");
    masterNode1->start();
    auto masterNode2 = std::make_shared<MasterNode>("localhost", 9997, "SpecifiedId2");
    masterNode2->start();

    // Ensure the server has a chance to start its background
    // processes for better thread-testing
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Add the second node to the first
    auto response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9996/internal/master/join",
            {{"NodeId", "SpecifiedId2"}, {"NodeUrl", "http://localhost:9997"}});
    REQUIRE(response.code == 201);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["AddedNode"] == "True");
    REQUIRE(response.body["NodeId"] == "SpecifiedId2");
    REQUIRE(response.body["NodeUrl"] == "http://localhost:9997");
    REQUIRE(response.body["Message"] == "The node will be added to the cluster");

    // Wait for the two nodes to be synchronized
    std::this_thread::sleep_for(std::chrono::seconds(20));

    // Validate that the cluster status shows the two nodes together
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9996/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["SpecifiedId1"] == "SelfInstance");
    REQUIRE(response.body["SpecifiedId2"] == "Connected");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "2/2");
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9997/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["SpecifiedId1"] == "Connected");
    REQUIRE(response.body["SpecifiedId2"] == "SelfInstance");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "2/2");

    // Try to add a node without a node id
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9996/internal/master/join",
            {{"NodeUrl", "http://localhost:9998"}});
    REQUIRE(response.code == 400);
    REQUIRE(response.body.size() == 1);
    REQUIRE(response.body["MissingArgument"] == "NodeId");

    // Try to add a node without a node url
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9996/internal/master/join",
            {{"NodeId", "SpecifiedId3"}});
    REQUIRE(response.code == 400);
    REQUIRE(response.body.size() == 1);
    REQUIRE(response.body["MissingArgument"] == "NodeUrl");

    // Try to add a node with an id that matches the recipient
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9996/internal/master/join",
            {{"NodeId", "SpecifiedId1"}, {"NodeUrl", "http://localhost:9998"}});
    REQUIRE(response.code == 400);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["AddedNode"] == "False");
    REQUIRE(response.body["NodeId"] == "SpecifiedId1");
    REQUIRE(response.body["NodeUrl"] == "http://localhost:9998");
    REQUIRE(response.body["Message"] == "A node with the same ID already exists");

    // Try to add a node with a url that matches the recipient
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9996/internal/master/join",
            {{"NodeId", "SpecifiedId3"}, {"NodeUrl", "http://localhost:9996"}});
    REQUIRE(response.code == 400);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["AddedNode"] == "False");
    REQUIRE(response.body["NodeId"] == "SpecifiedId3");
    REQUIRE(response.body["NodeUrl"] == "http://localhost:9996");
    REQUIRE(response.body["Message"] == "A node with the same URL already exists");

    // Try to add a node with an id that matches an existing node
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9996/internal/master/join",
            {{"NodeId", "SpecifiedId2"}, {"NodeUrl", "http://localhost:9998"}});
    REQUIRE(response.code == 400);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["AddedNode"] == "False");
    REQUIRE(response.body["NodeId"] == "SpecifiedId2");
    REQUIRE(response.body["NodeUrl"] == "http://localhost:9998");
    REQUIRE(response.body["Message"] == "A node with the same ID already exists");

    // Try to add a node with a url that matches an existing node
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9996/internal/master/join",
            {{"NodeId", "SpecifiedId3"}, {"NodeUrl", "http://localhost:9997"}});
    REQUIRE(response.code == 400);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["AddedNode"] == "False");
    REQUIRE(response.body["NodeId"] == "SpecifiedId3");
    REQUIRE(response.body["NodeUrl"] == "http://localhost:9997");
    REQUIRE(response.body["Message"] == "A node with the same URL already exists");
}

TEST_CASE("Post a Bad Master Leave Cluster Test", "[MasterNodeTest]")
{

    // Create three master nodes
    auto masterNode1 = std::make_shared<MasterNode>("localhost", 9996, "SpecifiedId1");
    masterNode1->start();
    auto masterNode2 = std::make_shared<MasterNode>("localhost", 9997, "SpecifiedId2");
    masterNode2->start();

    // Ensure the server has a chance to start its background
    // processes for better thread-testing
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Add the second node to the first
    auto response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9996/internal/master/join",
            {{"NodeId", "SpecifiedId2"}, {"NodeUrl", "http://localhost:9997"}});
    REQUIRE(response.code == 201);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["AddedNode"] == "True");
    REQUIRE(response.body["NodeId"] == "SpecifiedId2");
    REQUIRE(response.body["NodeUrl"] == "http://localhost:9997");
    REQUIRE(response.body["Message"] == "The node will be added to the cluster");

    // Wait for the two nodes to be synchronized
    std::this_thread::sleep_for(std::chrono::seconds(20));

    // Validate that the cluster status shows the two nodes together
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9996/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["SpecifiedId1"] == "SelfInstance");
    REQUIRE(response.body["SpecifiedId2"] == "Connected");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "2/2");
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9997/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["SpecifiedId1"] == "Connected");
    REQUIRE(response.body["SpecifiedId2"] == "SelfInstance");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "2/2");

    // Try to add a node without a node id
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9996/internal/master/join",
            {{"NodeUrl", "http://localhost:9998"}});
    REQUIRE(response.code == 400);
    REQUIRE(response.body.size() == 1);
    REQUIRE(response.body["MissingArgument"] == "NodeId");

    // Try to add a node without a node url
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9996/internal/master/join",
            {{"NodeId", "SpecifiedId3"}});
    REQUIRE(response.code == 400);
    REQUIRE(response.body.size() == 1);
    REQUIRE(response.body["MissingArgument"] == "NodeUrl");

    // Try to remove a node without specifying an id
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9996/internal/master/leave", {});
    REQUIRE(response.code == 400);
    REQUIRE(response.body.size() == 1);
    REQUIRE(response.body["MissingArgument"] == "NodeId");

    // Try to remove a node which does not exist in the cluster
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9996/internal/master/leave",
            {{"NodeId", "SpecifiedId3"}});
    REQUIRE(response.code == 400);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["RemovedNode"] == "False");
    REQUIRE(response.body["NodeId"] == "SpecifiedId3");
    REQUIRE(response.body["Message"] == "No node exists with the provided ID");
}

TEST_CASE("Re-Join Master after Leaving Cluster Test", "[MasterNodeTest]")
{

    // Create three master nodes
    auto masterNode1 = std::make_shared<MasterNode>("localhost", 9996, "SpecifiedId1");
    masterNode1->start();
    auto masterNode2 = std::make_shared<MasterNode>("localhost", 9997, "SpecifiedId2");
    masterNode2->start();

    // Ensure the server has a chance to start its background
    // processes for better thread-testing
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Add the second node to the first
    auto response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9996/internal/master/join",
            {{"NodeId", "SpecifiedId2"}, {"NodeUrl", "http://localhost:9997"}});
    REQUIRE(response.code == 201);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["AddedNode"] == "True");
    REQUIRE(response.body["NodeId"] == "SpecifiedId2");
    REQUIRE(response.body["NodeUrl"] == "http://localhost:9997");
    REQUIRE(response.body["Message"] == "The node will be added to the cluster");

    // Wait for the two nodes to be synchronized
    std::this_thread::sleep_for(std::chrono::seconds(20));

    // Validate that the cluster status shows the two nodes together
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9996/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["SpecifiedId1"] == "SelfInstance");
    REQUIRE(response.body["SpecifiedId2"] == "Connected");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "2/2");
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9997/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["SpecifiedId1"] == "Connected");
    REQUIRE(response.body["SpecifiedId2"] == "SelfInstance");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "2/2");

    // Shutdown the second node
    masterNode2 = nullptr;

    // Next, have the second node formally leave the cluster
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9996/internal/master/leave",
            {{"NodeId", "SpecifiedId2"}});
    REQUIRE(response.code == 202);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["RemovedNode"] == "True");
    REQUIRE(response.body["NodeId"] == "SpecifiedId2");
    REQUIRE(response.body["Message"] == "The node will be removed from the cluster");

    // Wait for the first node to adjust accordingly
    std::this_thread::sleep_for(std::chrono::seconds(20));

    // Validate that the first node is now at quorum with one node
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9996/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["SpecifiedId1"] == "SelfInstance");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "1/1");

    // Restart the second node so it can rejoin the cluster
    masterNode2 = std::make_shared<MasterNode>("localhost", 9997, "SpecifiedId2");
    masterNode2->start();

    // Have the second node formally re-join the first again
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9996/internal/master/join",
            {{"NodeId", "SpecifiedId2"}, {"NodeUrl", "http://localhost:9997"}});

    // Wait for the two nodes to be synchronized
    std::this_thread::sleep_for(std::chrono::seconds(20));

    // Validate that the cluster status shows the two nodes together
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9996/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["SpecifiedId1"] == "SelfInstance");
    REQUIRE(response.body["SpecifiedId2"] == "Connected");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "2/2");
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9997/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["SpecifiedId1"] == "Connected");
    REQUIRE(response.body["SpecifiedId2"] == "SelfInstance");
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "2/2");
}

TEST_CASE("Single Master Multiple Worker Node Cluster Test", "[MasterNodeTest]")
{

    // Create a single master node
    auto masterNode = std::make_shared<MasterNode>("localhost", 9996, "SpecifiedId1");
    masterNode->setWorkerNodeTimeout(60);
    masterNode->start();

    // TODO: Need to check id against a regex for sha256

    // Ensure the server has a chance to start its background
    // processes for better thread-testing
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Validate that it has a quorum status
    auto response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9996/cluster/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "1/1");

    // Validate that the internal status checks-out
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9996/internal/master/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["QuorumMet"] == "True");
    REQUIRE(response.body["ClusterSize"] == "1/1");

    // Now create a worker node with a master timeout of only 30 seconds
    // for easier testing of the round-robbin feature of changing masters
    auto workerNode1 = std::make_shared<WorkerNode>("localhost", 9986, "WorkerId1");
    workerNode1->start();

    // Ensure the server has a chance to start its background
    // processes for better thread-testing
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Get the initial un-connected status of the worker node
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9986/internal/worker/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 2);
    REQUIRE(response.body["InCluster"] == "False");
    REQUIRE(response.body["ConnectedTo"] == "None");

    // Now have the worker node attempt to join one master node
    // More specifically, we'll have it join master node 1
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9986/internal/worker/join",
            {{"NodeId", "SpecifiedId1"}, {"NodeUrl", "http://localhost:9996"}});
    REQUIRE(response.code == 201);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["AddedNode"] == "True");
    REQUIRE(response.body["NodeId"] == "SpecifiedId1");
    REQUIRE(response.body["NodeUrl"] == "http://localhost:9996");
    REQUIRE(response.body["Message"] == "The node will be added to the cluster");

    // At this point, the worker should be left for a little while
    // to build-up its master-node list for fail-over purposes
    std::this_thread::sleep_for(std::chrono::seconds(20));

    // Get the status of the worker node and verify it is connected
    // to the cluster throught the single master node
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9986/internal/worker/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["SpecifiedId1"] == "0");
    REQUIRE(response.body["InCluster"] == "True");
    REQUIRE(response.body["ConnectedTo"] == "SpecifiedId1");

    // Verify that the master node list contains the new worker
    auto connectedWorkers = masterNode->getConnectedWorkers();
    REQUIRE(connectedWorkers.size() == 1);
    REQUIRE(connectedWorkers[0] == "WorkerId1");

    // Verify that the master node shows itself in quorum
    // and an empty list of other connected master nodes
    REQUIRE (masterNode->isInQuorum());
    auto connectedMasters = masterNode->getConnectedMasters();
    REQUIRE (connectedMasters.size() == 0);

    // Create two more worker nodes to connect
    auto workerNode2 = std::make_shared<WorkerNode>("localhost", 9987, "WorkerId2");
    workerNode2->start();
    auto workerNode3 = std::make_shared<WorkerNode>("localhost", 9988, "WorkerId3");
    workerNode3->start();

    // Ensure the server has a chance to start its background
    // processes for better thread-testing
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Now have the new worker nodes attempt to join one master node
    // More specifically, we'll have it join master node 1
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9987/internal/worker/join",
            {{"NodeId", "SpecifiedId1"}, {"NodeUrl", "http://localhost:9996"}});
    REQUIRE(response.code == 201);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["AddedNode"] == "True");
    REQUIRE(response.body["NodeId"] == "SpecifiedId1");
    REQUIRE(response.body["NodeUrl"] == "http://localhost:9996");
    REQUIRE(response.body["Message"] == "The node will be added to the cluster");
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9988/internal/worker/join",
            {{"NodeId", "SpecifiedId1"}, {"NodeUrl", "http://localhost:9996"}});
    REQUIRE(response.code == 201);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["AddedNode"] == "True");
    REQUIRE(response.body["NodeId"] == "SpecifiedId1");
    REQUIRE(response.body["NodeUrl"] == "http://localhost:9996");
    REQUIRE(response.body["Message"] == "The node will be added to the cluster");

    // At this point, the worker should be left for a little while
    // to build-up its master-node list for fail-over purposes
    std::this_thread::sleep_for(std::chrono::seconds(20));

    // Get the status of the worker nodes and verify they connected
    // to the cluster throught the single master node
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9987/internal/worker/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["SpecifiedId1"] == "0");
    REQUIRE(response.body["InCluster"] == "True");
    REQUIRE(response.body["ConnectedTo"] == "SpecifiedId1");
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9988/internal/worker/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["SpecifiedId1"] == "0");
    REQUIRE(response.body["InCluster"] == "True");
    REQUIRE(response.body["ConnectedTo"] == "SpecifiedId1");

    // Verify that the master node list contains the new workers
    connectedWorkers = masterNode->getConnectedWorkers();
    REQUIRE(connectedWorkers.size() == 3);
    REQUIRE(((connectedWorkers[0] == "WorkerId1")
                || (connectedWorkers[0] == "WorkerId2")
                || (connectedWorkers[0] == "WorkerId3")));
    REQUIRE(((connectedWorkers[1] == "WorkerId1")
                || (connectedWorkers[1] == "WorkerId2")
                || (connectedWorkers[1] == "WorkerId3")));
    REQUIRE(((connectedWorkers[2] == "WorkerId1")
                || (connectedWorkers[2] == "WorkerId2")
                || (connectedWorkers[2] == "WorkerId3")));
    REQUIRE(((connectedWorkers[0] != connectedWorkers[1])
                && (connectedWorkers[1] != connectedWorkers[2])
                && (connectedWorkers[2] != connectedWorkers[0])));

    // Verify that the master node shows itself in quorum
    // and an empty list of other connected master nodes
    REQUIRE (masterNode->isInQuorum());
    connectedMasters = masterNode->getConnectedMasters();
    REQUIRE (connectedMasters.size() == 0);

    // Now remove one of the workers so we can validate they time-out
    workerNode2 = nullptr;

    // Give the master node a little bit to verify we are going to
    // lose the worker node soon (but not yet)
    std::this_thread::sleep_for(std::chrono::seconds(30));

    // Verify we are still connected to all 3 workers
    connectedWorkers = masterNode->getConnectedWorkers();
    REQUIRE(connectedWorkers.size() == 3);
    REQUIRE(((connectedWorkers[0] == "WorkerId1")
                || (connectedWorkers[0] == "WorkerId2")
                || (connectedWorkers[0] == "WorkerId3")));
    REQUIRE(((connectedWorkers[1] == "WorkerId1")
                || (connectedWorkers[1] == "WorkerId2")
                || (connectedWorkers[1] == "WorkerId3")));
    REQUIRE(((connectedWorkers[2] == "WorkerId1")
                || (connectedWorkers[2] == "WorkerId2")
                || (connectedWorkers[2] == "WorkerId3")));
    REQUIRE(((connectedWorkers[0] != connectedWorkers[1])
                && (connectedWorkers[1] != connectedWorkers[2])
                && (connectedWorkers[2] != connectedWorkers[0])));

    // Give the master node a little more time to ensure the
    // worker node officially falls-off
    std::this_thread::sleep_for(std::chrono::seconds(45));

    // Verify that the master node list contains only the two
    // remaining worker nodes
    connectedWorkers = masterNode->getConnectedWorkers();
    REQUIRE(connectedWorkers.size() == 2);
    REQUIRE(((connectedWorkers[0] == "WorkerId1")
                || (connectedWorkers[0] == "WorkerId3")));
    REQUIRE(((connectedWorkers[1] == "WorkerId1")
                || (connectedWorkers[1] == "WorkerId3")));
    REQUIRE((connectedWorkers[0] != connectedWorkers[1]));

    // Verify that the master node shows itself in quorum
    // and an empty list of other connected master nodes
    REQUIRE (masterNode->isInQuorum());
    connectedMasters = masterNode->getConnectedMasters();
    REQUIRE (connectedMasters.size() == 0);
}

#endif //BITQUARK_MASTERNODE_TEST_HPP
