#!/bin/bash

###############################################################################
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"). You may
# not use this file except in compliance with the License. A copy of the
# License is located at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
###############################################################################

# Other environment variables
BUGFIX_BRANCH=${GITHUB_PR_SOURCE_BRANCH}
TARGET_BRANCH=${GITHUB_PR_TARGET_BRANCH}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
source "${REPO_ROOT}/script/settings.sh" 1
API_BASE="${GITBASE}/api/v3"


echo "GITHUB_PR_TRIGGER_SENDER_AUTHOR $GITHUB_PR_TRIGGER_SENDER_AUTHOR"
echo "GITHUB_PR_URL $GITHUB_PR_URL"
echo "GITHUB_PR_NUMBER $GITHUB_PR_NUMBER"

# Parse owner and repo from PR URL
REPO_URL=$(echo $GITHUB_PR_URL | sed "s#${GITBASE}/##" | sed 's/\/pull.*//') &&
OWNER=$(echo $REPO_URL | cut -d'/' -f1) &&
REPO=$(echo $REPO_URL | cut -d'/' -f2) &&
echo "OWNER: $OWNER" &&
echo "REPO: $REPO" &&

# Using GitHub API to fetch PR details
PR_DETAILS=$(curl -s -H "Authorization: token $GITHUB_TOKEN_AUTH_ARUN" "${API_BASE}/repos/$OWNER/$REPO/pulls/$GITHUB_PR_NUMBER") &&

# Extracting source and destination repo and branch details
SOURCE_REPO=$(echo $PR_DETAILS | jq -r '.head.repo.full_name') &&
SOURCE_BRANCH=$(echo $PR_DETAILS | jq -r '.head.ref') &&
DEST_REPO=$(echo $PR_DETAILS | jq -r '.base.repo.full_name') &&
DEST_BRANCH=$(echo $PR_DETAILS | jq -r '.base.ref') &&

# Print the details
echo "Source Repository: $SOURCE_REPO" &&
echo "Source Branch: $SOURCE_BRANCH" &&
echo "Destination Repository: $DEST_REPO" &&
echo "Destination Branch: $DEST_BRANCH" &&

# Check out the bugfix/source branch
GIT_URL="${GITBASE}/$DEST_REPO.git" &&
SOURCE_REPO="${GITBASE}/$SOURCE_REPO.git"

git clone $GIT_URL aie-baremetal &&
cd aie-baremetal &&

# Check out the bugfix branch
git checkout $DEST_BRANCH &&
git remote add source_repo $SOURCE_REPO
git fetch source_repo
MERGE_RESULT=$(git merge source_repo/$SOURCE_BRANCH 2>&1)

if [[ $? -ne 0 ]]; then
    echo "Merge conflict or other error when merging from master to bugfix. Manual intervention required."
    echo "$MERGE_RESULT"
    exit 1
fi

echo "Checked out the $DEST_BRANCH branch from the pull request's source repo." &&

mkdir -p thirdparty
cd thirdparty
git clone --recursive "${GITBASE}/ai-engine/aiebaremetal.git" aiebaremetal
cd ..

# source ./test_aiebaremetal/script/settings.sh

# source /proj/xbuilds/HEAD_qualified_latest/installs/lin64/HEAD/Vitis/settings64.sh

source setup.sh
mkdir build
cd build
cmake ../
make
cd ../data
source ../thirdparty/aiebaremetal/script/settings.sh
../convert.sh ./graph.cc -compile -aie_version 2 -out ./graph_new.cc | tee cmd_output.txt

# Use grep and awk to extract the names of the failing tests from sim_output.txt
failing_tests=($(grep ' failed' cmd_output.txt 2>/dev/null | awk '{print $(NF-1)}'))
if [ ${#failing_tests[@]} -eq 0 ] && [ ! -f sim_output.txt ]; then
  echo "Error: cmd_output.txt does not exist. Marking build as failed."
  exit 1
fi