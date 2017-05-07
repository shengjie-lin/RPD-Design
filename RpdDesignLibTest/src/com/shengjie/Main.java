package com.shengjie;

import org.apache.jena.ontology.OntModel;
import org.apache.jena.ontology.OntModelSpec;
import org.apache.jena.rdf.model.ModelFactory;
import org.opencv.core.Mat;

import static org.opencv.imgcodecs.Imgcodecs.imread;
import static org.opencv.imgcodecs.Imgcodecs.imwrite;

public class Main {
    static {
        System.loadLibrary("RpdDesignLib");
        System.loadLibrary("opencv_java320");
    }

    public static native Mat getRpdDesign(OntModel ontModel, Mat mat);

    public static native Mat getRpdDesign(OntModel ontModel);

    public static void main(String[] args) {
        OntModel ontModel = ModelFactory.createOntologyModel(OntModelSpec.OWL_DL_MEM);
        ontModel.read("../sample/sample.owl");
        imwrite("design_with_base.png", getRpdDesign(ontModel, imread("../sample/base.png")));
        imwrite("design.png", getRpdDesign(ontModel));
    }
}
