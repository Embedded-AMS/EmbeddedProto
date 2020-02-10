:: Prebuild script to generate source code based on the proto files.
@echo off
if not exist "Core\Inc\generated" md Core\Inc\generated

cd ..\..
protoc --plugin=protoc-gen-eams=protoc-gen-eams.bat -I.\examples\proto --eams_out=.\examples\NUCLEO-F446RE\Core\Inc\generated .\examples\proto\request.proto
protoc --plugin=protoc-gen-eams=protoc-gen-eams.bat -I.\examples\proto --eams_out=.\examples\NUCLEO-F446RE\Core\Inc\generated .\examples\proto\reply.proto
