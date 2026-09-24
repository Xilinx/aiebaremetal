#!/bin/bash
# Copyright (C) 2024 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

WORKDIR=$PWD
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/settings.sh" 1
API_BASE="${GITBASE}/api/v3"

# Other environment variables
BUGFIX_BRANCH=${GITHUB_PR_SOURCE_BRANCH}
TARGET_BRANCH=${GITHUB_PR_TARGET_BRANCH}

echo "=== Jenkins Environment Variables ==="
echo "CHANGE_ID (PR Number): ${CHANGE_ID:-not set}"
echo "CHANGE_URL (PR URL): ${CHANGE_URL:-not set}"
echo "CHANGE_BRANCH (Source Branch): ${CHANGE_BRANCH:-not set}"
echo "CHANGE_TARGET (Target Branch): ${CHANGE_TARGET:-not set}"
echo "BRANCH_NAME: ${BRANCH_NAME:-not set}"
echo "GIT_URL: ${GIT_URL:-not set}"
echo "GITHUB_PR_TRIGGER_SENDER_AUTHOR $GITHUB_PR_TRIGGER_SENDER_AUTHOR"
echo "GITHUB_PR_URL $GITHUB_PR_URL"
echo "GITHUB_PR_NUMBER $GITHUB_PR_NUMBER"

# Check if this is a PR build or a regular branch build
if [ -z "$GITHUB_PR_NUMBER" ] || [ -z "$GITHUB_PR_URL" ]; then
  echo "=== NOT A PR BUILD - Running on branch: ${BRANCH_NAME} ==="
  echo "Skipping PR merge logic, using already checked-out code..."
  # Jenkins already checked out the code, just work with it directly
  IS_PR_BUILD=false
else
  echo "=== PR BUILD DETECTED ==="
  IS_PR_BUILD=true
fi

# Only do PR-specific operations if this is a PR build
if [ "$IS_PR_BUILD" = true ]; then
  # Parse owner and repo from PR URL
  REPO_URL=$(echo $GITHUB_PR_URL | sed "s#${GITBASE}/##" | sed 's/\/pull.*//') &&
  OWNER=$(echo $REPO_URL | cut -d'/' -f1) &&
  REPO=$(echo $REPO_URL | cut -d'/' -f2) &&
  echo "OWNER: $OWNER" &&
  echo "REPO: $REPO" &&

  # Using GitHub API to fetch PR details
  echo "Fetching PR details from GitHub API..."
  # Jenkins Username/Password credentials create _PSW suffix for password
  # Use the password (which should be a PAT) for API authentication
  GITHUB_TOKEN="${GITHUB_TOKEN_AUTH_PSW:-$GITHUB_TOKEN_AUTH}"
  echo "Token type: $([ -n "$GITHUB_TOKEN_AUTH_PSW" ] && echo 'Username/Password credential' || echo 'Secret text credential')"
  PR_DETAILS=$(curl -s -H "Authorization: token $GITHUB_TOKEN" "${API_BASE}/repos/$OWNER/$REPO/pulls/$GITHUB_PR_NUMBER") &&

  # Check if API call was successful
  if [ -z "$PR_DETAILS" ] || echo "$PR_DETAILS" | jq -e '.message' > /dev/null 2>&1; then
    echo "ERROR: Failed to fetch PR details from GitHub API"
    echo "Response: $PR_DETAILS"
    echo "Check if credential is valid and has proper permissions"
    echo ""
    echo "The credential should be one of:"
    echo "  1. Secret Text credential containing a GitHub Personal Access Token (PAT)"
    echo "  2. Username/Password credential where password is a GitHub PAT"
    echo ""
    echo "To create a PAT: ${GITBASE}/settings/tokens"
    echo "Required scopes: repo (full control)"
    exit 1
  fi

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

  # Clean up any previous aie-baremetal directory
  if [ -d "aie-baremetal" ]; then
    echo "Removing existing aie-baremetal directory..."
    rm -rf aie-baremetal
  fi

  git clone $GIT_URL aie-baremetal &&
  cd aie-baremetal &&

  # Check out the bugfix branch
  git checkout $DEST_BRANCH &&
  git remote add source_repo $SOURCE_REPO
  git fetch source_repo
  MERGE_RESULT=$(git merge source_repo/$SOURCE_BRANCH 2>&1)


  if [[ $? -eq 0 ]]; then
    if [[ $DEST_BRANCH == "main-release" ]]; then
      echo "Manually clone the submodules for main-release branch."
      cd thirdparty
      git clone "${GITBASE}/embeddedsw/embeddedsw.git"
      cd aielib
      git clone "${GITBASE}/ai-engine/aie-rt.git"
      cd ../..
      echo "Done cloning submodules"
    else
      echo "Merge successful. Synchronizing submodules to source branch versions..."
      git submodule init
      git submodule update --recursive
      if [ ! -d "thirdparty/aielib/aie-rt" ]; then
        cd thirdparty/aielib
        git clone "${GITBASE}/ai-engine/aie-rt.git"
        cd ../..
      fi
      echo "Submodules synchronized to source branch versions."
    fi
  else
    echo "Merge conflict or other error when merging from master to bugfix. Manual intervention required."
    echo "$MERGE_RESULT"
    exit 1
  fi

  echo "Checked out the $DEST_BRANCH branch from the pull request's source repo." &&
  BUILD_DIR="./aie-baremetal"
else
  # For regular branch builds, use the code Jenkins already checked out
  echo "Using Jenkins checked-out code at: $WORKDIR"
  BUILD_DIR="$WORKDIR"
fi

# Navigate to build directory
cd "$BUILD_DIR"

# Set up the environment
source /proj/xbuilds/HEAD_qualified_latest/installs/lin64/HEAD/Vitis/settings64.sh
if [ -d "./test" ]; then
  cd ./test
else
  cd ./example
fi

make clean
# make ./build/libaiebaremetal.so AIE_KERNEL_LOC=./testkernel -j32

# Run simulation and capture output
set -Eeuo pipefail

set +e  
(
  set -x         # trace every command for visibility

  # Run simulation and capture output
  cd ./testkernel || exit 1

  if ! source ./build.sh 2; then  
    echo "Error in build.sh, stopping execution" >&2  
    ret=${PIPESTATUS[0]}
    exit $ret  
  fi  

  (
    exec > >(tee -a "$WORKDIR/sim_output.txt") 2>&1
    stdbuf -oL -eL bash -lc "source runsim.sh"
  )
  ret=${PIPESTATUS[0]}

  # exit the subshell with the same code
  exit $ret
)

ret=$?

cd $WORKDIR

# Use grep and awk to extract the names of the failing tests from sim_output.txt
failing_tests=($(grep -iE 'test.*fail|fail.*test' $WORKDIR/sim_output.txt 2>/dev/null))

if grep -iE 'test.*pass|pass.*test' $WORKDIR/sim_output.txt; then  
  echo "Test has passed, exiting graciously..."  
fi  

if [ ! -f "$WORKDIR/sim_output.txt" ]; then  
  echo "Error: $WORKDIR/sim_output.txt does not exist. Marking build as failed."  
  exit 1  
fi  

if [ "${#failing_tests[@]}" -gt 0 ]; then  
  echo "Failing tests: ${failing_tests[*]}. Marking build as failed."  
  exit 1  
fi  

if [ "$ret" -eq 139 ]; then  
  echo "Error: SIGSEGV (exit code $ret). Marking build as failed."  
  exit 1  
fi 

if [ "$ret" -ne 0 ]; then  
  echo "Error with exit code $ret. Marking build as failed."  
  exit 1  
fi 

echo "Simulation completed successfully. No errors found."