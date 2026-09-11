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

from setuptools import find_packages
from setuptools.command.build import build
from setuptools.command.editable_wheel import editable_wheel
from setuptools.command.sdist import sdist
from setuptools import setup
import subprocess
import sys
import json
import re


def build_proto():

    command = [
        sys.executable,
        "-m",
        "grpc_tools.protoc",
        "-I./EmbeddedProto",
        "--python_out=EmbeddedProto",
        "embedded_proto_options.proto",
    ]

    subprocess.run(command, check=True)


class EditableWheel(editable_wheel):
    def run(self):
        build_proto()
        super().run()


class Sdist(sdist):
    def run(self):
        build_proto()
        super().run()


VERSION_RE = r"\d+\.\d+\.\d+((a|b|rc)\d+)?(\.dev\d+)?"


def get_version():
    """Read the package version from version.json.

    The checked-in file holds the plain release version. For a beta or development build the CI
    workflow rewrites it with scripts/set_version.py before building, so this only validates the shape.
    """
    with open("EmbeddedProto/version.json") as f:
        version = json.load(f)["version"]
    if not re.fullmatch(VERSION_RE, version):
        raise SystemExit("version.json holds '%s', expected MAJOR.MINOR.PATCH with an optional PEP 440 "
                         "pre-release or dev suffix." % version)
    return version


setup(
    cmdclass={
        "editable_wheel": EditableWheel,
        "sdist": Sdist,
    },
    version=get_version(),
)
