#! /bin/sh


./build/test/test_EmbeddedProto

# rm -rf ./code_coverage_report/*
# lcov --directory ./build/test --capture --output-file ./code_coverage_report/total_code_coverage.info -rc lcov_branch_coverage=1
# lcov --remove ./code_coverage_report/total_code_coverage.info $PWD'/external/googletest/*' '/usr/include/*' -o ./code_coverage_report/filtered_code_coverage.info
# genhtml ./code_coverage_report/filtered_code_coverage.info --branch-coverage --output-directory ./code_coverage_report

rm -rf ./code_coverage_report/*
mkdir -p code_coverage_report
cd code_coverage_report

# Run gcov on the static source files.
gcov ../test/*.cpp --object-directory ../build/test/CMakeFiles/test_EmbeddedProto.dir/test/ 
gcov ../src/*.cpp --object-directory ../build/test/CMakeFiles/test_EmbeddedProto.dir/src/

cd -