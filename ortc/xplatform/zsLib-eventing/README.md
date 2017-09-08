# zsLib-eventing

zsLib.Eventing.Tool.Compiler is a command line tool for generating eventing header files and manifest files to capture and render these events in an event viewer. Currently only Windows supports event recording and rendering, for example, the Windows Performance Recorder and Windows Performance Analyzer included on most systems.

````txt
-?
 -h
 -help    output this help text.

 -c       config_file_name          - input event provider json configuration file.
 -s       source_file_name_1 ... n  - input C/C++ source file.
 -o       output_name ... n         - output name.
 -author  "John Q Public"           - manifest author.
 ````

The important input files are as follows:  
`-c configpath/example.events.json`

This contains the eventing provider information. Typically one provider is used per library although it's possible for a library to contain more than one provider. A provider is a collection of events captured about a major subsystem. This configuration file contains the provider identifier information, source files to scan, tasks, operation codes, source typedefs, and aliases.

The C++ source files to scan for events are either contained in the configuration file or they are contained in the configuration file or optional included as command line arguments to the eventing compiler. All events and additional source code specific eventing configurations are all prefixed with the macro `ZS_EVENTING_...()` macros.

The generated output files are based on the following:  
`-o path/example.events`

Which will generate the following output files:
````txt
path/example.events.jman
path/example.events.h
path/example.events_win.h
path/example.events_win_etw.man
path/example.events_win_etw.wprp
````

Example of running the tool:  
`zsLib.Eventing.Tool.Compiler.exe -c configpath/example.events.json -o path/example.events"

The prefixes are as follows:
`.h` - A non-windows cross platform header file that maps the event macros to generate event on cross platform machines (except windows).  
`.jman` - A json based manifest file containing detailed information about the provider and all provider events which can be read by this eventing tool to capture, read, and re-emit events from a remote cross platform machine onto a windows machine to then capture using an event recorder (such as Windows Performance Recorder)  
`_win.h` - A windows specific header that maps event macros to the windows event generation macros (an eventing header generated from the `_win_etw.man` using the message compiler)  
`_win_etw.man` - A windows specific manifest file to generate a windows header file for capturing events on windows as well as an information DLL to display the events on windows in a human readable friendly manner (such as Windows Performance Analyzer).
`-win_etw.wprp` - Windows Performance Recorder Profile file which contains the provider information needed to capture the events using a windows event recorder (such as Windows Performance Recorder)  

The `_win_etw.man` file is needed to generate a `_win_etw.h` and  file using the windows message compiler.

The following windows batch script can be used to generate the `_win_etw.h` header file and `_win_etw.dll`:  
````bat
echo Create manifest header file...
mc.exe -um -r IntermediateOutputPath\ -h "path" "path\example.events_win_etw.man"
echo Create resource file...
rc.exe IntermediateOutputPath\example.events_win_etw.rc
echo Create manifest resource dll...
echo If compiling to managed code resource DLL use: csc.exe /out:IntermediateOutputPath\example.events_win_etw.dll /target:library /win32res:IntermediateOutputPath\example.events_win_etw.res
link -dll -noentry /MACHINE:x86 -out:IntermediateOutputPath\example.events_win_etw.dll IntermediateOutputPath\example.events_win_etw.res
echo Copy files to output directory...
copy IntermediateOutputPath\example.events_win_etw.dll "FinalOutputPath\"
copy "path\example.events_win_etw.man" "FinalOutputPath\"
copy "path\example.events_win_etw.wprp" "FinalOutputPath\"
icacls "FinalOutputPath\example.events_win_etw.dll" /grant Users:RX
````

This will generate the following output files:  
````
path/example.events_win_etw.h
FinalOutputPath/example.events_win_etw.dll
````

`_win_etw.h` - A windows specific header file for capturing events generated on a windows platform  
`_win_etw.dll` - A resource DLL containing all the information needed to display the captured events in a human readable format using an event viewer (such as the Windows Performance Analyzer)  


Once the `_win_etw.h` generation is complete, the source is ready to compile on Windows. On other non-windows platforms, this windows specific header is not used an thus generating this header is not needed.


The following windows batch script can be used to register the compiled message DLL for viewing events with a windows event viewer (such as the Windows Perforamce Analyzer):  
````bat
echo.
echo Registering manifest file and DLL for Windows Performance Recorder...
echo.
echo NOTE: Only run from command prompt as administrator
echo.

cd "FinalOutputPath\"
wevtutil.exe im example.events_win_etw.man /rf:"C:\FullPath\FinalOutputPath\example.events_win_etw.dll" /mf:"C:\FullPath\FinalOutputPath\example.events_win_etw.dll"
````

The following windows batch script can be used to unregister the compiled message DLL to clean up a previous registration:  
````bat
echo.
echo Unregistering manifest file and DLL for Windows Performance Recorder...
echo.
echo NOTE: Only run from command prompt as administrator
echo.
cd "FinalOutputPath\"
wevtutil.exe um example.events_win_etw.man
````

NOTE: To compile a project to not use the eventing header, define the following preprocesssor macro `ZSLIB_EVENTING_NOOP`. This will ensure all the eventing macros are compiled to dummy no-operational code.
