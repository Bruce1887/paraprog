#!/bin/bash

# Get the program name, number of runs, and option
program_name="excercise_4"
option="$1"
num_runs="$2"

# Check if the program name and number of runs are provided
if [[ -z "$program_name" || -z "$num_runs" ]]; then
    echo "Usage: $0 <option (integer 1-5)> <num_runs>..."
    exit 1
fi

# Check if the option is a valid integer between 1 and 5
if [[ ! "$option" =~ ^[1-5]$ ]]; then
    echo "Invalid option: $option"
    exit 1
fi

make

# Run the program the specified number of times
for (( i=1; i<=num_runs; i++ )); do
    echo ""
    echo "#------------------------------------------------#"
    echo "Running program $program_name with "$(( 2 ** $i))" threads."
    echo "#------------------------------------------------#"
    "./$program_name" "$option" "$(( 2 ** $i))"

done