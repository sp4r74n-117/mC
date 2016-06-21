#!/bin/bash
BASE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
$BASE/mC --no-unit-tests --libs $BASE/lib/ "$@"
