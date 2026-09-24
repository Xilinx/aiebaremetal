#!/usr/bin/env bash

###############################################################################
# Copyright (C) 2025 Advanced Micro Devices, Inc.
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

# Show a single dialog (form) with:
#   1) A "Source File" field that has a "Select" button
#   2) A "Destination" text field
#   3) Two buttons at the bottom: "Run" and "Cancel"
# The "FL" type automatically provides a file chooser button
# next to the text box.

RESULT=$(yad --form \
             --title="Select Source and Destination" \
             --center \
             --width=500 \
             --field="Source File:FL" "" \
             --field="Destination:TXT" "" \
             --button="Run:0" --button="Cancel:1")

# If user pressed Cancel or closed the window, exit.
if [ $? -ne 0 ]; then
    echo "User canceled."
    exit 1
fi

# Yad returns the two fields separated by "|"
SOURCE_FILE=$(echo "$RESULT" | cut -d '|' -f 1)
DEST_FILE=$(echo "$RESULT" | cut -d '|' -f 2)

# Quick sanity check
if [ -z "$SOURCE_FILE" ] || [ -z "$DEST_FILE" ]; then
    yad --error --text="Source or Destination is empty!"
    exit 1
fi

# Now run your command using the selected source and typed destination
./run.sh "$SOURCE_FILE" -o "$DEST_FILE"

# Show a message if the command succeeded or failed
if [ $? -eq 0 ]; then
    yad --info --text="Command completed successfully!"
else
    yad --error --text="Command failed!"
fi
