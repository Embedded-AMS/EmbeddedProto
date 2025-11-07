function(generate_embedded_proto OUTPUT_VARIABLE)
    # Define the expected named parameters
    set(options)  # No options/flags
    # Single value arguments
    # OUTPUT_DIR: where to place generated files
    set(oneValueArgs OUTPUT_DIR)
    # Multi-value arguments: multiple include directories + list of proto files
    set(multiValueArgs INCLUDE_DIRS PROTO_FILES)

    # Parse the named parameters
    cmake_parse_arguments(PROTO "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required parameters
    if(NOT PROTO_INCLUDE_DIRS)
        message(FATAL_ERROR "INCLUDE_DIRS is required (use INCLUDE_DIRS <dir1> <dir2> ...)")
    endif()
    if(NOT PROTO_OUTPUT_DIR)
        message(FATAL_ERROR "OUTPUT_DIR is required")
    endif()
    if(NOT PROTO_PROTO_FILES)
        message(FATAL_ERROR "PROTO_FILES is required")
    endif()
    if(PROTO_PROTO_FILES STREQUAL "")
        message(WARNING "PROTO_FILES is empty")
    endif()
    # If caller did not provide an output variable name (first positional arg), use a default
    if(NOT OUTPUT_VARIABLE)
        set(OUTPUT_VARIABLE "GENERATED_PROTO_FILES")
    endif()

    file(MAKE_DIRECTORY ${PROTO_OUTPUT_DIR})
    get_filename_component(ABS_GENERATED_SRC_DIR ${PROTO_OUTPUT_DIR} ABSOLUTE)

    # Build include arguments (-I ...) from INCLUDE_DIRS
    set(INCLUDE_ARGS)
    foreach(INC_DIR ${PROTO_INCLUDE_DIRS})
        get_filename_component(ABS_INC_DIR ${INC_DIR} ABSOLUTE)
        list(APPEND INCLUDE_ARGS -I${ABS_INC_DIR})
    endforeach()

    set(GENERATED_FILES)
    # Process each protobuf file
    foreach(PROTO_FILE ${PROTO_PROTO_FILES})
        # Get absolute path
        get_filename_component(ABS_PROTO_FILE ${PROTO_FILE} ABSOLUTE)
        # Get the filename without the .proto extension
        get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)

        # Add the custom command to generate code for each proto file
        add_custom_command(
            OUTPUT "${ABS_GENERATED_SRC_DIR}/${PROTO_NAME}.h"
            WORKING_DIRECTORY ${EMBEDDED_PROTO_GEN_PATH}
            COMMAND protoc
            ARGS
                --plugin=protoc-gen-eams=protoc-gen-eams
                ${INCLUDE_ARGS}
                --eams_out=${ABS_GENERATED_SRC_DIR}
                ${ABS_PROTO_FILE}
            DEPENDS ${ABS_PROTO_FILE} ${EMBEDDED_PROTO_GEN_PATH}/protoc-gen-eams
            COMMENT "Generating EmbeddedProto code for ${PROTO_FILE}"
            VERBATIM
        )
        # Record the generated file path so the caller can consume it
        list(APPEND GENERATED_FILES "${ABS_GENERATED_SRC_DIR}/${PROTO_NAME}.h")
    endforeach()

    # Export the list of generated files to the caller (parent scope)
    set(${OUTPUT_VARIABLE} ${GENERATED_FILES} PARENT_SCOPE)
endfunction()
