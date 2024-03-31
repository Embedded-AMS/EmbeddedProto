#! /bin/bash

#
# Copyright (C) 2020-2024 Embedded AMS B.V. - All Rights Reserved
#
# This file is part of Embedded Proto.
#
# Embedded Proto is open source software: you can redistribute it and/or 
# modify it under the terms of the GNU General Public License as published 
# by the Free Software Foundation, version 3 of the license.
#
# Embedded Proto  is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Embedded Proto. If not, see <https://www.gnu.org/licenses/>.
#
# For commercial and closed source application please visit:
# <https://EmbeddedProto.com/license/>.
#
# Embedded AMS B.V.
# Info:
#   info at EmbeddedProto dot com
#
# Postal address:
#   Atoomweg 2
#   1627 LE, Hoorn
#   the Netherlands
#

# Run the sonar scanner locally.
source bitbucket-pipelines-dependencies.sh
./build_test.sh
./code_coverage.sh

cd build/test
build-wrapper-linux-x86-64 --out-dir SonarQube-output make clean all

cd -
export SONAR_SCANNER_OPTS="-Xmx2024m"
sonar-scanner -Dproject.settings=sonar-project.properties -Dsonar.login=aa1c8fec928ed71e4f94bf6b30444cf8192f8191