#!/bin/bash

program_name="conway"
arr_size="$1"
timesteps="$2"

if [[ -z "$program_name" || -z "$arr_size" || -z "$timesteps" ]]; then
    echo "Usage: $0 <arr_size> <timesteps> "
    exit 1
fi

make


# Run the program with different number of threads 1, 2, 4 , 8 and 16
for (( i=0; i<=4; i++ )); do 
    echo ""
    echo "#------------------------------------------------#"
    echo "Running program $program_name with "$(( 2 ** $i))" threads."
    echo "#------------------------------------------------#"
    "./$program_name" "$arr_size" "$timesteps" "$(( 2 ** $i))"
done