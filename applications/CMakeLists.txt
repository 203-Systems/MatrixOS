add_library(MatrixOSAPPInterface 
    Application.cpp
    Application.h
)

target_include_directories(MatrixOSAPPInterface PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(MatrixOSAPPInterface PUBLIC
    MatrixOSInterface
)

# Add boot animation framework
add_subdirectory(BootAnimation)

# Read application list from device family path
set(APPLICATION_LIST_FILE "${FAMILY_PATH}/ApplicationList.txt")

# Function to add enabled application subdirectories
function(add_enabled_applications CONFIG_FILE)
    if(NOT EXISTS ${CONFIG_FILE})
        message(WARNING "Application list file not found: ${CONFIG_FILE}")
        return()
    endif()
    
    file(STRINGS ${CONFIG_FILE} CONFIG_LINES)
    set(ENABLED_APPS "" PARENT_SCOPE)
    set(ENABLED_APP_LIBS "" PARENT_SCOPE)
    
    foreach(LINE ${CONFIG_LINES})
        # Skip comments (lines starting with #) and empty lines
        string(STRIP "${LINE}" LINE_TRIMMED)
        if(NOT LINE_TRIMMED MATCHES "^#" AND NOT LINE_TRIMMED STREQUAL "")
            # Parse FolderName or FolderName:HeaderFileName format
            if(LINE_TRIMMED MATCHES ":")
                # Format: FolderName:HeaderFileName
                string(REPLACE ":" ";" LINE_PARTS ${LINE_TRIMMED})
                list(GET LINE_PARTS 0 APP_PATH)
                list(GET LINE_PARTS 1 HEADER_NAME)
                message(STATUS "Loaded application: '${HEADER_NAME}'")
            else()
                # Format: FolderName (subdirectory only, no header generation)
                set(APP_PATH "${LINE_TRIMMED}")
                set(HEADER_NAME "")
                # message(STATUS "Processing application: '${APP_PATH}' (subdirectory only)")
            endif()
            
            # Check if application directory and CMakeLists.txt exist
            set(APP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${APP_PATH}")
            set(APP_CMAKE "${APP_DIR}/CMakeLists.txt")

            if(EXISTS ${APP_DIR} AND EXISTS ${APP_CMAKE})
                # Add the subdirectory for this application
                add_subdirectory(${APP_PATH})
                # message(STATUS "Added application: ${APP_PATH}")
                
                # Extract library name from CMakeLists.txt
                file(READ ${APP_CMAKE} CMAKE_CONTENT)
                if(CMAKE_CONTENT MATCHES "add_library\\(([A-Za-z0-9_]+)")
                    set(LIB_NAME "${CMAKE_MATCH_1}")
                    list(APPEND ENABLED_APP_LIBS ${LIB_NAME})
                    # message(STATUS "Found library: ${LIB_NAME}")
                else()
                    message(WARNING "Could not find add_library() in ${APP_CMAKE}")
                endif()
                
                # Add to enabled apps list for header generation (only if header name is specified)
                if(NOT HEADER_NAME STREQUAL "")
                    list(APPEND ENABLED_APPS "${APP_PATH}:${HEADER_NAME}")
                endif()
            else()
                message(WARNING "Application '${APP_PATH}' directory or CMakeLists.txt not found")
            endif()
        endif()
    endforeach()
    
    set(ENABLED_APPS ${ENABLED_APPS} PARENT_SCOPE)
    set(ENABLED_APP_LIBS ${ENABLED_APP_LIBS} PARENT_SCOPE)
endfunction()

# Add enabled application subdirectories
add_enabled_applications(${APPLICATION_LIST_FILE})

# Generate Applications.h header file
function(generate_applications_header OUTPUT_FILE)
    set(HEADER_CONTENT "// THIS FILE IS GENERATED. PLEASE EDIT applicationList.txt INSTEAD\n\n")
    string(APPEND HEADER_CONTENT "#pragma once\n\n")
    string(APPEND HEADER_CONTENT "#include \"Application.h\"\n\n")
    
    # Add all enabled applications
    if(ENABLED_APPS)
        foreach(APP_ENTRY ${ENABLED_APPS})
            # Parse APP_PATH:HEADER_NAME format
            string(REPLACE ":" ";" ENTRY_PARTS ${APP_ENTRY})
            list(GET ENTRY_PARTS 0 APP_PATH)
            list(GET ENTRY_PARTS 1 HEADER_NAME)
            string(APPEND HEADER_CONTENT "#include \"${APP_PATH}/${HEADER_NAME}.h\"\n")
            string(APPEND HEADER_CONTENT "REGISTER_APPLICATION(${HEADER_NAME})\n\n")
        endforeach()
        string(APPEND HEADER_CONTENT "\n")
    endif()
    
    # Write the header file
    file(WRITE ${OUTPUT_FILE} ${HEADER_CONTENT})
    message(STATUS "Generated Applications.h at: ${OUTPUT_FILE}")
endfunction()

# Generate the Applications.h file in build directory
set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated/applications")
file(MAKE_DIRECTORY ${GENERATED_DIR})
set(APPLICATIONS_HEADER_FILE "${GENERATED_DIR}/Applications.h")
generate_applications_header(${APPLICATIONS_HEADER_FILE})

add_library(MatrixOSApps INTERFACE)
target_include_directories(MatrixOSApps INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GENERATED_DIR}
)

# Link all enabled application libraries
if(ENABLED_APP_LIBS)
    target_link_libraries(MatrixOSApps INTERFACE ${ENABLED_APP_LIBS})
endif()
