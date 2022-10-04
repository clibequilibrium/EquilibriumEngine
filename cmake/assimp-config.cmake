set(ASSIMP_INCLUDE_DIRS "${PROJECT_WORKING_DIRECTORY}/3rdparty/assimp/include/")
set(ASSIMP_LIBRARIES
    "${PROJECT_WORKING_DIRECTORY}/3rdparty/assimp/lib/assimp.lib;${PROJECT_WORKING_DIRECTORY}/3rdparty/assimp/lib/zlibstatic.lib"
)

string(STRIP "${ASSIMP_LIBRARIES}" ASSIMP_LIBRARIES)
