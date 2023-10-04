#!/bin/bash

# author: Pascal Bercher (pascal.bercher@anu.edu.au)
#         improved by Lijia Yuan and Pascal again; both with GPT 4.0

# the program requires two input files: domain first, then problem.
# the timeout has to be set directly in the script.



# Timeout in minutes (here, two)
TIMEOUT_SECONDS=$((2 * 60))  # this could also be made an argument of the script; but most likely it's static anyway.

handle_timeout() {
    echo "Timeout reached!"
    kill 0  # Kills the current process group
}

trap handle_timeout ALRM

function process_problem() {
    # input 1: domain file name
    # input 2: problem file name
    local domain="$1"
    local problem="$2"

    # this is added to every generated file to have a unique identifier
    # it just uses the two inputs, but you could also use another logic, for
    # example use a third argument, in which case UniqueName="${3}"
    local unique_name="--${domain}--${problem}" #could be improved by removing .hddl parts

    # Parsing
    ./pandaPIparser "$domain" "$problem" "problem${unique_name}.parsed" > "problem${unique_name}.parsed.log"
    if [[ ! -f "problem${unique_name}.parsed" ]]; then
        echo "problem${unique_name}.parsed was not generated. Exiting."
        return 1
    fi

    # Grounding
    ./pandaPIgrounder "problem${unique_name}.parsed" "problem${unique_name}.sas" 2> "problem${unique_name}.stderr.statistics" > "problem${unique_name}.stdout.statistics"
    if [[ ! -f "problem${unique_name}.sas" ]]; then
        echo "problem${unique_name}.sas was not generated. Exiting."
        return 1
    fi

    # Planner (adapt parameters as required or put them into an additional argument)
    ./pandaPIengine "problem${unique_name}.sas" -H "rc2(h=add)" -g none > "problem${unique_name}.solution"
}


(
    # Run the function in the background
    process_problem "$1" "$2"
) &

# Wait for the function or the timeout
sleep "${TIMEOUT_SECONDS}" && kill -ALRM $$
