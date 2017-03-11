# RPD Design
Shengjie Lin's graduation project at the Department of Electronic Engineering, Tsinghua University.

## Introduction
Given both the base image and the descriptive Ontology file, the program will generate the RPD design layout.

## Installation
To compile the code, you will need:
- [Visual Studio 2015](https://www.visualstudio.com/)
- [Qt 5.x for Windows](https://www.qt.io/)
- [Qt VS Tools](http://doc.qt.io/qtvstools/index.html)
- [OpenCV 3.x](http://opencv.org/)
- [JDK 8](http://www.oracle.com/technetwork/java/javase/downloads/index.html)
- [Apache Jena](https://jena.apache.org/)

Set new environment variables:
- OPENCV_DIR="C:\Program Files\opencv\build\x64\vc14" or alike
- JDK_DIR="C:\Program Files\Java\jdk1.8.0_121" or alike

Add to PATH:
- "%OPENCV_DIR%\bin"
- "C:\Program Files\Java\jre1.8.0_121\bin\server" or alike

In "%ROOT%\RpdDesign\GlobalVariables.h", modify accordingly the following line:

<code>const string jenaLibPath = "C:/Program Files/apache-jena-3.2.0/lib/";</code>

## Run
After successful compilation, the executalbe will be stored as "%ROOT%\x64\Debug\RpdDesign.exe". Before execution, add to PATH:
- "C:\Qt\Qt5.8.0\5.8\msvc2015_64\bin" or alike
