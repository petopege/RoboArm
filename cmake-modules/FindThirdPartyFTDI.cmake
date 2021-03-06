# ---------------------------------
# Finds FTDI API
# Adds library to target
# Adds include path
# ---------------------------------
IF(WIN32)
	FIND_PATH(PATH_FTDI ftd2xx.h PATHS 
		${OV_CUSTOM_DEPENDENCIES_PATH}/ftdi/include
		${OV_CUSTOM_DEPENDENCIES_PATH}/ftdi/lib)
	IF(PATH_FTDI)
		MESSAGE(STATUS "  Found FTDI API...")
		INCLUDE_DIRECTORIES(${PATH_FTDI})
		FIND_LIBRARY(LIB_FTDI ftd2xx PATHS ${PATH_FTDI}/../lib)
		IF(LIB_FTDI)
			MESSAGE(STATUS "    [  OK  ] Third party lib ${LIB_FTDI}")
			TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIB_FTDI} )
		ELSE(LIB_FTDI)
			MESSAGE(STATUS "    [FAILED] lib ftd2xx")
		ENDIF(LIB_FTDI)

		ADD_DEFINITIONS(-DTARGET_HAS_ThirdPartyFTDI)
	ELSE(PATH_FTDI)
		MESSAGE(STATUS "  FAILED to find FTDI API")
	ENDIF(PATH_FTDI)
ENDIF(WIN32)
