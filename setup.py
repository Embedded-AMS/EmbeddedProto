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

from setuptools import find_packages
from setuptools.command.build import build
from setuptools.command.editable_wheel import editable_wheel
from setuptools.command.sdist import sdist
from setuptools import setup
import subprocess
import os


def build_proto():
    command = [
        "protoc",
        "-I",
        ".",
        "--python_out=EmbeddedProto",
        "embedded_proto_options.proto",
    ]

    if "EMBEDDEDPROTO_PROTOC_INCLUDE" in os.environ:
        command.extend(["-I", os.environ["EMBEDDEDPROTO_PROTOC_INCLUDE"]])

    subprocess.run(command, check=True)


class Build(build):
    def run(self):
        build_proto()
        super().run()


class EditableWheel(editable_wheel):
    def run(self):
        build_proto()
        super().run()


class Sdist(sdist):
    def run(self):
        build_proto()
        super().run()


setup(
    cmdclass={
        "build": Build,
        "editable_wheel": EditableWheel,
        "sdist": Sdist,
    },
)
