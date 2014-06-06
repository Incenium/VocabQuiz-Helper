-- A solution contains projects, and defines the available configurations
solution "VocabQuizHelper"
	configurations { "Debug", "Release" }
 
   -- A project defines one build target
	project "VocabQuizHelper"
		location "build"
		kind "ConsoleApp"
		language "C++"
		files { "src/**.cpp" }
		buildoptions {"-std=c++11"}
		
		-- Windows specific
		includedirs {"include/"}
		libdirs {"lib/"}

		links {"curl"}
 
		configuration "Debug"
			defines { "DEBUG" }
			flags { "Symbols" }
 
		configuration "Release"
			defines { "NDEBUG" }
			flags { "Optimize" }
