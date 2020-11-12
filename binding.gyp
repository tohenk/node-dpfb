{
	"targets": [
		{
			"target_name": "dpfp",
			"sources": [
				"src/dpfp/fp.cc",
				"src/dpfp/worker.cc",
				"src/dpfp/acquire.cc",
				"src/dpfp/compare.cc",
				"src/dpfp/identify.cc",
				"src/dpfp/api.cc"
			],
			"conditions": [
				['OS=="win"',
					{
						"sources": [
							"src/shared/msg.cc",
						],
						"include_dirs": [
							"<!(echo %ProgramFiles%)/DigitalPersona/U.are.U SDK/Include",
							"src/shared"
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
							"<!(echo %ProgramFiles%)/DigitalPersona/U.are.U SDK/Windows/Lib/win32/dpfpdd.lib",
							"<!(echo %ProgramFiles%)/DigitalPersona/U.are.U SDK/Windows/Lib/win32/dpfj.lib"
						]
					}
				],
				['OS=="win" and "<@(target_arch)"=="x64"',
					{
						"libraries": [
							"<!(echo %ProgramFiles%)/DigitalPersona/U.are.U SDK/Windows/Lib/x64/dpfpdd.lib",
							"<!(echo %ProgramFiles%)/DigitalPersona/U.are.U SDK/Windows/Lib/x64/dpfj.lib"
						]
					}
				],
				['OS=="linux"',
					{
						"include_dirs": [
							"/opt/DigitalPersona/urusdk-linux/Include"
						],
						"libraries": [
							"/usr/lib/libdpfpdd.so",
							"/usr/lib/libdpfj.so"
						]
					}
				]
			]
		}
	]
}
