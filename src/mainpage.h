// Mainpage for doxygen

/** @mainpage package catalogAccess

 @author A. Sauvageon

 @section intro Introduction

 This package is to be used as a library for other GLAST packages.
 The user of U9, typically an application programmer, shall be able to select
 a catalog name from a predefined list.
 U9 shall provide the methods such that tools like the model definition tool U7
 or the source identification tool A2 can import a catalog into an internal
 representation with which they can work on their respective tasks.

 In order to ensure that the appropriate catalog version be available, the tool
 shall also provide methods to save and load a copy of the imported catalog
 in a FITS format corresponding to the internal catalog representation
 mentioned above.

 The tool shall offer methods for

   1. importing the selected part of the catalog in the internal representation

   2. accessing the data elements of the internal representation

   3. saving the internal representation into a FITS file

   4. loading the internal representation from a FITS file


*/
