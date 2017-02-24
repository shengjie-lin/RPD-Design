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
- [Apache Jena 3.x](https://jena.apache.org/)

Add new environment variables:
- OPENCV_DIR="C:\Program Files\opencv\build\x64\vc14" or alike
- JDK_DIR="C:\Program Files\Java\jdk1.8.0_121" or alike

Add values to Path:
- "%OPENCV_DIR%\bin"
- "C:\Program Files\Java\jre1.8.0_121\bin\server" or alike

In \RpdDesign\Utilities.cpp, modify accordingly the line:

<code>const string jenaLibPath = "C:/Program Files/apache-jena-3.2.0/lib/";</code>
