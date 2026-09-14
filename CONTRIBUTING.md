
![alt text](https://embeddedproto.com/wp-content/uploads/2022/04/Embedded_Proto.png "Embedded Proto Logo")


Embedded Proto is a product of Embedded AMS B.V. For more information about Embedded Proto please visit [EmbeddedProto.com](https://EmbeddedProto.com).

Copyright (C) 2020-2026 Embedded AMS B.V. - All Rights Reserved, [www.EmbeddedAMS.nl](https://www.EmbeddedAMS.nl), [info@EmbeddedAMS.nl](mailto:info@EmbeddedAMS.nl)


# Contributing

Thank you for contribruting to Embedded Proto! In this document it is shortly decribed how you can contribute to the project.

## Give your feedback

[![alt text](https://embeddedproto.com/wp-content/uploads/2022/06/feedback.png)](https://embeddedproto.com/feedback/)

## Reporting an Issue

When reporting an inssue on the Github page please include the following:
* Which version of Embedded Proto you are using.
* What behaviour you where expecting.
* What behaviour you observed.

When you have trouble compiling the code also include:
* Which compiler you are using.
* For which target you are compiling.
* Which compiler flags you are using.

## Creating a pull request

If you wish to contribute code to Embedded Proto please use the Github work flow of creating a fork and requesting a pull request. Please note we wish you to branch of the **develop branch**. Your pull request should merge back into develop.

## Building and testing

The development scripts live in `scripts/` and can be called from any directory, they change to the repository root themselves. Set up the virtual environment with `python install.py` first, the unit tests need CMake and the GTest submodule (clone with `--recursive`).

* `scripts/build_test.sh [full|partial] [nullterm]` builds the C++ unit tests, by default with full serialization.
* `scripts/run_tests.sh` runs the unit tests and only prints the failures.
* `scripts/code_coverage.sh -l` runs the tests and writes an HTML coverage report to `code_coverage_report/`.
* `scripts/build_package.sh` builds the python package into `dist/`, run it with the virtual environment active.

## Release process

Embedded Proto follows git flow. Development happens on **develop**, releases are prepared on a `release/X.Y.Z` branch and end up on **master**. Publishing is done by the GitHub Actions workflow `.github/workflows/distribute_pypi.yml`; the script `scripts/release.sh` only manages branches, version files and tags. The version is stored as plain `X.Y.Z` in `EmbeddedProto/version.json` and `src/EmbeddedProto/Version.h`, the workflow derives any suffix itself.

1. **Stage** from develop: `scripts/release.sh --version X.Y.Z --stage`. This creates `release/X.Y.Z`, sets the version files to `X.Y.Z`, commits and pushes the branch to both remotes.
2. **Beta**: every push of the release branch to the github remote (`git push github release/X.Y.Z`) runs the tests and publishes `X.Y.ZbN` to PyPI, where N counts the commits on the release branch. The workflow also creates a GitHub pre-release with the tag `X.Y.ZbN`. Beta testers install it with `pip install --pre EmbeddedProto` or `pip install EmbeddedProto==X.Y.ZbN`; a plain `pip install` never picks a beta.
3. **Release**: `scripts/release.sh --version X.Y.Z --release` merges the release branch into master and develop, tags master with `X.Y.Z`, pushes everything and deletes the release branch. The tag publishes the final `X.Y.Z` to PyPI and creates the GitHub release.

Pushes to develop publish `X.Y.Z.devN` to TestPyPI only, to keep the packaging exercised. A push to master publishes nothing, the tag does.

A version published on PyPI can never be replaced. When a beta is broken, push another commit to the release branch to get the next one. Do not force-push a release branch, the reused beta number would be rejected by PyPI.

## Requesting a feature

Please send in feature requests to: [info@EmbeddedAMS.nl](mailto:info@EmbeddedAMS.nl)
