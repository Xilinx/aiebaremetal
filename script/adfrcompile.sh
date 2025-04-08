#!/bin/bash
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

# Check if a filename is provided
if [ -z "$1" ]; then
  echo "Please provide a filename as an argument."
  exit 1
fi

generate_timestamp() {
  date +"%Y%m%d_%H"
}

copy_file_with_timestamp() {
  local file="$1"
  local timestamp
  timestamp=$(generate_timestamp)
  
  local base_name
  base_name=$(basename "$file")
  
  local dir_name
  dir_name=$(dirname "$file")
  
  local new_file="${dir_name}/${base_name%.*}_$timestamp.${base_name##*.}"
  
  cp "$file" "$new_file"
  echo "$new_file"
}

file=$(copy_file_with_timestamp "$1")

transform_file() {
	# Get the filename

	# Define the class name to search for
	class_name="AdfrGraph"

	# Define the macro
	macro="(defined(__AIESIM__) || defined(__ADF_FRONTEND__))"

	# Search the specified file for the AdfrGraph constructor
	if grep -qE "${class_name}\s+\w+\s*\(\s*\"[^\"]*\"\s*,\s*\"[^\"]*\"\s*,\s*\"[^\"]*\"\s*\)" "$file"; then
			
			# Use grep and sed to extract gclassname, variable name, and gheader
			grep -Eo "${class_name}\s+\w+\s*\(\s*\"[^\"]*\"\s*,\s*\"[^\"]*\"\s*,\s*\"[^\"]*\"\s*\)" "$file" | while read -r line; do
					variable_name=$(echo "$line" | sed -E 's/.*\(\s*"([^"]*)".*/\1/')
					gclassname=$(echo "$line" | sed -E 's/.*\(\s*"[^"]*"\s*,\s*"([^"]*)".*/\1/')
					gheader=$(echo "$line" | sed -E 's/.*\(\s*"[^"]*"\s*,\s*"[^"]*"\s*,\s*"([^"]*)".*/\1/')

					# Check if the #include directive is already present within the GRAPH_COMPILE_OR_SIM macro
					if ! grep -qE "#include \"$gheader\"" "$file"; then
							sed -i "1i#if $macro\n#include \"$gheader\"\n#endif" "$file"
							echo "Added #include \"$gheader\" to $file within $macro"
					else
							echo "#include \"$gheader\" already exists in $file within $macro"
					fi

					# Check if the global class declaration is already present within the GRAPH_COMPILE_OR_SIM macro
					if ! grep -qE "$gclassname\s+$variable_name;" "$file"; then
							sed -i "/#include \"$gheader\"/a $gclassname $variable_name;" "$file"
							echo "Added global $gclassname $variable_name; to $file within $macro"
					else
							echo "Global $gclassname $variable_name; already exists in $file within $macro"
					fi

					# Add the init() call immediately after the first '{' following the main function
					if ! grep -qE "$variable_name.init\(\);" "$file"; then
							#sed -i "/\s\+main\s*(.*)/{
							#    n; # Skip the line with '{'
							#		s/{/{\n#ifdef $macro\n $variable_name.init();\n#endif/
							#}" "$file"
							sed -i -e '/int\s\+main\s*(.*)/{N; :a; N; /{/!ba; /{/a\'$'\n'"#if $macro\n $variable_name.init();\n#endif" -e '}' "$file"

							echo "Added $variable_name.init(); to $file within $macro"
					else
							echo "$variable_name.init(); already exists in $file within $macro"
					fi
			done
	else
			echo "No matching constructor found in the file."
	fi
}

transform_file "$@"
#remove first parameter
shift
if [ "$#" -ge 1 ]; then
	echo "aiecompiler $file $@"
	aiecompiler "$file" "$@"
#	if [ -f "$file" ]; then
#    rm "$file"
#	fi
fi
