add_library(MatrixOSAPPInterface
    Application.h
)

target_include_directories(MatrixOSAPPInterface PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(MatrixOSAPPInterface PUBLIC
    MatrixOSInterface
)

# Global list to store registered applications
set_property(GLOBAL PROPERTY REGISTERED_APPLICATIONS "")

# Macro for applications to register their classes
macro(RegisterApplicationClass CLASS_NAME)
    # Get the current directory name as the app folder
    get_filename_component(APP_FOLDER ${CMAKE_CURRENT_SOURCE_DIR} NAME)

    # Determine if this is a system app from the current system flag
    if(DEFINED IS_SYSTEM_APP AND IS_SYSTEM_APP)
        set(SYSTEM_FLAG "true")
    else()
        set(SYSTEM_FLAG "false")
    endif()

    # Store the registration: FOLDER:CLASS:IS_SYSTEM
    set_property(GLOBAL APPEND PROPERTY REGISTERED_APPLICATIONS "${APP_FOLDER}:${CLASS_NAME}:${SYSTEM_FLAG}")

    # message(STATUS "Registered application class: ${CLASS_NAME} (${APP_FOLDER}, system=${SYSTEM_FLAG})")
endmacro()

# Add boot animation framework
add_subdirectory(BootAnimation)

# Read application list from device family path
set(APPLICATION_LIST_FILE "${FAMILY_PATH}/ApplicationList.txt")

# Function to clone git repository for applications
function(clone_git_application GIT_URL GIT_REF APP_NAME)
    set(GIT_APPS_DIR "${FAMILY_PATH}/Applications")
    set(APP_DIR "${GIT_APPS_DIR}/${APP_NAME}")

    # Create git apps directory if it doesn't exist
    if(NOT EXISTS ${GIT_APPS_DIR})
        file(MAKE_DIRECTORY ${GIT_APPS_DIR})
    endif()

    # Check if app already exists and is up to date
    set(NEEDS_CLONE TRUE)
    if(EXISTS ${APP_DIR})
        # Check if it's a git repo and has the correct remote
        execute_process(
            COMMAND git -C ${APP_DIR} remote get-url origin
            OUTPUT_VARIABLE CURRENT_REMOTE
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(CURRENT_REMOTE STREQUAL GIT_URL)
            # Check if we're on the correct ref locally
            execute_process(
                COMMAND git -C ${APP_DIR} rev-parse HEAD
                OUTPUT_VARIABLE CURRENT_COMMIT
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
            execute_process(
                COMMAND git -C ${APP_DIR} rev-parse ${GIT_REF}
                OUTPUT_VARIABLE TARGET_COMMIT
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
            if(CURRENT_COMMIT STREQUAL TARGET_COMMIT)
                set(NEEDS_CLONE FALSE)
            endif()
        endif()
    endif()

    if(NEEDS_CLONE)
        message(STATUS "Cloning git application: ${APP_NAME} from ${GIT_URL}@${GIT_REF}")

        # Remove existing directory if it exists
        if(EXISTS ${APP_DIR})
            file(REMOVE_RECURSE ${APP_DIR})
        endif()

        # Clone the repository
        execute_process(
            COMMAND git clone ${GIT_URL} ${APP_DIR}
            RESULT_VARIABLE CLONE_RESULT
            ERROR_QUIET
        )

        if(NOT CLONE_RESULT EQUAL 0)
            message(WARNING "Failed to clone ${GIT_URL}")
            return()
        endif()

        # Checkout the specific branch/commit
        execute_process(
            COMMAND git -C ${APP_DIR} checkout ${GIT_REF}
            RESULT_VARIABLE CHECKOUT_RESULT
            ERROR_QUIET
        )

        if(NOT CHECKOUT_RESULT EQUAL 0)
            message(WARNING "Failed to checkout ${GIT_REF} in ${APP_NAME}")
            return()
        endif()

        message(STATUS "Successfully cloned ${APP_NAME}")
    else()
        message(STATUS "Git application ${APP_NAME} is up to date")
    endif()

    # Return the app directory path
    set(GIT_APP_DIR ${APP_DIR} PARENT_SCOPE)
endfunction()

# Function to add enabled application subdirectories
function(add_enabled_applications CONFIG_FILE)
    if(NOT EXISTS ${CONFIG_FILE})
        message(WARNING "Application list file not found at ${CONFIG_FILE}")
        return()
    endif()

    file(STRINGS ${CONFIG_FILE} CONFIG_LINES)
    set(ENABLED_APP_LIBS "" PARENT_SCOPE)

    foreach(LINE ${CONFIG_LINES})
        # Skip comments and empty lines
        string(STRIP "${LINE}" LINE_TRIMMED)
        if(LINE_TRIMMED MATCHES "^#" OR LINE_TRIMMED STREQUAL "")
            continue()
        endif()

        # Check for [System] prefix
        set(IS_SYSTEM_APP FALSE)
        if(LINE_TRIMMED MATCHES "^\\[System\\]")
            set(IS_SYSTEM_APP TRUE)
            # Remove [System] prefix
            string(REGEX REPLACE "^\\[System\\]" "" LINE_TRIMMED "${LINE_TRIMMED}")
        endif()

        # APP_PATH is just the folder name now
        set(APP_PATH "${LINE_TRIMMED}")

        # Check if this is a git URL
        set(IS_GIT_APP FALSE)
        if(APP_PATH MATCHES "^git:")
            set(IS_GIT_APP TRUE)
            # Parse git:URL@ref format
            string(REGEX REPLACE "^git:" "" GIT_SPEC "${APP_PATH}")
            string(FIND "${GIT_SPEC}" "@" AT_POS)
            if(AT_POS EQUAL -1)
                message(WARNING "Invalid git URL format: ${APP_PATH}. Expected git:URL@branch/hash")
                continue()
            endif()
            string(SUBSTRING "${GIT_SPEC}" 0 ${AT_POS} GIT_URL)
            math(EXPR REF_START "${AT_POS} + 1")
            string(SUBSTRING "${GIT_SPEC}" ${REF_START} -1 GIT_REF)

            # Extract app name from URL (last part of path without .git)
            string(REGEX REPLACE ".*/([^/]+)(\\.git)?$" "\\1" APP_NAME "${GIT_URL}")
            string(REGEX REPLACE "\\.git$" "" APP_NAME "${APP_NAME}")

            # Clone/update the git repository
            clone_git_application("${GIT_URL}" "${GIT_REF}" "${APP_NAME}")

            # Set paths to the cloned repository
            set(APP_DIR "${GIT_APP_DIR}")
            file(RELATIVE_PATH DISPLAY_PATH ${CMAKE_SOURCE_DIR} ${APP_DIR})
        endif()

        # Resolve paths and create display path for non-git apps
        if(NOT IS_GIT_APP)
            if(APP_PATH MATCHES "\\{FAMILY_PATH\\}")
                string(REPLACE "{FAMILY_PATH}" "${FAMILY_PATH}" APP_DIR "${APP_PATH}")
                file(RELATIVE_PATH DISPLAY_PATH ${CMAKE_SOURCE_DIR} ${APP_DIR})
            else()
                set(APP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${APP_PATH}")
                set(DISPLAY_PATH "Applications/${APP_PATH}")
            endif()
        endif()
        set(APP_CMAKE "${APP_DIR}/CMakeLists.txt")
        
        # Display loading message
        string(ASCII 27 Esc)
        get_filename_component(APP_NAME ${APP_PATH} NAME)
        if(IS_SYSTEM_APP)
            # Purple color (95m) for system apps
            message(STATUS "${Esc}[95mLoaded application with system level privilege: '${APP_NAME} at ${DISPLAY_PATH}'${Esc}[0m")
        else()
            # Yellow color (93m) for regular apps
            message(STATUS "${Esc}[93mLoaded application: '${APP_NAME} at ${DISPLAY_PATH}'${Esc}[0m")
        endif()

        if(EXISTS ${APP_DIR} AND EXISTS ${APP_CMAKE})
            # Set the system flag for the subdirectory to use
            set(IS_SYSTEM_APP ${IS_SYSTEM_APP})

            # Add the subdirectory for this application
            if(IS_GIT_APP)
                # For git applications, provide binary directory
                string(REPLACE "/" "_" BINARY_DIR_NAME "git_${APP_NAME}")
                string(REPLACE ":" "_" BINARY_DIR_NAME "${BINARY_DIR_NAME}")
                add_subdirectory(${APP_DIR} ${CMAKE_BINARY_DIR}/external_apps/${BINARY_DIR_NAME})
            elseif(APP_PATH MATCHES "\\{FAMILY_PATH\\}")
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
            
        else()
            message(WARNING "Application '${APP_PATH}' directory or CMakeLists.txt not found")
        endif()
    endforeach()

    set(ENABLED_APP_LIBS ${ENABLED_APP_LIBS} PARENT_SCOPE)
endfunction()

# Add enabled application subdirectories
add_enabled_applications(${APPLICATION_LIST_FILE})

# Generate Applications.h header file from registered applications
function(generate_applications_header OUTPUT_FILE)
    set(HEADER_CONTENT "// THIS FILE IS GENERATED BY CMAKE FROM RegisterApplicationClass() CALLS\n\n")
    string(APPEND HEADER_CONTENT "#pragma once\n\n")
    string(APPEND HEADER_CONTENT "#include \"Application.h\"\n\n")

    # Get all registered applications
    get_property(REGISTERED_APPS GLOBAL PROPERTY REGISTERED_APPLICATIONS)

    if(REGISTERED_APPS)
        foreach(APP_ENTRY ${REGISTERED_APPS})
            # Parse FOLDER:CLASS:IS_SYSTEM format
            string(REPLACE ":" ";" ENTRY_PARTS ${APP_ENTRY})
            list(GET ENTRY_PARTS 0 APP_FOLDER)
            list(GET ENTRY_PARTS 1 CLASS_NAME)
            list(GET ENTRY_PARTS 2 IS_SYSTEM)

            # Generate include and registration
            string(APPEND HEADER_CONTENT "#include \"${APP_FOLDER}/${CLASS_NAME}.h\"\n")
            string(APPEND HEADER_CONTENT "REGISTER_APPLICATION(${CLASS_NAME}, ${IS_SYSTEM})\n\n")
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
    ${FAMILY_PATH}/Applications
)

# Link all enabled application libraries
if(ENABLED_APP_LIBS)
    target_link_libraries(MatrixOSApps INTERFACE ${ENABLED_APP_LIBS})
endif()
