#!/bin/bash
ninja opt -C ../build && ../build/bin/opt a.ll -passes=edan75 -S -o b.ll

