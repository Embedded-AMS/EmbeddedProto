#! /bin/sh
./build/test/test_EmbeddedProto
lcov --directory ./build/test --capture --output-file ./code_coverage_report/code_coverage.info -rc lcov_branch_coverage=1
genhtml ./code_coverage_report/code_coverage.info --branch-coverage --output-directory ./code_coverage_report