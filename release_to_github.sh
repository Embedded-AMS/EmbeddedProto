#! /bin/sh

#
# Copyright (C) 2020-2026 Embedded AMS B.V. - All Rights Reserved
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
# <https://embeddedproto.com/pricing/>.
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

# Function to show usage information
show_usage() {
  echo "Usage: ./release_to_github.sh [OPTIONS]"
  echo "Options:"
  echo "  --version MAJOR.MINOR.PATCH   Set version number (required)"
  echo "  --stage                       Stage changes in release branch"
  echo "  --release                     Perform release merge and tagging"
  echo ""
  echo "Example:"
  echo "  ./release_to_github.sh --version 4.0.1 --stage    # Stage changes"
  echo "  ./release_to_github.sh --version 4.0.1 --release  # Perform release"
}

# Function to parse and validate version
parse_version() {
  if ! echo "$1" | grep -E "^[0-9]+\.[0-9]+\.[0-9]+$" > /dev/null; then
    echo "Error: Version must be in format MAJOR.MINOR.PATCH"
    exit 1
  fi
  
  VERSION_MAJOR=$(echo "$1" | cut -d. -f1)
  VERSION_MINOR=$(echo "$1" | cut -d. -f2)
  VERSION_PATCH=$(echo "$1" | cut -d. -f3)
}

# Function to update version files
update_version_files() {
  # Update Version.h
  sed -i "s/#define EMBEDDEDPROTO_VERSION_MAJOR [0-9]*/#define EMBEDDEDPROTO_VERSION_MAJOR $VERSION_MAJOR/" src/Version.h
  sed -i "s/#define EMBEDDEDPROTO_VERSION_MINOR [0-9]*/#define EMBEDDEDPROTO_VERSION_MINOR $VERSION_MINOR/" src/Version.h
  sed -i "s/#define EMBEDDEDPROTO_VERSION_PATCH [0-9]*/#define EMBEDDEDPROTO_VERSION_PATCH $VERSION_PATCH/" src/Version.h
  sed -i "s/#define EMBEDDEDPROTO_VERSION_STRING \".*\"/#define EMBEDDEDPROTO_VERSION_STRING \"$VERSION\"/" src/Version.h
  
  # Update version.json
  echo "{" > EmbeddedProto/version.json
  echo "  \"version\": \"$VERSION\"" >> EmbeddedProto/version.json
  echo "}" >> EmbeddedProto/version.json
}

# Function to create staging branch
create_stage() {
  BRANCH="release/$VERSION"
  
  # Create and switch to release branch
  git checkout -b "$BRANCH"
  
  # Update version files
  update_version_files
  
  # Stage and commit changes
  git add src/Version.h EmbeddedProto/version.json
  git commit -m "Preparing for release $VERSION"
  
  # Push to both remotes
  git push origin "$BRANCH"
  git push github "$BRANCH"
}

# Function to perform release
perform_release() {
  BRANCH="release/$VERSION"
  
  # Ensure we're on the release branch
  git checkout "$BRANCH"
  
  # Merge into master
  git checkout master
  git merge --no-ff "$BRANCH" -m "Merge release $VERSION into master"
  
  # Merge into develop
  git checkout develop
  git merge --no-ff "$BRANCH" -m "Merge release $VERSION into develop"
  
  # Create version tag
  git tag -d latest
  git push --delete origin latest
  git push --delete github latest
  
  git tag latest
  git push origin latest
  git push github latest
  
  git tag "$VERSION"
  git push origin "$VERSION"
  git push github "$VERSION"
  
  # Push master and develop
  git checkout master
  git push origin master
  git push github master
  
  git checkout develop
  git push origin develop
  git push github develop
  
  # Clean up release branch
  git branch -d "$BRANCH"
  git push origin --delete "$BRANCH"
  git push github --delete "$BRANCH"
}

# Parse command line arguments
VERSION=""
DO_STAGE=false
DO_RELEASE=false

while [ "$#" -gt 0 ]; do
  case "$1" in
    --version)
      VERSION="$2"
      shift 2
      ;;
    --stage)
      DO_STAGE=true
      shift
      ;;
    --release)
      DO_RELEASE=true
      shift
      ;;
    --help)
      show_usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1"
      show_usage
      exit 1
      ;;
  esac
done

# Validate arguments
if [ -z "$VERSION" ]; then
  echo "Error: Version is required"
  show_usage
  exit 1
fi

# Parse version into components
parse_version "$VERSION"

# Fetch latest changes
git fetch --prune

# Execute requested operations
if [ "$DO_STAGE" = true ]; then
  create_stage
fi

if [ "$DO_RELEASE" = true ]; then
  perform_release
fi

# If no operation specified, show usage
if [ "$DO_STAGE" = false ] && [ "$DO_RELEASE" = false ]; then
  echo "Error: Either --stage or --release must be specified"
  show_usage
  exit 1
fi
