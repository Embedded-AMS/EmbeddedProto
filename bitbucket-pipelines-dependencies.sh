#!/bin/bash

if [ -d "$PWD/build-wrapper-linux-x86" ]; then
  echo "found sonar cloud wrapper cache"
else
  curl --insecure -OL https://sonarcloud.io/static/cpp/build-wrapper-linux-x86.zip
  unzip -o build-wrapper-linux-x86.zip
fi
export PATH="$PATH:$PWD/build-wrapper-linux-x86"

if [ -d "$PWD/sonar-scanner-cli-3.3.0.1492-linux" ]; then
  echo "found sonar scanner scanner cache"
else
  curl --insecure -OL https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-3.3.0.1492-linux.zip
  unzip -o sonar-scanner-cli-3.3.0.1492-linux.zip
fi
export PATH="$PATH:$PWD/sonar-scanner-cli-3.3.0.1492-linux/bin"
