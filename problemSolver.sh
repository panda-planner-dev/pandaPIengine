#!/bin/bash

#author: Pascal Bercher (pascal.bercher@anu.edu.au)

./pandaPIparser $1 $2 problem$3.parsed > problem$3.parsed.log
./pandaPIgrounder problem$3.parsed problem$3.sas 2> problem$3.stderr.statistics > problem$3.stdout.statistics
./pandaPIengine problem$3.sas > problem$3.solution
