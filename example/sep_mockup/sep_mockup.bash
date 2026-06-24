#!/bin/bash

cd $(dirname -- "${BASH_SOURCE[0]}")

test_20250624_1_stitched_root=test_20250624_1.stitched.root

[ -e $test_20250624_1_stitched_root ] || \
    wget http://fy.chalmers.se/subatom/subexp-daq/sep_mockup/$test_20250624_1_stitched_root

plutt=../../plutt

$plutt -f ana1.plutt -r h101 $test_20250624_1_stitched_root &
