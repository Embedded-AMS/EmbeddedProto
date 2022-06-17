::
:: Copyright (C) 2020-2022 Embedded AMS B.V. - All Rights Reserved
::
:: This file is part of Embedded Proto.
::
:: Embedded Proto is open source software: you can redistribute it and/or 
:: modify it under the terms of the GNU General Public License as published 
:: by the Free Software Foundation, version 3 of the license.
::
:: Embedded Proto  is distributed in the hope that it will be useful,
:: but WITHOUT ANY WARRANTY; without even the implied warranty of
:: MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
:: GNU General Public License for more details.
::
:: You should have received a copy of the GNU General Public License
:: along with Embedded Proto. If not, see <https://www.gnu.org/licenses/>.
::
:: For commercial and closed source application please visit:
:: <https://EmbeddedProto.com/license/>.
::
:: Embedded AMS B.V.
:: Info:
::   info at EmbeddedProto dot com
::
:: Postal address:
::   Johan Huizingalaan 763a
::   1066 VH, Amsterdam
::   the Netherlands
::
@echo off

:: This script will setup the environment to generate source code in your project.

:: Setup the virtual envirounment for Python packages
python -m venv venv
.\venv\Scripts\pip.exe install -r requirements.txt

:: Build the protobuf extension file used to include Embedded Proto custom options: 
:: embedded_proto_options_pb2.py. Read from the parameters the location of the 
:: google/protobuf/descriptor.proto file. This should be in your protoc-XX.Y-win64\include folder.
IF [%1] == [] ( 
  echo.
  echo Warning:
  echo   Unable to generate embedded_proto_options_pb2.py because the protoc folder was not 
  echo   provided as a parameter. Please refere to the README.md or the windows installation 
  echo   documentation.
  echo.
) ELSE (
  protoc -I%1 -I.\generator --python_out=.\generator .\generator\embedded_proto_options.proto 
)


