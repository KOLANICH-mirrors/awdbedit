buildAndPackageLib("awdbe_builtins"
	COMPONENT "builtins"
	DESCRIPTION "Parsers for Award own stuff"
	PRIVATE_INCLUDES ${Include_dir}
	#PUBLIC_LIBS "awdbedit"
	PUBLIC_LIBS "$<TARGET_FILE:awdbedit>"  # a workaround
	PRIVATE_LIBS comdlg32 gdi32
)

posix_and_wine_fixes(awdbe_builtins)
