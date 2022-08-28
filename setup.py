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
import re


####################################################################################

def read_required_version():
    with open("requirements.txt", 'r') as f:
        lines = f.readlines()
        required_re_compiled = re.compile(r"protobuf==(?P<major>\d+)\.(?P<minor>\d+)\.(?P<patch>\d+)")
        for line in lines:
            match_req = required_re_compiled.search(line)
            if match_req:
                return match_req

        raise Exception("Unable to find protobuf version in requirements.txt.")


def check_protoc_version():
    # Protobuf has a version numbering change with version 3.21.0. The python packages got a major rewrite at this
    # point. Therefore the python package got a major version update (4.21.0). After this point, the Protoc version
    # is indicated as v21.0 without the major version. The minor version between Protoc and the python protobuf
    # package should match.

    print("Checking your Protoc version.")
    output = subprocess.run(["protoc", "--version"], check=True, capture_output=True)
    version_re_compiled = re.compile(r".*\s(?P<major>\d+)\.(?P<minor>\d+)\.(?P<patch>\d+)")
    installed_version = version_re_compiled.search(output.stdout.decode("utf-8"))
    required_version = read_required_version()

    if installed_version.group('minor') != required_version.group('minor'):
        text = "\n"
        text += "The version of Protoc (v{0}.{1})".format(installed_version.group('minor'),
                                                          installed_version.group('patch'))
        text += " you have installed is not compatible with the version of\nthe protobuf python package " \
                "(v{0}.{1}) ".format(required_version.group('minor'), required_version.group('patch'))
        text += "Embedded Proto requires. These are your options:\n" \
                "\t1. Install a matching version of Protoc.\n" \
                "\t2. Change the version of Embedded Proto.\n"

        # Check if all versions are above v21.0
        if ((21 <= int(installed_version.group('minor'))) and (21 <= int(required_version.group('minor')))) or \
                ((21 > int(installed_version.group('minor'))) and (21 > int(required_version.group('minor')))):
            while True:
                text += "\t3. Ignore the difference at your own risk!\n" \
                        "Ignore the difference [Y/n]: "

                user_input = input(text)
                if ('Y' == user_input) or ('y' == user_input):
                    # Continue the setup
                    print("Ignoring the difference.")
                    break
                elif ('N' == user_input) or ('n' == user_input):
                    # Stop the setup.
                    print("Stopping the setup.")
                    exit(1)
        else:
            # We can only stop if we have a version prior to v21.0.
            print(text)
            print("Stopping the setup.")
            exit(1)


####################################################################################

def display_feedback():
    text = "\n"
    text += "================================================================================\n"
    text += "|                                                                              |\n"
    text += "| Three simple things you can do to improve Embedded Proto:                    |\n"
    text += "|  * Give private feedback:                                                    |\n"
    text += "|        https://EmbeddedProto.com/feedback                                    |\n"
    text += "|  * Report an issue in public on Github:                                      |\n"
    text += "|        https://github.com/Embedded-AMS/EmbeddedProto/issues                  |\n"
    text += "|  * Stay up to date on Embedded Proto via our User mailing list:              |\n"
    text += "|        https://EmbeddedProto.com/signup                                      |\n"
    text += "|                                                                              |\n"
    text += "================================================================================\n"

    print(text)


####################################################################################

class ReadableDir(argparse.Action):
    # This class is used to check if the --include path provided as a parameter is a valid directory.

    def __call__(self, parser, namespace, values, option_string=None):
        prospective_dir = values
        if not os.path.isdir(prospective_dir):
            raise argparse.ArgumentTypeError("readable_dir: \"{0}\" is not a valid path".format(prospective_dir))
        if os.access(prospective_dir, os.R_OK):
            setattr(namespace, self.dest, prospective_dir)
        else:
            raise argparse.ArgumentTypeError("readable_dir: \"{0}\" is not a readable dir".format(prospective_dir))


####################################################################################

if __name__ == "__main__":

    # See on which platform we are running.
    on_windows = "Windows" == platform.system()

    parser = argparse.ArgumentParser(description="This script will setup the environment to generate Embedded Proto "
                                                 "code in your project.")
    parser.add_argument('-I', '--include', required=on_windows, action=ReadableDir,
                        help="Required on Windows. Provide the folder in which you installed the Google Protobuf "
                             "compiler Protoc. For Linux this is optional. Provide it when you installed Protoc in a "
                             "custom folder.")

    display_feedback()

    args = parser.parse_args()

    try:
        # ---------------------------------------
        check_protoc_version()

        # ---------------------------------------
        subprocess.run(["python", "-m", "venv", "venv"], check=True, capture_output=True)
        print("Creating a virtual environment for Embedded Proto.")

        # ---------------------------------------
        print("Installing requirement Python packages in the virtual environment.")
        if on_windows:
            subprocess.run(["./venv/Scripts/pip", "install", "-r", "requirements.txt"], check=True, capture_output=True)
        else:
            subprocess.run(["./venv/bin/pip", "install", "-r", "requirements.txt"], check=True, capture_output=True)

        # ---------------------------------------
        print("Build the protobuf extension file used to include Embedded Proto custom options.")
        command = ["protoc", "-I", "generator", "--python_out=generator", "embedded_proto_options.proto"]
        if args.include is not None:
            command.extend(["-I", str(args.include)])
        subprocess.run(command, check=True, capture_output=True)

        # ---------------------------------------
        print("Setup completed with success!")

    except Exception as e:
        print("Error: " + str(e))
