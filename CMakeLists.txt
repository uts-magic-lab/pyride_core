# CMake file for PyRideCore

cmake_minimum_required(VERSION 2.8)
project(PyRideCore)
find_package(qibuild)

include_directories( "." )
add_definitions(-DUSE_ENCRYPTION)

if(OLD_NAO)
add_definitions(-DOLD_NAO)
endif()

qi_create_lib( pyridecore
  SRC PyRideCommon.cpp
      jdatabufferdst.cpp
      DeviceController.cpp
      ServerDataProcessor.cpp
      RTPDataReceiver.cpp
      PyRideNetComm.cpp
      PyModuleStub.cpp
      AppConfigManager.cpp
      PythonServer.cpp
      VideoToWebBridge.cpp
  SHARED
)

qi_install_header( *.h )

qi_use_lib(pyridecore pthread dl ccrtp ccgnu2 jpeg python opus tinyxml asyncweb OPENSSL)
qi_stage_lib(pyridecore)

