#pragma once

#if !defined(AWDBEDIT_EXPORT_API) || !defined(AWDBEDIT_IMPORT_API)
	#ifdef _MSC_VER
		#define AWDBEDIT_EXPORT_API __declspec(dllexport)
		#define AWDBEDIT_IMPORT_API __declspec(dllimport)
		#define AWDBEDIT_PACKED __declspec(packed)
	#else
		#if defined(__cplusplus) && __cplusplus >= 201703L && __has_cpp_attribute(gnu::dllexport) && __has_cpp_attribute(gnu::visibility)
			#define GNU_ATTR_INErtOLBrYhVMsJExbvSkBfuQvOKNS(attrBody) [[gnu::attrBody]]
		#else
			#if (defined(__STDC_VERSION__) && __STDC_VERSION__ > 201710L)
				#define GNU_ATTR_INErtOLBrYhVMsJExbvSkBfuQvOKNS(attrBody) [[gnu::attrBody]]
			#else
				#define GNU_ATTR_INErtOLBrYhVMsJExbvSkBfuQvOKNS(attrBody) __attribute__((attrBody))
			#endif
		#endif

		#define AWDBEDIT_PACKED GNU_ATTR_INErtOLBrYhVMsJExbvSkBfuQvOKNS(packed)
		#if defined(_WIN32) && !defined(__WINE__)
			#define AWDBEDIT_EXPORT_API GNU_ATTR_INErtOLBrYhVMsJExbvSkBfuQvOKNS(dllexport)
			#define AWDBEDIT_IMPORT_API GNU_ATTR_INErtOLBrYhVMsJExbvSkBfuQvOKNS(dllimport)
		#else
			#define AWDBEDIT_EXPORT_API GNU_ATTR_INErtOLBrYhVMsJExbvSkBfuQvOKNS(visibility("default"))
			#define AWDBEDIT_IMPORT_API
		#endif
		//#undef GNU_ATTR_INErtOLBrYhVMsJExbvSkBfuQvOKNS
	#endif

	#ifdef AWDBEDIT_EXPORTS
		#define AWDBEDIT_API AWDBEDIT_EXPORT_API
	#else
		#define AWDBEDIT_API AWDBEDIT_IMPORT_API
	#endif

	#ifdef AWDBEDIT_PLUGIN_EXPORTS
		#define AWDBEDIT_PLUGIN_API AWDBEDIT_EXPORT_API
	#else
		#define AWDBEDIT_PLUGIN_API AWDBEDIT_IMPORT_API
	#endif
#endif
