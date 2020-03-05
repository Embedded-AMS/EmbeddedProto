@echo off

:: This file is used to invoke protoc-gen-eams.py as a plugin to protoc on 
:: Windows. The reason this has to be used is that protoc expects a binary or
:: terminal script as plugin. Directly calling python scripts is not supported.

venv\Scripts\python generator\protoc-gen-eams.py --protoc-plugin