# OBS CMake Windows CPack configuration module

include_guard(GLOBAL)

include(cpackconfig_common)

# Add GPLv2 license file to CPack
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/frontend/data/license/gplv2.txt")
set(CPACK_PACKAGE_VERSION "${OBS_VERSION_CANONICAL}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-windows-${CMAKE_VS_PLATFORM_NAME}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)

# Empire-OBS: ship both a portable ZIP and an NSIS .exe installer. NSIS ships on
# the windows-2022 CI runner; the build job keeps a rundir-ZIP fallback so a
# release never breaks even if the installer step fails.
set(CPACK_GENERATOR ZIP NSIS)
set(CPACK_THREADS 0)

# --- NSIS installer (Empire-OBS) ---
set(CPACK_NSIS_PACKAGE_NAME "Empire-OBS")
set(CPACK_NSIS_DISPLAY_NAME "Empire-OBS")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Empire-OBS")
set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\64bit\\obs64.exe")
set(CPACK_NSIS_MENU_LINKS "bin/64bit/obs64.exe" "Empire-OBS")
set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/Gh0s777tt/empire-OBS")
set(CPACK_NSIS_HELP_LINK "https://github.com/Gh0s777tt/empire-OBS/wiki")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

# Empire-OBS: bundle the MSVC runtime (vcruntime140 / msvcp140 / …) next to
# obs64.exe so a clean machine without Visual Studio or the VC++ Redistributable
# does NOT hit "failed to load module". UCRT is part of Windows 10+, so it is
# left to the OS. Makes both the ZIP and the installer self-contained.
set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "bin/64bit")
set(CMAKE_INSTALL_UCRT_LIBRARIES FALSE)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP FALSE)
include(InstallRequiredSystemLibraries)

include(CPack)
