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
from sys import version
from sys import stderr
import venv

# Perform a system call to beable to display colors on windows
os.system("")

CGREEN = '\33[92m'
CRED = '\33[91m'
CYELLOW = '\33[93m'
CEND = '\33[0m'

# The required version of Python.
REQ_PYTHO_VERSION = {"major": 3, "minor": 8}


####################################################################################

def check_python_version():
    print("Checking the version of Python", end='')
    version_str = version.split(' ')[0]
    major, minor, patch = list(map(int, version_str.split('.')))
    if (major < REQ_PYTHO_VERSION["major"]) or ((major == REQ_PYTHO_VERSION["major"]) and
                                                (minor < REQ_PYTHO_VERSION["minor"])):
        print(" [" + CRED + "Fail" + CEND + "]")
        print("The used version of Python ({0}) is incompatible with the minimal required version {1}.{2}.x) "
              "for Embedded Proto".format(version_str, REQ_PYTHO_VERSION["major"], REQ_PYTHO_VERSION["minor"]),
              file=stderr)
        exit(1)

    print(" [" + CGREEN + "Success" + CEND + "]")


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

    print("Checking your Protoc version", end='')
    
    try:
        output = subprocess.run(["protoc", "--version"], check=False, capture_output=True)
    except OSError:
        print(" [" + CRED + "Fail" + CEND + "]")
        print("Unable to find protoc in your path.")
        print("Stopping the setup.")
        exit(0)

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

            print(" [" + CYELLOW + "Warning" + CEND + "]")
            text += "\t3. Ignore the difference at your own risk!\n"
            print(text)
            while True:
                user_input = input("Ignore the difference [Y/n]: ")
                if ('Y' == user_input) or ('y' == user_input):
                    # Continue the setup
                    print("Ignoring the difference.")
                    break
                elif ('N' == user_input) or ('n' == user_input):
                    # Stop the setup.
                    print("Stopping the setup.")
                    exit(0)
        else:
            # We can only stop if we have a version prior to v21.0.
            print(" [" + CRED + "Fail" + CEND + "]")
            print(text)
            print("Stopping the setup.")
            exit(0)

    else:
        print(" [" + CGREEN + "Success" + CEND + "]")


####################################################################################

def display_feedback():
    text = "\n"
    text += "================================================================================\n"
    text += "|                                                                              |\n"
    text += "| Three simple things you can do to help improve Embedded Proto:               |\n"
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

def run(arguments):
    # Execute the setup process for Embedded Proto.

    try:

        # ---------------------------------------
        check_python_version()

        # ---------------------------------------
        check_protoc_version()

        # ---------------------------------------
        print("Creating a virtual environment for Embedded Proto.", end='')
        venv.create("venv", with_pip=True)
        print(" [" + CGREEN + "Success" + CEND + "]")

        # ---------------------------------------
        print("Installing requirement Python packages in the virtual environment.", end='')
        on_windows = "Windows" == platform.system()
        command = []
        if on_windows:
            command.append("./venv/Scripts/pip")
        else:
            command.append("./venv/bin/pip")
        command.extend(["install", "-r", "requirements.txt"])
        result = subprocess.run(command, check=False, capture_output=True)
        if result.returncode:
            print(" [" + CRED + "Fail" + CEND + "]")
            print(result.stderr.decode("utf-8"), end='', file=stderr)
            exit(1)
        else:
            print(" [" + CGREEN + "Success" + CEND + "]")

        # ---------------------------------------
        print("Build the protobuf extension file used to include Embedded Proto custom options.", end='')
        command = ["protoc", "-I", "generator", "--python_out=generator", "embedded_proto_options.proto"]
        if arguments.include is not None:
            command.extend(["-I", str(arguments.include)])
        result = subprocess.run(command, check=False, capture_output=True)
        if result.returncode:
            print(" [" + CRED + "Fail" + CEND + "]")
            print(result.stderr.decode("utf-8"), end='', file=stderr)
            print("Waring: Unable to generate the options file. This might be solved by providing the --include option."
                  " See --help for more info.", file=stderr)
            exit(1)
        else:
            print(" [" + CGREEN + "Success" + CEND + "]")

    except Exception as e:
        print(" [" + CRED + "Fail" + CEND + "]")
        print("Error: " + str(e), file=stderr)
        exit(1)


####################################################################################

class ReadableDir(argparse.Action):
    # This class is used to check if the --include path provided as a parameter is a valid directory.

    def __call__(self, parser_obj, namespace, values, option_string=None):
        prospective_dir = values
        if not os.path.isdir(prospective_dir):
            raise argparse.ArgumentTypeError("readable_dir: \"{0}\" is not a valid path".format(prospective_dir))
        if os.access(prospective_dir, os.R_OK):
            setattr(namespace, self.dest, prospective_dir)
        else:
            raise argparse.ArgumentTypeError("readable_dir: \"{0}\" is not a readable dir".format(prospective_dir))


####################################################################################

def add_parser_arguments(parser_obj):
    # This function is used to add parameters required by the Embedded Proto script. Setup scripts used in examples 
    # now can extend it with their own parameters.
    parser_obj.add_argument('-I', '--include', action=ReadableDir,
                            help="Provide the protoc include folder. Required when you installed protoc in a non "
                                 "standard folder, for example: \"~/protobuf/protoc-21.5/include\".")


####################################################################################

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="This script will setup the environment to generate Embedded Proto "
                                                 "code in your project.")
    add_parser_arguments(parser)
    display_feedback()
    args = parser.parse_args()

    run(args)
    
    # ---------------------------------------
    print("Setup completed with success!")
