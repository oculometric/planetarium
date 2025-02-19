#! /bin/bash

all_files=$(find inc/scenegraph/ -wholename '*node.h')
class_lines=$(echo "$all_files" | xargs grep -w -o -h -E "^class PT[[:alnum:]]+ : public PT[[:alnum:]]*Node")
target_files=$(echo "$all_files" | xargs grep -w -l -E "^class PT[[:alnum:]]+ : public PT[[:alnum:]]*Node")
type_names='Node'
node_list=$'\tINSTANTIATE_FUNC(Node)'
include_list=$'#include "node.h"'

while IFS= read -r line; do
    tmp=$(echo "$line" | grep -w -o -h -E "^class PT[[:alnum:]]+ :")
    tmp=${tmp:8}
    type_name=${tmp::-2}
    type_names="$type_names"$'\n'"$type_name"
    node_list="$node_list"$',\n\tINSTANTIATE_FUNC('"$type_name"')'
done <<< "$class_lines"

while IFS= read -r line; do
    include_list="$include_list"$'\n#include \"'"${line:4}"$'\"'
done <<< "$target_files"

nodes_list_generated=$'#pragma once\n\n'"$include_list"$'\n\nstatic map<string, PTNodeInstantiateFunc> node_instantiators = \n{\n'"$node_list"$'\n};'

if [[ "$(< inc/node_list.generated.h)" != "$nodes_list_generated" ]]; then
    echo "Updating tracked node types: "$type_names
    echo "$nodes_list_generated" > inc/node_list.generated.h
fi
