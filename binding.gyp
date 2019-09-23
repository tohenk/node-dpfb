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
						"libraries": [
							"<!(echo %ProgramFiles%)/DigitalPersona/One Touch SDK/C-C++/Lib/<@(target_arch)/DPFPApi.lib",
							"<!(echo %ProgramFiles%)/DigitalPersona/One Touch SDK/C-C++/Lib/<@(target_arch)/dpHFtrEx.lib",
							"<!(echo %ProgramFiles%)/DigitalPersona/One Touch SDK/C-C++/Lib/<@(target_arch)/dpHMatch.lib"
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
			"include_dirs": [
				"<!@(node -p \"require('node-addon-api').include\")"
			],
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
