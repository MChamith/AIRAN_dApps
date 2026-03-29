#!/bin/bash
################################################################################
# Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The OpenAirInterface Software Alliance licenses this file to You under
# the OAI Public License, Version 1.1  (the "License"); you may not use this
# file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.openairinterface.org/?page_id=698
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#-------------------------------------------------------------------------------
# For more information about the OpenAirInterface (OAI) Software Alliance:
#      contact@openairinterface.org
################################################################################

################################################################################
# Build script for ARM64 Docker images with E2/E3 agent support
# This script builds OAI 5G gNB and nrUE for ARM64 architecture
################################################################################

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
DOCKER_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OAI_ROOT="$(cd "${DOCKER_DIR}/.." && pwd)"
TAG="${TAG:-latest}"
E2AP_VERSION="${E2AP_VERSION:-E2AP_V3}"
KPM_VERSION="${KPM_VERSION:-KPM_V3_00}"
BUILD_OPTION="${BUILD_OPTION:---build-e3}"

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}OAI 5G ARM64 Docker Build${NC}"
echo -e "${GREEN}========================================${NC}"
echo "Docker directory: ${DOCKER_DIR}"
echo "OAI root: ${OAI_ROOT}"
echo "Tag: ${TAG}"
echo "E2AP Version: ${E2AP_VERSION}"
echo "KPM Version: ${KPM_VERSION}"
echo "Build Option: ${BUILD_OPTION}"
echo ""

cd "${OAI_ROOT}"

# Step 1: Build base image
echo -e "${YELLOW}[1/5] Building ran-base:${TAG} (ARM64 cross-compile base)...${NC}"
docker build \
    --target ran-base \
    --tag ran-base:${TAG} \
    --file docker/Dockerfile.base.ubuntu.arm64 \
    . || { echo -e "${RED}Failed to build ran-base${NC}"; exit 1; }
echo -e "${GREEN}✓ ran-base:${TAG} built successfully${NC}"
echo ""

# Step 2: Build with E2/E3 support
echo -e "${YELLOW}[2/5] Building ran-build:${TAG} (with E2AP ${E2AP_VERSION}, KPM ${KPM_VERSION}, E3 agent)...${NC}"
docker build \
    --target ran-build \
    --tag ran-build:${TAG} \
    --build-arg E2AP_VERSION=${E2AP_VERSION} \
    --build-arg KPM_VERSION=${KPM_VERSION} \
    --build-arg BUILD_OPTION="${BUILD_OPTION}" \
    --file docker/Dockerfile.build.ubuntu.arm64 \
    . || { echo -e "${RED}Failed to build ran-build${NC}"; exit 1; }
echo -e "${GREEN}✓ ran-build:${TAG} built successfully${NC}"
echo ""

# Step 3: Build gNB image
echo -e "${YELLOW}[3/5] Building oai-gnb:${TAG}-arm64 (gNB runtime)...${NC}"
docker build \
    --target oai-gnb \
    --tag oai-gnb:${TAG}-arm64 \
    --file docker/Dockerfile.gNB.ubuntu.arm64 \
    . || { echo -e "${RED}Failed to build oai-gnb${NC}"; exit 1; }
echo -e "${GREEN}✓ oai-gnb:${TAG}-arm64 built successfully${NC}"
echo ""

# Step 4: Build nrUE image
echo -e "${YELLOW}[4/5] Building oai-nr-ue:${TAG}-arm64 (nrUE runtime)...${NC}"
docker build \
    --target oai-nr-ue \
    --tag oai-nr-ue:${TAG}-arm64 \
    --file docker/Dockerfile.nrUE.ubuntu.arm64 \
    . || { echo -e "${RED}Failed to build oai-nr-ue${NC}"; exit 1; }
echo -e "${GREEN}✓ oai-nr-ue:${TAG}-arm64 built successfully${NC}"
echo ""

# Step 5: Summary
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Build Summary${NC}"
echo -e "${GREEN}========================================${NC}"
docker images | grep -E "(ran-base|ran-build|oai-gnb|oai-nr-ue)" | grep "${TAG}"
echo ""
echo -e "${GREEN}All images built successfully!${NC}"
echo ""
echo "To run the containers, use:"
echo "  docker-compose -f docker/docker-compose.arm64.yml up"
echo ""
echo "Or run individually:"
echo "  # gNB:"
echo "  docker run -it --rm --privileged oai-gnb:${TAG}-arm64 <gnb-args>"
echo ""
echo "  # nrUE:"
echo "  docker run -it --rm --privileged oai-nr-ue:${TAG}-arm64 <ue-args>"
