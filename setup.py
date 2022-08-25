#
# Copyright (C) 2020-2022 Embedded AMS B.V. - All Rights Reserved
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
#   Johan Huizingalaan 763a
#   1066 VH, Amsterdam
#   the Netherlands
#

import subprocess
import argparse
import platform
import os


# This class is used to check if the --include path provided as a parameter is a valid directory.
class ReadableDir(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        prospective_dir = values
        if not os.path.isdir(prospective_dir):
            raise argparse.ArgumentTypeError("readable_dir:{0} is not a valid path".format(prospective_dir))
        if os.access(prospective_dir, os.R_OK):
            setattr(namespace, self.dest, prospective_dir)
        else:
            raise argparse.ArgumentTypeError("readable_dir:{0} is not a readable dir".format(prospective_dir))


####################################################################################

if __name__ == "__main__":

    # See on which platform we are running.
    on_windows = "Windows" == platform.system()

    parser = argparse.ArgumentParser(description="This script will setup the environment to generate source code in "
                                                 "your project.")
    parser.add_argument('-I', '--include', required=on_windows, action=ReadableDir,
                        help="Required on Windows. Provide the folder in which you installed the Google Protobuf "
                             "compiler protoc. For Linux this is optional if you installed protoc in a custom folder.")

    args = parser.parse_args()

    try:
        print("Creating a virtual environment for Embedded Proto.")
        subprocess.run(["python", "-m", "venv", "venv"], check=True)

        print("Installing requirement Python packages in the virtual environment.")
        if on_windows:
            subprocess.run(["./venv/Scripts/pip", "install", "-r", "requirements.txt"], check=True)
        else:
            subprocess.run(["./venv/bin/pip", "install", "-r", "requirements.txt"], check=True)

        print("Build the protobuf extension file used to include Embedded Proto custom options.")
        command = ["protoc", "-I", "generator", "--python_out=generator", "embedded_proto_options.proto"]
        if args.include is not None:
            command.extend(["-I", str(args.include)])
        subprocess.run(command, check=True)

        print("Setup completed without errors.")

    except Exception as e:
        print("Error: " + str(e))
