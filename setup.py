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

import subprocess
import argparse
import platform
import os
import re
from sys import stderr, stdout
import venv
import shutil

# Perform a system call to beable to display colors on windows
os.system("")

CGREEN = '\33[92m'
CRED = '\33[91m'
CYELLOW = '\33[93m'
CEND = '\33[0m'


###################################################################################

def clean_folder():
    # This function removes the folders and files created during setup and building the project. This way the folder is
    # brought back to its original state.

    print("Cleaning the folder.")

    shutil.rmtree("./venv", ignore_errors=True)
    shutil.rmtree("./build", ignore_errors=True)
    shutil.rmtree("./generator/EmbeddedProto.egg-info", ignore_errors=True)
    try:
        os.remove("./generator/EmbeddedProto/embedded_proto_options_pb2.py")
    except FileNotFoundError:
        # This exception we can safely ignore as it means the file was not there. In that case we do not have to remove
        # it.
        pass


####################################################################################

def read_required_version():
    with open("generator/pyproject.toml", "r") as f:
        lines = f.readlines()
        required_re_compiled = re.compile(r"protobuf>=(?P<major>\d+)\.(?P<minor>\d+)\.(?P<patch>\d+)")
        for line in lines:
            match_req = required_re_compiled.search(line)
            if match_req:
                return match_req

        raise Exception("Unable to find protobuf version in generator/pyproject.toml")


def check_protoc_version(arguments):
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

    version_re_compiled = re.compile(r".*\s(?P<major>\d+)\.(?P<minor>\d+)(?:\.(?P<patch>\d+))?")
    installed_version = version_re_compiled.search(output.stdout.decode("utf-8"))
    required_version = read_required_version()

    installed_version_major = installed_version.group('major')
    installed_version_minor = installed_version.group('minor')
    installed_version_patch = installed_version.group('patch')

    # If the installed protobuf version does not include a major number
    if installed_version_patch is None:
        installed_version_patch = installed_version_minor
        installed_version_minor = installed_version_major

    if installed_version_minor != required_version.group('minor'):
        text = "\n"
        text += "The version of Protoc (v{0}.{1})".format(installed_version_minor,
                                                          installed_version_patch)
        text += " you have installed is not the same as the version of\nthe protobuf python package " \
                "(v{0}.{1}).".format(required_version.group('minor'), required_version.group('patch'))
        text += "These are your options:\n" \
                "\t1. Install a matching version of Protoc.\n" \
                "\t2. Change the version of Embedded Proto.\n"

        # Check if all versions are above v21.0
        if ((21 <= int(installed_version_minor)) and (21 <= int(required_version.group('minor')))) or \
                ((21 > int(installed_version_minor)) and (21 > int(required_version.group('minor')))):

            print(" [" + CYELLOW + "Caution" + CEND + "]")
            text += ("\t3. Ignore the difference and try if it works for you (please let us know if it does not stating"
                     " the versions you used).\n")
            print(text)
            if arguments.ignore_version_diff:
                # Continue the setup
                print("Automatically ignoring the difference.")
            else:
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
        if arguments.clean:
            clean_folder()

        # ---------------------------------------
        check_protoc_version(arguments)

        # ---------------------------------------
        print("Creating a virtual environment for Embedded Proto.", end='')
        stdout.flush()
        venv.create("venv", with_pip=True)
        print(" [" + CGREEN + "Success" + CEND + "]")

        # Add extra include directories for protobuf build
        if arguments.include is not None:
            os.environ["EMBEDDEDPROTO_PROTOC_INCLUDE"] = str(arguments.include)

        # ---------------------------------------
        print("Installing EmbeddedProto in the virtual environment.", end='')
        stdout.flush()
        on_windows = "Windows" == platform.system()
        command = []
        if on_windows:
            command.append("./venv/Scripts/pip")
        else:
            command.append("./venv/bin/pip")
        command.extend(["install", "-e", "./generator"])
        result = subprocess.run(command, check=False, capture_output=True)
        if result.returncode:
            print(" [" + CRED + "Fail" + CEND + "]")
            print(result.stderr.decode("utf-8"), end='', file=stderr)
            print("If the error is related to protoc generating the options file it might be solved by providing"
                  " the --include option. See --help for more info.", end='', file=stderr)
            stdout.flush()
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

    parser_obj.add_argument('-c', '--clean', action='store_true',
                            help="Clean aka delete  the virtual environment and previous build results.")

    parser_obj.add_argument('--ignore_version_diff', action='store_true',
                            help="Ignore differences in the version of Protoc and that of the installed python package."
                                 " Try to run with the different version.")


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
