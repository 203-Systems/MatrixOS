add_library(MatrixOSAPPInterface 
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
        message(WARNING "Application list file not found at ${CONFIG_FILE}")
        return()
    endif()
    
    file(STRINGS ${CONFIG_FILE} CONFIG_LINES)
    set(ENABLED_APPS "" PARENT_SCOPE)
    set(ENABLED_APP_LIBS "" PARENT_SCOPE)
    
    foreach(LINE ${CONFIG_LINES})
        # Skip comments and empty lines
        string(STRIP "${LINE}" LINE_TRIMMED)
        if(LINE_TRIMMED MATCHES "^#" OR LINE_TRIMMED STREQUAL "")
            continue()
        endif()
        
        # Parse path and header name
        if(LINE_TRIMMED MATCHES ":")
            string(REPLACE ":" ";" LINE_PARTS ${LINE_TRIMMED})
            list(GET LINE_PARTS 0 APP_PATH)
            list(GET LINE_PARTS 1 HEADER_NAME)
        else()
            set(APP_PATH "${LINE_TRIMMED}")
            set(HEADER_NAME "")
        endif()
        
        # Resolve paths and create display path
        if(APP_PATH MATCHES "\\{FAMILY_PATH\\}")
            string(REPLACE "{FAMILY_PATH}" "${FAMILY_PATH}" APP_DIR "${APP_PATH}")
            file(RELATIVE_PATH DISPLAY_PATH ${CMAKE_SOURCE_DIR} ${APP_DIR})
        else()
            set(APP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${APP_PATH}")
            set(DISPLAY_PATH "Applications/${APP_PATH}")
        endif()
        set(APP_CMAKE "${APP_DIR}/CMakeLists.txt")
        
        # Display loading message
        string(ASCII 27 Esc)
        if(HEADER_NAME)
            set(APP_NAME "${HEADER_NAME}")
        else()
            get_filename_component(APP_NAME ${APP_PATH} NAME)
        endif()
        message(STATUS "${Esc}[93mLoaded application: '${APP_NAME} at ${DISPLAY_PATH}'${Esc}[0m")

        if(EXISTS ${APP_DIR} AND EXISTS ${APP_CMAKE})
            # Add the subdirectory for this application
            if(APP_PATH MATCHES "\\{FAMILY_PATH\\}")
                # For family path substitutions, provide binary directory
                string(REPLACE "/" "_" BINARY_DIR_NAME "${APP_PATH}")
                string(REPLACE "{FAMILY_PATH}" "FAMILY" BINARY_DIR_NAME "${BINARY_DIR_NAME}")
                string(REPLACE ":" "_" BINARY_DIR_NAME "${BINARY_DIR_NAME}")
                add_subdirectory(${APP_DIR} ${CMAKE_BINARY_DIR}/external_apps/${BINARY_DIR_NAME})
            else()
                # Regular relative path
                add_subdirectory(${APP_PATH})
            endif()
            
            # Extract library name from CMakeLists.txt
            file(READ ${APP_CMAKE} CMAKE_CONTENT)
            if(CMAKE_CONTENT MATCHES "add_library\\(([A-Za-z0-9_]+)")
                set(LIB_NAME "${CMAKE_MATCH_1}")
                list(APPEND ENABLED_APP_LIBS ${LIB_NAME})
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
            
            # Handle {FAMILY_PATH} substitution in include path
            if(APP_PATH MATCHES "\\{FAMILY_PATH\\}")
                # For family path, make it relative from applications directory to the family directory
                file(RELATIVE_PATH REL_FAMILY_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${FAMILY_PATH})
                string(REPLACE "{FAMILY_PATH}" "${REL_FAMILY_PATH}" APP_INCLUDE_PATH "${APP_PATH}")
            else()
                # Regular relative path
                set(APP_INCLUDE_PATH "${APP_PATH}")
            endif()
            
            string(APPEND HEADER_CONTENT "#include \"${APP_INCLUDE_PATH}/${HEADER_NAME}.h\"\n")
            string(APPEND HEADER_CONTENT "REGISTER_APPLICATION(${HEADER_NAME})\n\n")
        endforeach()
        string(APPEND HEADER_CONTENT "\n")
    endif()
    
    # Write the header file
    file(WRITE ${OUTPUT_FILE} ${HEADER_CONTENT})
endfunction()

# Generate the Applications.h file in build directory (only if ApplicationList.txt is newer)
set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated/applications")
file(MAKE_DIRECTORY ${GENERATED_DIR})
set(APPLICATIONS_HEADER_FILE "${GENERATED_DIR}/Applications.h")

# Check if Applications.h needs to be regenerated
set(NEED_REGENERATE FALSE)
if(NOT EXISTS ${APPLICATIONS_HEADER_FILE})
    set(NEED_REGENERATE TRUE)
    message(STATUS "Applications.h does not exist, generating...")
else()
    if(${APPLICATION_LIST_FILE} IS_NEWER_THAN ${APPLICATIONS_HEADER_FILE})
        set(NEED_REGENERATE TRUE)
        message(STATUS "ApplicationList.txt modified. Updating Applications.h...")
    else()
        message(STATUS "Applications.h is up to date, skipping generation")
    endif()
endif()

if(NEED_REGENERATE)
    generate_applications_header(${APPLICATIONS_HEADER_FILE})
endif()

add_library(MatrixOSApps INTERFACE)
target_include_directories(MatrixOSApps INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GENERATED_DIR}
)

# Link all enabled application libraries
if(ENABLED_APP_LIBS)
    target_link_libraries(MatrixOSApps INTERFACE ${ENABLED_APP_LIBS})
endif()
