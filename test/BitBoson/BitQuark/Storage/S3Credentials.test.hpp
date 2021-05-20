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

#ifndef BITQUARK_S3CREDENTIALS_TEST_HPP
#define BITQUARK_S3CREDENTIALS_TEST_HPP

#include <catch.hpp>
#include <iostream>
#include <BitBoson/BitQuark/Storage/S3Credentials.h>

using namespace BitBoson::BitQuark;

TEST_CASE ("General S3-Credentials Test", "[S3CredentialsTest]")
{

    // Create S3 Credentials with some random values
    auto s3Credentials = S3Credentials("S3Endpoint", "S3Bucket",
            "S3Prefix", "S3AccessKey", "S3SecretKey");

    // Validate that the values were set
    REQUIRE(s3Credentials.getS3Endpoint() == "S3Endpoint");
    REQUIRE(s3Credentials.getBucket() == "S3Bucket");
    REQUIRE(s3Credentials.getDirectoryPrefix() == "S3Prefix");
    REQUIRE(s3Credentials.getAccessKey() == "S3AccessKey");
    REQUIRE(s3Credentials.getSecretKey() == "S3SecretKey");
}

TEST_CASE ("File-String and Hash S3-Credentials Test", "[S3CredentialsTest]")
{

    // Create S3 Credentials with some random values
    auto s3Credentials = S3Credentials("S3Endpoint", "S3Bucket",
            "S3Prefix", "S3AccessKey", "S3SecretKey");

    // Validate that the unique hash of the object
    REQUIRE(s3Credentials.getUniqueHash() == "AF1D0F5382FB02F26478B623CFCD53B04AEB966FB84226273103E31B8A8ED705");

    // Change the S3 Endpoint and verify the unique hash
    s3Credentials = S3Credentials("S3Endpoint1", "S3Bucket",
            "S3Prefix", "S3AccessKey", "S3SecretKey");
    REQUIRE(s3Credentials.getUniqueHash() == "CB41905BA354A140B97A28BCEA590834F44B67AAD953693C1EC702197D9EB9DA");

    // Change the S3 Bucket and verify the unique hash
    s3Credentials = S3Credentials("S3Endpoint", "S3Bucket1",
            "S3Prefix", "S3AccessKey", "S3SecretKey");
    REQUIRE(s3Credentials.getUniqueHash() == "E69A095CC1725B1F181EEE01ECDDE9DFD622C7382DF016CD5C89CCBF57BF2AA7");

    // Change the S3 Prefix and verify the unique hash
    s3Credentials = S3Credentials("S3Endpoint", "S3Bucket",
            "S3Prefix1", "S3AccessKey", "S3SecretKey");
    REQUIRE(s3Credentials.getUniqueHash() == "5DB5C35047E877492F79F80CACEAE017E5F2E73E490DB0D8DFBF2E68547B9905");

    // Change the S3 Access Key and verify the unique hash
    s3Credentials = S3Credentials("S3Endpoint", "S3Bucket",
            "S3Prefix", "S3AccessKey1", "S3SecretKey");
    REQUIRE(s3Credentials.getUniqueHash() == "C63AD591E3B5A47CCECA4A20A9DD2FEAAB8D71426B003A779B3D092E02DDF1BF");

    // Change the S3 Secret Key and verify the unique hash
    s3Credentials = S3Credentials("S3Endpoint", "S3Bucket",
            "S3Prefix", "S3AccessKey", "S3SecretKey1");
    REQUIRE(s3Credentials.getUniqueHash() == "CEC077AFBF20D41B5528341D4D131FF39655AFCE93C94A1940EC9D6E289FED23");
}

TEST_CASE ("Create with File-String S3-Credentials Test", "[S3CredentialsTest]")
{

    // Create S3 Credentials with some random values
    auto s3Credentials = S3Credentials("S3Endpoint", "S3Bucket",
            "S3Prefix", "S3AccessKey", "S3SecretKey");

    // Validate that the unique hash of the object
    REQUIRE(s3Credentials.getUniqueHash() == "AF1D0F5382FB02F26478B623CFCD53B04AEB966FB84226273103E31B8A8ED705");

    // Create a second credentials instance with the file-string
    // from the first credentials instance
    auto s3Credentials2 = S3Credentials(s3Credentials.getFileString());

    // Validate that the values were set
    REQUIRE(s3Credentials2.getS3Endpoint() == "S3Endpoint");
    REQUIRE(s3Credentials2.getBucket() == "S3Bucket");
    REQUIRE(s3Credentials2.getDirectoryPrefix() == "S3Prefix");
    REQUIRE(s3Credentials2.getAccessKey() == "S3AccessKey");
    REQUIRE(s3Credentials2.getSecretKey() == "S3SecretKey");
    REQUIRE(s3Credentials2.getUniqueHash() == "AF1D0F5382FB02F26478B623CFCD53B04AEB966FB84226273103E31B8A8ED705");
}

TEST_CASE ("Create with Bad File-String S3-Credentials Test", "[S3CredentialsTest]")
{

    // Create S3 Credentials with a bad file-string
    auto s3Credentials = S3Credentials("ThisIsABadFileString");

    // Validate that the default values were set
    REQUIRE(s3Credentials.getS3Endpoint().empty());
    REQUIRE(s3Credentials.getBucket().empty());
    REQUIRE(s3Credentials.getDirectoryPrefix().empty());
    REQUIRE(s3Credentials.getAccessKey().empty());
    REQUIRE(s3Credentials.getSecretKey().empty());
}

#endif //BITQUARK_S3CREDENTIALS_TEST_HPP
