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

#ifndef BITQUARK_RESOURCE_TEST_HPP
#define BITQUARK_RESOURCE_TEST_HPP

#include <BitBoson/BitQuark/Cluster/State/Resource.h>

using namespace BitBoson::BitQuark;

class DummyResource : public Resource
{
    private:
        std::string _v1;
        std::string _v2;
        std::string _v3;
    public:
        DummyResource(std::string v1, std::string v2, std::string v3) {
            _v1 = v1; _v2 = v2; _v3 = v3; }
        ResourceCost getResourceCost() const override {
            auto cost = ResourceCost();
            cost.setResourceSize(getFileString().size());
            cost.setMemoryRequirements(_v1.size() + _v2.size() + _v3.size());
            cost.setResourceThreads(0); return cost; }
        std::vector<std::string> getPackedVector() const override { return {_v1, _v2, _v3}; }
        bool setPackedVector(const std::vector<std::string>& pV) override {
            _v1 = pV[0]; _v2 = pV[1]; _v3 = pV[2]; return true; }
        std::string getValue1() const { return _v1; }
        std::string getValue2() const { return _v2; }
        std::string getValue3() const { return _v3; }
        virtual ~DummyResource() = default;
};

TEST_CASE("Generic Resource Test", "[ResourceTest]")
{

    // Create the dummy resource with some values
    auto dummyResource = DummyResource("Howdy", "Y'all", "!");

    // Get the packed vector from the dummy resource
    // and validate that it is correct
    auto packedVect = dummyResource.getPackedVector();
    REQUIRE (packedVect.size() == 3);
    REQUIRE (packedVect[0] == "Howdy");
    REQUIRE (packedVect[1] == "Y'all");
    REQUIRE (packedVect[2] == "!");

    // Verify the resource cost also makes sense
    auto cost = dummyResource.getResourceCost();
    REQUIRE (cost.getResourceSize() == 43);
    REQUIRE (cost.getMemoryRequirements() == 11);
    REQUIRE (cost.getResourceThreads() == 0);

    // Create a second, different dummy resource, but then
    // populate it with the packed vector from the first one
    auto dummyResource2 = DummyResource("", "", "");
    dummyResource2.setPackedVector(packedVect);

    // Get the packed vector from the second dummy resource
    // and validate that it is correct
    packedVect = dummyResource2.getPackedVector();
    REQUIRE (packedVect.size() == 3);
    REQUIRE (packedVect[0] == "Howdy");
    REQUIRE (packedVect[1] == "Y'all");
    REQUIRE (packedVect[2] == "!");

    // Verify the resource cost also makes sense
    cost = dummyResource2.getResourceCost();
    REQUIRE (cost.getResourceSize() == 43);
    REQUIRE (cost.getMemoryRequirements() == 11);
    REQUIRE (cost.getResourceThreads() == 0);

    // Verify that the two resources "equal" each other
    REQUIRE (dummyResource.getUniqueHash() == dummyResource2.getUniqueHash());

    // Create yet another dummy resource and set it up using the file-string
    auto dummyResource3 = DummyResource("", "", "");
    dummyResource3.setFileString(dummyResource2.getFileString());

    // Get the packed vector from the third dummy resource
    // and validate that it is correct
    packedVect = dummyResource3.getPackedVector();
    REQUIRE (packedVect.size() == 3);
    REQUIRE (packedVect[0] == "Howdy");
    REQUIRE (packedVect[1] == "Y'all");
    REQUIRE (packedVect[2] == "!");

    // Verify the resource cost also makes sense
    cost = dummyResource3.getResourceCost();
    REQUIRE (cost.getResourceSize() == 43);
    REQUIRE (cost.getMemoryRequirements() == 11);
    REQUIRE (cost.getResourceThreads() == 0);
}

#endif //BITQUARK_RESOURCE_TEST_HPP
