if(WITH_XC_SSHAGENT)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

    set(sshagent_SOURCES
        AgentSettingsPage.cpp
        AgentSettingsWidget.cpp
        ASN1Key.cpp
        BinaryStream.cpp
        KeeAgentSettings.cpp
        OpenSSHKey.cpp
        OpenSSHKeyGen.cpp
        OpenSSHKeyGenDialog.cpp
        SSHAgent.cpp
    )

    add_library(sshagent STATIC ${sshagent_SOURCES})
    target_link_libraries(sshagent Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)
endif()
