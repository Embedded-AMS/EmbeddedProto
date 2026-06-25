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

import subprocess
import argparse
import platform
import os
import re
import sys
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
    shutil.rmtree("./EmbeddedProto.egg-info", ignore_errors=True)
    shutil.rmtree("./dist", ignore_errors=True) 
    shutil.rmtree("./generator", ignore_errors=True) # Remove remainders of version 3.
    try:
        os.remove("./EmbeddedProto/embedded_proto_options_pb2.py")
    except FileNotFoundError:
        # This exception we can safely ignore as it means the file was not there. In that case we do not have to remove
        # it.
        pass


####################################################################################

def read_required_version():
    with open("pyproject.toml", "r") as f:
        lines = f.readlines()
        required_re_compiled = re.compile(r"protobuf>=(?P<major>\d+)\.(?P<minor>\d+)\.(?P<patch>\d+)")
        for line in lines:
            match_req = required_re_compiled.search(line)
            if match_req:
                return match_req

        raise Exception("Unable to find protobuf version in pyproject.toml")


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
        command.extend(["install", "-e", "."])
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

        # ---------------------------------------
        # Generate the license config (and store/verify a token if one was given).
        configure_credentials(arguments)

    except Exception as e:
        print(" [" + CRED + "Fail" + CEND + "]")
        print("Error: " + str(e), file=stderr)
        exit(1)


####################################################################################

def configure_credentials(arguments):
    # Set up the license configuration. This always ensures a default config file
    # exists (so the user can find and edit it), and when a token or server URL was
    # passed it stores them and verifies the token against the server. The license
    # modules are pure standard library, so this runs without the freshly built venv.
    script_dir = os.path.dirname(os.path.abspath(__file__))
    if script_dir not in sys.path:
        sys.path.insert(0, script_dir)
    try:
        from EmbeddedProto import custom_header, config
    except Exception as e:
        print("Skipping license configuration (could not load module): " + str(e), file=stderr)
        return

    config.ensure_default()
    if arguments.token is not None or arguments.server_url is not None:
        # configure() prints its own colored result; a bad token is reported there
        # and does not fail the install (the environment is already set up).
        custom_header.configure(token=arguments.token, server_url=arguments.server_url)


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

    parser_obj.add_argument('--token', default=None,
                            help="Store this license build token in the EmbeddedProto user config "
                                 "(~/.config/embeddedproto/config.ini) and verify it against the server.")

    parser_obj.add_argument('--server-url', dest='server_url', default=None,
                            help="Store a custom license server URL (must be https://) in the user config. "
                                 "Use to point at a staging server.")


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
