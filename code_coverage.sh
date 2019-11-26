#! /bin/sh

rm -rf ./code_coverage_report/*

./build/test/test_EmbeddedProto

lcov --directory ./build/test --capture --output-file ./code_coverage_report/total_code_coverage.info -rc lcov_branch_coverage=1
lcov --remove ./code_coverage_report/total_code_coverage.info $PWD'/external/googletest/*' '/usr/include/*' -o ./code_coverage_report/filtered_code_coverage.info

genhtml ./code_coverage_report/filtered_code_coverage.info --branch-coverage --output-directory ./code_coverage_report