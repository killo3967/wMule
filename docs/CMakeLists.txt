# Documentation files
# These files are for reference only and not installed by default on Windows

if (WIN32)
    # On Windows, documentation is typically accessed via Help menu in the application
    # No automatic installation of documentation files
else()
    # Unix-like systems
    add_subdirectory (man)
    
    install (FILES
        ABOUT-NLS
        amulesig.txt
        Changelog
        EC_Protocol.txt
        TODO
        DESTINATION "${CMAKE_INSTALL_DOCDIR}"
    )
endif()
