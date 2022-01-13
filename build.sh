#!/bin/bash

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color


if   [[ $1 == 1 ]]
then
    echo -e "${GREEN}malloc_1.cpp${NC}"
    g++ -g -Wall -c malloc_1.cpp
elif [[ $1 == 2 ]]
then
    echo -e "${GREEN}malloc_2.cpp${NC}"
    g++ -g -Wall -c malloc_2.cpp
elif [[ $1 == 3 ]]
then
    echo -e "${GREEN}malloc_3.cpp${NC}"
    g++ -g -Wall -c malloc_3.cpp
elif [[ $1 == 4 ]]
then
    echo -e "${GREEN}malloc_4.cpp${NC}"
    g++ -g -Wall -c malloc_4.cpp
elif [[ $1 == "all" ]]
then
    echo -e "${GREEN}Compile all four files${NC}"
    g++ -g -Wall -c malloc_1.cpp
    g++ -g -Wall -c malloc_2.cpp
    g++ -g -Wall -c malloc_3.cpp
    g++ -g -Wall -c malloc_4.cpp
else
    echo -e "${RED}Invalid input!${NC}"
    echo -e "Usage: ./build.sh <ARG>"
    echo -e "ARG Mapping:"
    echo -e "\tARG = 1 - compile malloc_1.cpp"
    echo -e "\tARG = 2 - compile malloc_2.cpp"
    echo -e "\tARG = 3 - compile malloc_3.cpp"
    echo -e "\tARG = 4 - compile malloc_4.cpp"
    echo -e "\tARG = all - compile malloc_N.cpp for N = 1, 2, 3, 4"
fi