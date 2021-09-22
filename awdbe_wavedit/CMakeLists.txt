buildAndPackageLib("wavedit"
	COMPONENT "wave_edit"
	DESCRIPTION "Editor for embedded Wav"
	PRIVATE_INCLUDES ${Include_dir}
	#PUBLIC_LIBS "awdbedit"
	PUBLIC_LIBS "$<TARGET_FILE:awdbedit>"  # a workaround
	PRIVATE_LIBS comdlg32 gdi32 winmm
)

posix_and_wine_fixes(wavedit)
