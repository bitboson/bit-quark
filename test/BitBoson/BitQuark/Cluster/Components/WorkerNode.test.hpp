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

#ifndef BITQUARK_WORKERNODE_TEST_HPP
#define BITQUARK_WORKERNODE_TEST_HPP

#include <catch.hpp>
#include <memory>
#include <BitBoson/BitQuark/Networking/Requests.h>
#include <BitBoson/BitQuark/Cluster/Components/MasterNode.h>
#include <BitBoson/BitQuark/Cluster/Components/WorkerNode.h>

using namespace BitBoson::BitQuark;

TEST_CASE("Single Random Worker-Node Test", "[WorkerNodeTest]")
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

    // Add the second two nodes to the first node
    auto response = Requests::makeRequest(
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

    // Create a single master node
    auto workerNode = std::make_shared<WorkerNode>("localhost", 9986);
    workerNode->start();

    // TODO: Need to check id against a regex for sha256

    // Ensure the server has a chance to start its background
    // processes for better thread-testing
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Verify that the worker node is not connected to any master nodes
    REQUIRE (!workerNode->isInQuorum());
    REQUIRE (workerNode->getConnectedMaster().empty());
    auto knownMasters = workerNode->getKnownMasters();
    REQUIRE (knownMasters.size() == 0);

    // Get the initial un-connected status of the worker node
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9986/internal/worker/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 2);
    REQUIRE(response.body["InCluster"] == "False");
    REQUIRE(response.body["ConnectedTo"] == "None");

    // Now have the worker node attempt to join one master node
    // More specifically, we'll have it join master node 2
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9986/internal/worker/join",
            {{"NodeId", "SpecifiedId2"}, {"NodeUrl", "http://localhost:9997"}});
    REQUIRE(response.code == 201);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["AddedNode"] == "True");
    REQUIRE(response.body["NodeId"] == "SpecifiedId2");
    REQUIRE(response.body["NodeUrl"] == "http://localhost:9997");
    REQUIRE(response.body["Message"] == "The node will be added to the cluster");

    // At this point, the worker should be left for a little while
    // to build-up its master-node list for fail-over purposes
    std::this_thread::sleep_for(std::chrono::seconds(20));

    // Verify that the worker node is connected to the master nodes
    REQUIRE (workerNode->isInQuorum());
    REQUIRE (workerNode->getConnectedMaster() == "SpecifiedId2");
    knownMasters = workerNode->getKnownMasters();
    REQUIRE (knownMasters.size() == 3);
    REQUIRE (knownMasters[0] == "SpecifiedId2");
    REQUIRE (knownMasters[1] == "SpecifiedId1");
    REQUIRE (knownMasters[2] == "SpecifiedId3");

    // Get the status of the worker node and verify it is still
    // connected to node 2, but that it is aware of the other
    // master nodes in the cluster
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9986/internal/worker/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 5);
    REQUIRE(response.body["SpecifiedId1"] == "0");
    REQUIRE(response.body["SpecifiedId2"] == "0");
    REQUIRE(response.body["SpecifiedId3"] == "0");
    REQUIRE(response.body["InCluster"] == "True");
    REQUIRE(response.body["ConnectedTo"] == "SpecifiedId2");
}

TEST_CASE("Multiple Master-Nodes Single Worker-Node Cluster Test", "[WorkerNodeTest]")
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

    // Add the second two nodes to the first node
    auto response = Requests::makeRequest(
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

    // Now create a worker node with a master timeout of only 30 seconds
    // for easier testing of the round-robbin feature of changing masters
    auto workerNode = std::make_shared<WorkerNode>("localhost", 9986, "WorkerId1");
    workerNode->start();
    REQUIRE(workerNode->setMasterNodeTimeout(30));

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
    // More specifically, we'll have it join master node 2
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9986/internal/worker/join",
            {{"NodeId", "SpecifiedId2"}, {"NodeUrl", "http://localhost:9997"}});
    REQUIRE(response.code == 201);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["AddedNode"] == "True");
    REQUIRE(response.body["NodeId"] == "SpecifiedId2");
    REQUIRE(response.body["NodeUrl"] == "http://localhost:9997");
    REQUIRE(response.body["Message"] == "The node will be added to the cluster");

    // At this point, the worker should be left for a little while
    // to build-up its master-node list for fail-over purposes
    std::this_thread::sleep_for(std::chrono::seconds(20));

    // Verify that the worker node is connected to the master nodes
    REQUIRE (workerNode->isInQuorum());
    REQUIRE (workerNode->getConnectedMaster() == "SpecifiedId2");
    auto knownMasters = workerNode->getKnownMasters();
    REQUIRE (knownMasters.size() == 3);
    REQUIRE (knownMasters[0] == "SpecifiedId2");
    REQUIRE (knownMasters[1] == "SpecifiedId1");
    REQUIRE (knownMasters[2] == "SpecifiedId3");

    // Get the status of the worker node and verify it is still
    // connected to node 2, but that it is aware of the other
    // master nodes in the cluster
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9986/internal/worker/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 5);
    REQUIRE(response.body["SpecifiedId1"] == "0");
    REQUIRE(response.body["SpecifiedId2"] == "0");
    REQUIRE(response.body["SpecifiedId3"] == "0");
    REQUIRE(response.body["InCluster"] == "True");
    REQUIRE(response.body["ConnectedTo"] == "SpecifiedId2");

    // Since the worker is currently connected to master node 2
    // we will bring master node 2 down to force the worker to
    // iterate to one of the other two master nodes
    // NOTE: We have no way of knowing which node it will pick
    masterNode2 = nullptr;

    // Wait for the worker node to switch-over to a new master node
    std::this_thread::sleep_for(std::chrono::seconds(60));

    // Verify that the worker node is connected to the master nodes
    // but has switched due to master node 2 dropping-off
    REQUIRE (workerNode->isInQuorum());
    REQUIRE (((workerNode->getConnectedMaster() == "SpecifiedId1")
            || (workerNode->getConnectedMaster() == "SpecifiedId3")));
    knownMasters = workerNode->getKnownMasters();
    REQUIRE (knownMasters.size() == 3);
    REQUIRE (knownMasters[0] == "SpecifiedId2");
    REQUIRE (knownMasters[1] == "SpecifiedId1");
    REQUIRE (knownMasters[2] == "SpecifiedId3");

    // Get the status of the worker node and verify it is still
    // connected to the cluster, but now is connected to either
    // master node 1 or master node 3 and that master node 2 has
    // a non-zero connection time value
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9986/internal/worker/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 5);
    REQUIRE(response.body["SpecifiedId1"] == "0");
    REQUIRE(response.body["SpecifiedId2"] != "0");
    REQUIRE(response.body["SpecifiedId3"] == "0");
    REQUIRE(response.body["InCluster"] == "True");
    REQUIRE(((response.body["ConnectedTo"] == "SpecifiedId1")
            || (response.body["ConnectedTo"] == "SpecifiedId3")));

    // Shut-down master node 1 so we can verify the worker is no
    // longer connected since the last master isn't in quorum
    masterNode1 = nullptr;

    // Wait for the worker node to switch-over to a new master node
    std::this_thread::sleep_for(std::chrono::seconds(60));

    // Verify that the worker node is not connected to the master nodes
    // but has switched due to master node 2 and 3 dropping-off
    REQUIRE (!workerNode->isInQuorum());
    REQUIRE (workerNode->getConnectedMaster() == "SpecifiedId3");
    knownMasters = workerNode->getKnownMasters();
    REQUIRE (knownMasters.size() == 3);
    REQUIRE (knownMasters[0] == "SpecifiedId2");
    REQUIRE (knownMasters[1] == "SpecifiedId1");
    REQUIRE (knownMasters[2] == "SpecifiedId3");

    // Verify that the node is no longer "in the cluster"
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9986/internal/worker/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 5);
    REQUIRE(response.body["InCluster"] == "False");

    // Observe the worker node for a few minutes to ensure it tries to
    // "connect" to every master node at least once in an attempt to
    // find one which is contactable with quorum
    std::unordered_map<std::string, int> connectionMap;
    connectionMap["SpecifiedId1"] = 0;
    connectionMap["SpecifiedId2"] = 0;
    connectionMap["SpecifiedId3"] = 0;
    for (int ii = 0; ii < 36; ii++)
    {

        // Verify that the node is still longer "in the cluster"
        // and note which node it is "connected to"
        response = Requests::makeRequest(
                Servable::HttpMethod::GET, "http://localhost:9986/internal/worker/status", {});
        REQUIRE(response.code == 200);
        REQUIRE(response.body.size() == 5);
        REQUIRE(response.body["InCluster"] == "False");
        connectionMap[response.body["ConnectedTo"]]++;

        // Delay for 5 seconds between checks/refreshes
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    // Verify that each master node was "connected to" at least once
    REQUIRE(connectionMap["SpecifiedId1"] > 0);
    REQUIRE(connectionMap["SpecifiedId2"] > 0);
    REQUIRE(connectionMap["SpecifiedId3"] > 0);

    // Bring back up both master nodes to ensure the cluster stabilizes again
    masterNode1 = std::make_shared<MasterNode>("localhost", 9996, "SpecifiedId1");
    masterNode1->start();
    masterNode2 = std::make_shared<MasterNode>("localhost", 9997, "SpecifiedId2");
    masterNode2->start();

    // Wait for a while for everything to sync back up
    std::this_thread::sleep_for(std::chrono::seconds(60));

    // Verify that the worker node is connected to the master nodes
    // and is connected to one of the master nodes (can be any)
    REQUIRE (workerNode->isInQuorum());
    REQUIRE (((workerNode->getConnectedMaster() == "SpecifiedId1")
            || (workerNode->getConnectedMaster() == "SpecifiedId2")
            || (workerNode->getConnectedMaster() == "SpecifiedId3")));
    knownMasters = workerNode->getKnownMasters();
    REQUIRE (knownMasters.size() == 3);

    // Verify that the master nodes are re-connected
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

    // Verify that the worker has re-joined as well
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9986/internal/worker/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 5);
    REQUIRE(response.body["InCluster"] == "True");
    REQUIRE(((response.body["ConnectedTo"] == "SpecifiedId1")
            || (response.body["ConnectedTo"] == "SpecifiedId2")
            || (response.body["ConnectedTo"] == "SpecifiedId3")));
    REQUIRE(response.body[response.body["ConnectedTo"]] == "0");

    // Next, we will remove one manager node from the cluster to
    // verify that the worker also stops keeing track of it
    masterNode2 = nullptr;
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9996/internal/master/leave",
            {{"NodeId", "SpecifiedId2"}});
    REQUIRE(response.code == 202);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["RemovedNode"] == "True");
    REQUIRE(response.body["NodeId"] == "SpecifiedId2");
    REQUIRE(response.body["Message"] == "The node will be removed from the cluster");
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9998/internal/master/leave",
            {{"NodeId", "SpecifiedId2"}});
    REQUIRE(response.code == 202);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["RemovedNode"] == "True");
    REQUIRE(response.body["NodeId"] == "SpecifiedId2");
    REQUIRE(response.body["Message"] == "The node will be removed from the cluster");

    // Wait for the worker to figure out that the manager has left
    std::this_thread::sleep_for(std::chrono::seconds(30));

    // Verify that the worker no longer tracks the second manager node
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9986/internal/worker/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 4);
    REQUIRE((!response.body["SpecifiedId1"].empty()));
    REQUIRE((response.body["SpecifiedId2"].empty()));
    REQUIRE((!response.body["SpecifiedId3"].empty()));
    REQUIRE(response.body["InCluster"] == "True");
    REQUIRE(((response.body["ConnectedTo"] == "SpecifiedId1")
            || (response.body["ConnectedTo"] == "SpecifiedId3")));
    REQUIRE(response.body[response.body["ConnectedTo"]] == "0");
}

TEST_CASE("Post a Bad Worker Join Cluster Test", "[WorkerNodeTest]")
{

    // Create three master nodes
    auto masterNode = std::make_shared<MasterNode>("localhost", 9996, "SpecifiedId1");
    masterNode->start();
    auto workerNode = std::make_shared<WorkerNode>("localhost", 9986, "WorkerNode1");
    workerNode->start();

    // Ensure the server has a chance to start its background
    // processes for better thread-testing
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Add the worker node to the master node
    auto response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9986/internal/worker/join",
            {{"NodeId", "SpecifiedId1"}, {"NodeUrl", "http://localhost:9996"}});
    REQUIRE(response.code == 201);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["AddedNode"] == "True");
    REQUIRE(response.body["NodeId"] == "SpecifiedId1");
    REQUIRE(response.body["NodeUrl"] == "http://localhost:9996");
    REQUIRE(response.body["Message"] == "The node will be added to the cluster");

    // Wait for the two nodes to be synchronized
    std::this_thread::sleep_for(std::chrono::seconds(20));

    // Validate that the cluster status shows the worker connected
    response = Requests::makeRequest(
            Servable::HttpMethod::GET, "http://localhost:9986/internal/worker/status", {});
    REQUIRE(response.code == 200);
    REQUIRE(response.body.size() == 3);
    REQUIRE(response.body["SpecifiedId1"] == "0");
    REQUIRE(response.body["InCluster"] == "True");
    REQUIRE(response.body["ConnectedTo"] == "SpecifiedId1");

    // Try to add a node without a node id
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9986/internal/worker/join",
            {{"NodeUrl", "http://localhost:9997"}});
    REQUIRE(response.code == 400);
    REQUIRE(response.body.size() == 1);
    REQUIRE(response.body["MissingArgument"] == "NodeId");

    // Try to add a node without a node url
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9986/internal/worker/join",
            {{"NodeId", "SpecifiedId2"}});
    REQUIRE(response.code == 400);
    REQUIRE(response.body.size() == 1);
    REQUIRE(response.body["MissingArgument"] == "NodeUrl");

    // Try to add a node with an id that matches the recipient
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9986/internal/worker/join",
            {{"NodeId", "WorkerNode1"}, {"NodeUrl", "http://localhost:9997"}});
    REQUIRE(response.code == 400);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["AddedNode"] == "False");
    REQUIRE(response.body["NodeId"] == "WorkerNode1");
    REQUIRE(response.body["NodeUrl"] == "http://localhost:9997");
    REQUIRE(response.body["Message"] == "A node with the same ID already exists");

    // Try to add a node with a url that matches the recipient
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9986/internal/worker/join",
            {{"NodeId", "SpecifiedId2"}, {"NodeUrl", "http://localhost:9986"}});
    REQUIRE(response.code == 400);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["AddedNode"] == "False");
    REQUIRE(response.body["NodeId"] == "SpecifiedId2");
    REQUIRE(response.body["NodeUrl"] == "http://localhost:9986");
    REQUIRE(response.body["Message"] == "A node with the same URL already exists");

    // Try to add a node with an id that matches an existing node
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9986/internal/worker/join",
            {{"NodeId", "SpecifiedId1"}, {"NodeUrl", "http://localhost:9997"}});
    REQUIRE(response.code == 400);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["AddedNode"] == "False");
    REQUIRE(response.body["NodeId"] == "SpecifiedId1");
    REQUIRE(response.body["NodeUrl"] == "http://localhost:9997");
    REQUIRE(response.body["Message"] == "A node with the same ID already exists");

    // Try to add a node with a url that matches an existing node
    response = Requests::makeRequest(
            Servable::HttpMethod::POST, "http://localhost:9986/internal/worker/join",
            {{"NodeId", "SpecifiedId2"}, {"NodeUrl", "http://localhost:9996"}});
    REQUIRE(response.code == 400);
    REQUIRE(response.body.size() == 4);
    REQUIRE(response.body["AddedNode"] == "False");
    REQUIRE(response.body["NodeId"] == "SpecifiedId2");
    REQUIRE(response.body["NodeUrl"] == "http://localhost:9996");
    REQUIRE(response.body["Message"] == "A node with the same URL already exists");
}

#endif //BITQUARK_WORKERNODE_TEST_HPP
