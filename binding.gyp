{
	"targets": [
		{
			"target_name": "dplib",
			"include_dirs": [
				"<!(node -e \"require('nan')\")"
			],
			"conditions": [
				['OS=="win"',
					{
						"sources": [
							"src/win/dplib/dp.cc",
							"src/win/dplib/fp.cc",
							"src/win/shared/msg.cc"
						],
						"include_dirs": [
							"<!(echo %ProgramFiles%)/DigitalPersona/One Touch SDK/C-C++/Include",
							"src/win/shared"
						],
						"msvs_settings": {
							"VCCLCompilerTool": {
								"ExceptionHandling": "2"  # /EHsc
							}
						},
						"defines": [
							"UNICODE",
							"_UNICODE"
						]
					}
				],
				['OS=="win" and "<@(target_arch)"=="ia32"',
					{
						"libraries": [
							"<!(echo %ProgramFiles%)/DigitalPersona/One Touch SDK/C-C++/Lib/win32/DPFPApi.lib",
							"<!(echo %ProgramFiles%)/DigitalPersona/One Touch SDK/C-C++/Lib/win32/dpHFtrEx.lib",
							"<!(echo %ProgramFiles%)/DigitalPersona/One Touch SDK/C-C++/Lib/win32/dpHMatch.lib"
						]
					}
				],
				['OS=="win" and "<@(target_arch)"=="x64"',
					{
						"libraries": [
							"<!(echo %ProgramFiles%)/DigitalPersona/One Touch SDK/C-C++/Lib/x64/DPFPApi.lib",
							"<!(echo %ProgramFiles%)/DigitalPersona/One Touch SDK/C-C++/Lib/x64/dpHFtrEx.lib",
							"<!(echo %ProgramFiles%)/DigitalPersona/One Touch SDK/C-C++/Lib/x64/dpHMatch.lib"
						]
					}
				]
			]
		},
		{
			"target_name": "dpax",
			"include_dirs": [
				"<!(node -e \"require('nan')\")"
			],
			"conditions": [
				['OS=="win"',
					{
						"sources": [
							"src/win/dpax/dp.cc",
							"src/win/dpax/fp.cc",
							"src/win/shared/msg.cc"
						],
						"include_dirs": [
							"src/win/shared"
						],
						"msvs_settings": {
							"VCCLCompilerTool": {
								"ExceptionHandling": "2"  # /EHsc
							}
						},
						"defines": [
							"UNICODE",
							"_UNICODE"
						]
					}
				]
			]
		},
		{
			"target_name": "dpaxver",
			"conditions": [
				['OS=="win"',
					{
						"sources": [
							"src/win/dpax/verify.cc",
							"src/win/dpax/fp.cc"
						],
						"msvs_settings": {
							"VCCLCompilerTool": {
								"ExceptionHandling": "2"  # /EHsc
							}
						},
						"defines": [
							"UNICODE",
							"_UNICODE"
						]
					}
				]
			]
		}
	]
}
