#!/bin/bash

if [ -d "$PWD/protoc" ]; then
  echo "found protoc cache"
else
  curl --insecure -OL https://github.com/protocolbuffers/protobuf/releases/download/v3.11.0/protoc-3.11.0-linux-x86_64.zip
  unzip -o protoc-3*.zip -d protoc/
fi
export PATH="$PATH:$PWD/protoc/bin"

if [ -d "$PWD/build-wrapper" ]; then
  echo "found sonar cloud wrapper cache"
else
  curl --insecure -OL https://sonarcloud.io/static/cpp/build-wrapper-linux-x86.zip
  unzip -o build-wrapper-linux-x86.zip
  mv build-wrapper-linux-x86/ build-wrapper/
  # The following is required for my local machine.
  cp build-wrapper/libinterceptor-x86_64.so build-wrapper/libinterceptor-haswell.so
fi
export PATH="$PATH:$PWD/build-wrapper"

if [ -d "$PWD/sonar-scanner/" ]; then
  echo "found sonar scanner scanner cache"
else
  curl --insecure -OL https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-4.2.0.1873-linux.zip
  unzip -o sonar-scanner-cli-*-linux.zip
  mv sonar-scanner*/ sonar-scanner/
fi
export PATH="$PATH:$PWD/sonar-scanner/bin"

if [ -d "$PWD/venv" ]; then
  echo "found python virtualenv in cache"
else
  python3 -m venv venv
fi
source venv/bin/activate
pip install -r requirements.txt