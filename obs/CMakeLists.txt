find_package(wxWidgets COMPONENTS core base REQUIRED)

include("${wxWidgets_USE_FILE}")

function(wx_required_version)
	include(CheckCXXSourceCompiles)
	set(WX_CHECK_DEFINITIONS "")
	foreach(wxw_def ${wxWidgets_DEFINITIONS})
		set(WX_CHECK_DEFINITIONS "${WX_CHECK_DEFINITIONS} -D${wxw_def}")
	endforeach()
	set(CMAKE_REQUIRED_DEFINITIONS ${WX_CHECK_DEFINITIONS})
	set(CMAKE_REQUIRED_INCLUDES ${wxWidgets_INCLUDE_DIRS})
	check_cxx_source_compiles("
		#include <wx/sysopt.h>
		#if (wxMINOR_VERSION < 9 && wxMAJOR_VERSION <= 2) || wxMAJOR_VERSION < 3
		#error
		#endif
		int main() {}"
		WX_REQUIRED_VERSION)

	if(NOT WX_REQUIRED_VERSION)
		message(SEND_ERROR "wxWidgets version 2.9 or later is required")
	endif()
endfunction()
wx_required_version()


link_libraries(${wxWidgets_LIBRARIES})

add_executable(obs
	window-obs-basic.cpp
	window-subclass.cpp
	obs.cpp
	OBSWindows.cpp)

if(APPLE)
	set_target_properties(obs PROPERTIES
		MACOSX_BUNDLE ${BUILD_APP_BUNDLE})
endif()
