@rem Customize to specify the location of your provider DLL or EXE

@set DLLFileMain=Debug\MultiProviderRes.dll
@set DLLFileAlternate=Release\MultiProviderRes.dll
@set ManifestFileMain=etwprovider.man
@set ManifestFileAlternate=etwprovider.man





@echo This will register the custom ETW providers.

@set DLLFile=%DLLFileMain%
@if exist %DLLFile% goto tier0OK
@set DLLFile=%DLLFileAlternate%
@if not exist %DLLFile% goto NoTier0
@:tier0OK

@set ManifestFile=%ManifestFileMain%
@if exist %ManifestFile% goto manifestOK
@set ManifestFile=%ManifestFileAlternate%
@if not exist %ManifestFile% goto NoManifest
@:manifestOK

xcopy /y %DLLFile% %windir%
wevtutil um %ManifestFile%
@if %errorlevel% == 5 goto NotElevated
wevtutil im %ManifestFile%
@exit /b

@:NotElevated
@echo ETW providers must be registered from an elevated (administrator) command
@echo prompt. Try again from an elevated prompt.
@exit /b

@:NoTier0
@echo Can't find %DLLFileMain% or %DLLFileAlternate%
@exit /b

@:NoManifest
@echo Can't find %ManifestFileMain% or %ManifestFileAlternate%
@exit /b
