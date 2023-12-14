#! /bin/sh

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


if [ $# -eq 0 ]; then
  echo "Please provice version number: \"./release_to_github.sh X.Y.Z\""
else
  git fetch --prune
  
  git checkout develop
  git pull
  git push github develop

  git checkout master
  git pull
  git push github master

  git tag -d latest
  git push --delete origin latest
  git push --delete github latest

  git tag latest
  git push origin latest
  git push github latest

  git tag "$1"
  git push origin "$1"
  git push github "$1"
fi