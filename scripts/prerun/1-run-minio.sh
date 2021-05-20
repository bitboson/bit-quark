#!/bin/bash

# Ensure errors are propagated
set -e

# Only attempt to execute any MinIO operations if it isn't already running
if [ "$(pgrep -c "minio")" -le 1 ]; then

  # Execute the MinIO executible to run the service locally
  rm -rf ~/data
  mkdir -p ~/data
  chmod +x scripts/refs/minio
  MINIO_ACCESS_KEY="AKIAIOSFODNN7EXAMPLE" MINIO_SECRET_KEY="wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY" scripts/refs/minio server ~/data &

  # Sleep for a few seconds, then wait until the server responds
  set +e
  wget http://localhost:9000
  set -e

  # Run the MinIO client to create the test bucket "test-bucket"
  rm -rf ~/.mc
  chmod +x scripts/refs/mc
  scripts/refs/mc config host add s3 http://localhost:9000 AKIAIOSFODNN7EXAMPLE wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY
  scripts/refs/mc mb s3/test-bucket
fi
