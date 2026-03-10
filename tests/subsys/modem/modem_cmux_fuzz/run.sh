#!/bin/bash

mkdir -p corpus
./build/zephyr/zephyr.exe -max_len=32 -runs=1000000 -jobs=8 corpus
lcov --capture --directory ./ --output-file lcov.info -q --rc branch_coverage=1 --ignore-errors gcov,gcov --gcov-tool llvm-cov --gcov-tool gcov
genhtml lcov.info --output-directory lcov_html -q --ignore-errors source --branch-coverage --highlight --legend

