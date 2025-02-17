#!/bin/bash

BLUE='\033[0;34m'
RED='\033[31m'
NC='\033[0m'

function message() {
    echo -e "${BLUE}[lab1] $1${NC}"
}

function error() {
    echo -e "${RED}[lab1] Error: $1${NC}"
}
