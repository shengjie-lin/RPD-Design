# RPD Design
Part of Shengjie Lin's graduation project at the Department of Electronic Engineering, Tsinghua University.

## RpdDesign
Given both the base image and the descriptive Ontology file, the program will generate the RPD design layout.

### Build
To build the project, you will need:
* [Visual Studio 2015](https://www.visualstudio.com/)
* [Qt 5.x for Windows](https://www.qt.io/)
* [Qt VS Tools](http://doc.qt.io/qtvstools/index.html)
* [OpenCV 3.x](http://opencv.org/)
* [JDK 8](http://www.oracle.com/technetwork/java/javase/downloads/index.html)
* [Apache Jena](https://jena.apache.org/)

Set new environment variables:
* `OPENCV_DIR="C:\Program Files\opencv\build\"` or alike
* `JDK_DIR="C:\Program Files\Java\jdk1.8.0_131\"` or alike

Add to `PATH`:
* `%OPENCV_DIR%\x64\vc14\bin\`
* `C:\Program Files\Java\jre1.8.0_131\bin\server\` or alike

In `%ROOT%\RpdDesign\RpdDesign.cpp`, modify accordingly the following line:
> `string RpdDesign::jenaLibPath = "D:/Utilities/apache-jena-3.2.0/lib/";`

Use `lrelease` to generate Qt Linguist translation files from sources `%ROOT%\RpdDesign\rpddesign_[en,zh].ts`.

### Run & Test
After successful build, the executalbe will be stored as `%ROOT%\x64\[Debug,Release]\RpdDesign.exe`.

Before execution, add to `PATH`:
* `C:\Qt\Qt5.8.0\5.8\msvc2015_64\bin\` or alike

You may use sample resources in `%ROOT%\sample\` to produce the following design:
> <img src=sample\sample.png width=400>

## RpdDesignLib
Wraps the above-mentioned functionality into a library (DLL), to be called by Java users.

### Build
Same as above, except that Qt is not required, and that Jena is not explictly required (should be provided instead by the Java caller). In addition, by default this project will be built together with RpdDesign, as they share one Visual Studio solution.

_N.B. `com_shengjie_Main.h` is directly related to the Java caller, and may be auto-generated._

### Run & Test
After successful compilation, the DLL will be stored as `%ROOT%\x64\[Debug,Release]\RpdDesignLib.dll`. Please refer to `RpdDesignLibTest` below for actual test details.

## RpdDesignLibTest
Serves as a test program, which calls in Java `RpdDesignLib.dll` to generate Rpd designs provided the Ontology model and an optional base image.

### Build
To build the project, you will need:
* [IntelliJ IDEA](https://www.jetbrains.com/idea/)
* [OpenCV 3.x](http://opencv.org/)
* [JDK 8](http://www.oracle.com/technetwork/java/javase/downloads/index.html)
* [Apache Jena](https://jena.apache.org/)
* `RpdDesignLib.dll` as generated in RpdDesignLib

Open Project Structure (Ctrl+Alt+Shift+S) and specify accordingly items of Dependencies in Modules (or modify `%ROOT%\RpdDesignLibTest\RpdDesignLibTest.iml` directly).

_N.B. `generate_header.bat` contains the command that will call `javah` to generate the C++ header for `RpdDesignLib`. Modify the arguments to specify the valid classpath (-cp) and the desired output directory (-d). Before run, add `%JDK_DIR%\bin` to `PATH`._

### Run & Test
After successful build, running the program directly will produce `design_with_base.png` and `design.png` in `%ROOT%\RpdDesignLibTest`. They should both resemble `%ROOT%\sample\sample.png`.
