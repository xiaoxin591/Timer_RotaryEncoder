@echo off
setlocal enabledelayedexpansion

:: ANSI яуи╚╢ЗбК
set "RED=[91m"
set "GREEN=[92m"
set "YELLOW=[93m"
set "BLUE=[94m"
set "RESET=[0m"

set foundPath=
set binPath=
set exeName=STM32_Programmer_CLI.exe

:: т╓╤╗рЕ╤Ю╦Жел╥Ш╨м©идэ╣дб╥╬╤
set drives=C D E F
set folderIDE=\ST\STM32CubeIDE*
set folderCLT=\ST\STM32CUbeCLT*
set folderPath=\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_*

:: ╠ИюЗкЫспел╥Ш╡╒╡Иур STM32_Programmer_CLI.exe
for %%d in (%drives%) do (
    for /d %%p in ("%%d:%folderCLT%") do (
        if exist "%%p\STM32CubeProgrammer\bin\%exeName%" (
            set foundPath=%%p\STM32CubeProgrammer\bin\%exeName%
            goto foundCLI
        )
    )
    for /d %%p in ("%%d:%folderIDE%") do (
        if exist "%%p%folderPath%" (
            for /d %%q in ("%%p%folderPath%") do (
                if exist "%%q\tools\bin\%exeName%" (
                    set foundPath=%%q\tools\bin\%exeName%
                    goto foundCLI
                )
            )
        )
    )
)

:: н╢ур╣╫ CLI ╧╓╬ъё╛©╙й╪╣щ╧ИкякВ
set searchFolderIDE=com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32*
set searchFolderCLT=STM32CubeProgrammer

echo %YELLOW%хМ╪Ч©идэн╢╟╡в╟тзд╛хон╩жцё╛©╙й╪╣щ╧И╡Иур%RESET%
echo уЩтз╢с D ел╡Иурё╛гКит╣хф╛©л......
:: D ел
:: ╣щ╧ИкякВ STM32CubeIDE ╧╓╬ъ
for /d /r D:\ %%i in (%searchFolderIDE%) do (
    if exist "%%i\tools\bin\%exeName%" (
        echo %GREEN%ур╣╫ CLT ╧╓╬ъё║бМио©╙й╪обть%RESET%
        set foundPath=%%i\tools\bin\%exeName%
        goto foundCLI
    )
)
:: ╣щ╧ИкякВ CLT ╧╓╬ъ
for /d /r D:\ %%i in (%searchFolderCLT%) do (
    if exist "%%i\bin\%exeName%" (
        echo %GREEN%ур╣╫ CLT ╧╓╬ъё║бМио©╙й╪обть%RESET%
        set foundPath=%%i\bin\%exeName%
        goto foundCLI
    )
)

echo тз D елн╢ур╣╫
echo.
echo уЩтз╢с C ел╡Иурё╛гКит╣хф╛©л......
:: D ел
:: ╣щ╧ИкякВ STM32CubeIDE ╧╓╬ъ
for /d /r C:\ %%i in (%searchFolderIDE%) do (
    if exist "%%i\tools\bin\%exeName%" (
        echo %GREEN%ур╣╫ CLT ╧╓╬ъё║бМио©╙й╪обть%RESET%
        set foundPath=%%i\tools\bin\%exeName%
        goto foundCLI
    )
)
:: ╣щ╧ИкякВ CLT ╧╓╬ъ
for /d /r C:\ %%i in (%searchFolderCLT%) do (
    if exist "%%i\bin\%exeName%" (
        echo %GREEN%ур╣╫ CLT ╧╓╬ъё║бМио©╙й╪обть%RESET%
        set foundPath=%%i\bin\%exeName%
        goto foundCLI
    )
)

:: хтх╩н╢ур╣╫ CLI ╧╓╬ъё╛╠╗╢М╡╒мкЁЖ
echo %RED%[ERROR]%RESET% STM32_Programmer_CLI.exe not found. Please install STM32CubeProgrammer.
echo %RED%[ERROR]%RESET% ╡ывВй╖╟эё║дЗ╣д╣Гдтн╢╟╡в╟ STM32CubeIDE хМ╪Чё╛╠ьпКох╟╡в╟╡е©иртобть║ё
echo %RED%[ERROR]%RESET% ╡ывВй╖╟эё║дЗ╣д╣Гдтн╢╟╡в╟ STM32CubeIDE хМ╪Чё╛╠ьпКох╟╡в╟╡е©иртобть║ё
echo %RED%[ERROR]%RESET% ╡ывВй╖╟эё║дЗ╣д╣Гдтн╢╟╡в╟ STM32CubeIDE хМ╪Чё╛╠ьпКох╟╡в╟╡е©иртобть║ё
timeout /t 29
exit /b 1

:foundCLI
echo %GREEN%Found STM32_Programmer_CLI:%RESET% %foundPath%

:: вт╤╞╡Иур bin нд╪Ч
if exist ".\build\Debug\*.bin" (
    for %%f in (.\build\Debug\*.bin) do (
        set binPath=%%~ff
        goto foundBin
    )
)

if exist ".\Debug\*.elf" (
    for %%f in (.\Debug\*.elf) do (
        set binPath=%%~ff
        goto foundBin
    )
)

:: ц╩ур╣╫ bin нд╪Чё╛╠╗╢МмкЁЖ
echo %RED%[ERROR]%RESET% No bin file found in expected locations!
echo.
echo %RED%[ERROR]%RESET% ╡ывВй╖╟эё║н╢ур╣╫©иобть╣днд╪Чё╛гК╪Л╡Иё╨
echo %RED%%RESET% 1║╒гК╡╩р╙тзя╧кУ╟ЭюО╢Р©╙ё╛╠ьпКох╫Бя╧кУтыткпп
echo %RED%%RESET% 2║╒╧╓Ёлнд╪Ч╡╩мЙуШё╛гКжьпбобть╠╬юЩЁл
echo %RED%%RESET% 3║╒╢к╫е╠╬пХр╙╥етз╧╓Ёл╦Ыд©б╪жпткпп
timeout /t 60
exit /b 1

:foundBin
echo %GREEN%Found BIN file:%RESET% %binPath%

:: обть bin нд╪Ч╣╫ Flash
echo %YELLOW%Flashing STM32 firmware...%RESET%
echo %YELLOW%©╙й╪обть...%RESET%
"%foundPath%" -c port=SWD -w "%binPath%" 0x08000000
if %errorlevel% neq 0 (
    echo %RED%[ERROR]%RESET% ST╧╓╬ъ╠╗╦Фобтьй╖╟э
    timeout /t 60
    exit /b 1
)

:: ╥╒км╦╢н╩цЭаН
echo %YELLOW%Flash programming successful. Resetting STM32...%RESET%
"%foundPath%" -c port=SWD -rst
if %errorlevel% neq 0 (
    echo %RED%[ERROR]%RESET% н╢дэ╦╢н╩╣╔ф╛╩Зё╛гКйж╤╞╟╢об╦╢н╩ё╛╩Ржьпбио╣Г╟Евс
    timeout /t 60
    exit /b 1
)

:: ╦ъааЁи╧╕пео╒
echo %GREEN%STM32 successfully programmed and reset! [0m
:: ╩╩пп
echo.
echo %GREEN%обтьЁи╧╕ё║ё║ [0m
timeout /t 15
exit


