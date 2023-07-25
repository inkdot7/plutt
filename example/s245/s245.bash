#!/bin/bash

cd $(dirname -- "${BASH_SOURCE[0]}")

r10_proton_root=r10_470-529_beam_proton_hit_track.root

r10_frag_root=r10_53x_beam_frag_hit_track.root

[ -e $r10_proton_root ] || \
    wget http://fy.chalmers.se/subatom/advsubdet/$r10_proton_root

[ -e $r10_frag_root ] || \
    wget http://fy.chalmers.se/subatom/advsubdet/$r10_frag_root

plutt=../../plutt

$plutt -f r10_proton.plutt -r h509 $r10_proton_root &

$plutt -f r10_frag.plutt -r h509 $r10_frag_root &
