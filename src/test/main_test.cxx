/**
 * @file   main_test.cxx
 * @brief  Test program to exercise catalogAccess interface.
 * @author A. Sauvageon
 *
 * $Header $
 */

#include "st_facilities/Env.h"
#include "src/catalog.h"
#include <iomanip>

void help();
void show_STEP(const std::string text);
void show_quant(const catalogAccess::Quantity &);
void show_string(const std::string name, const std::string val);
void show_double(const std::string name, const double val);

static const std::_Ios_Fmtflags outDouble=std::ios::right|std::ios::scientific;

int main(int iargc, char * argv[]) {

// Show number of main argument
  std::cout << std::setw(70) << std::setfill('*') << "\n"
            << iargc-1 << " given argument(s)" << std::endl;

// Parse the command line arguments.
  double axisDeg;
  std::string argString;
  if (iargc > 1) {
    axisDeg=std::atof(argv[1]); //static_cast<int>()
    argString="Argument #1: ";
  }
  else {
    axisDeg=5.0;
    argString="Argument #1 (default): ";
  }
  std::cout << argString << std::setw(10) << std::setfill(' ')
            << std::setiosflags(outDouble | std::ios::showpoint)
            << std::setprecision(4) << axisDeg
            << std::resetiosflags(outDouble) << std::endl;

  argString="/1rxs_50.out";
  if (iargc > 2) {

    // All subsequent arguments are either option flags or filenames
    for (int i=2; i < iargc; i++) {
      argString=argv[i]; /* convert C string to C++ string */
      if (argString == "-help") {
        help();
        return 0;
      }
      std::cout << "Argument #" << i << ": " << argString << std::endl;    
    } //loop on arguments

  }
  else std::cout << "Argument #2 (default): " << argString << std::endl;

  // Some info on machine dependent constants
  std::cout << "\nsizeof bool, int, long, float, double, pointer = "
            << sizeof(bool) <<", "<< sizeof(int) <<", "<< sizeof(long)
            <<", "<< sizeof(float) <<", "<< sizeof(double)
            <<", "<< sizeof(void *) << std::endl;
  std::cout << "screen outpout of Nan, +infinite, -infinite: " 
            << 0/0. << ",  " << 1/0. << ",  " << -1/0. << "\n" << std::endl;

  std::cout << "Number to unselect = " << std::setiosflags(outDouble)
            << NO_SEL_CUT << "\nConstant arcsecond = "
            << catalogAccess::Min_Axis << "\n" << std::endl;
  std::cout << "The minimum value for double is "
            << std::numeric_limits<double>::min() << std::endl;
  std::cout << "The maximum value for double is "
            << std::numeric_limits<double>::max() << std::endl;
  std::cout << "The epsilon value for double is "
            << std::numeric_limits<double>::epsilon() << std::setprecision(3)
            << std::resetiosflags(outDouble) << std::endl;
/* //not interesting info
  std::cout << "The epsilon value for float  is " << std::setiosflags(outDouble)
            << std::numeric_limits<float>::epsilon() << std::endl;
  std::cout << "The minimum exponent for double is "
            << std::numeric_limits<double>::min_exponent10 << std::endl;
  std::cout << "The maximum exponent for double is "
            << std::numeric_limits<double>::max_exponent10 << std::endl;
  std::cout << "Can double represent: positive infinity, quiet NaN and"
            << " signaling NaN ?\n"
            << std::numeric_limits<double>::has_infinity << ", "
            << std::numeric_limits<double>::has_quiet_NaN << " and "
            << std::numeric_limits<double>::has_signaling_NaN << std::endl;
  std::cout << "How does a double represent positive infinity ?\n"
            << std::numeric_limits<double>::infinity() << std::endl;
  std::cout << "How does a double represent quiet & signaling NaN ?\n"
            << std::numeric_limits<double>::quiet_NaN() << "\n"
            << std::numeric_limits<double>::signaling_NaN()
            << std::resetiosflags(outDouble) << std::endl;
*/
  long numRows=1;
  int vecSize, i, err;
  std::vector<std::string> catNames, webSites;
  std::string strVal;
  std::vector<catalogAccess::Quantity> allQ;

/****************************************************************************/
  show_STEP("\nSTEP 1) METHODS when NOTHING IMPORTED");
  catalogAccess::Catalog *myCat = new catalogAccess::Catalog();
  myCat->getNumRows(&numRows);
  std::cout << "* Number of rows in 'myCat' (new catalog pointer) = "
            << numRows << std::endl;

  std::cout << "\n* Calling: getQuantityDescription" << std::endl;
  allQ.resize(1);
  err=myCat->getQuantityDescription(&allQ);
  vecSize=allQ.size();
  std::cout << "* Value returned = " << err << " (with vector of size "
            << vecSize << ")" << std::endl;

  std::cout << "\n* Calling: getQuantityNames" << std::endl;
  catNames.resize(1);
  err=myCat->getQuantityNames(&catNames);
  vecSize=catNames.size();
  std::cout << "* Value returned = " << err << " (with vector of size "
            << vecSize << ")" << std::endl;

  std::cout << "\n* Calling: getSValue, unsetCuts, setLowerCut" << std::endl;
  err=myCat->getSValue("quant", 0, &strVal);
  myCat->unsetCuts();
  err=myCat->setLowerCut("", 1.0);


  myCat->getCatList(&catNames);
  myCat->getCatList(&webSites, false);
  std::cout << "\n* Available catalogs (with their web query name):"
            << std::endl;
  vecSize=catNames.size();
  for (i=0; i<vecSize; i++) {
    std::cout << std::setw(20) << std::setfill(' ') << catNames[i]
              << " (" << webSites[i] << ")" << std::endl;
  }


/****************************************************************************/
  show_STEP("\nSTEP 2) METHODS when IMPORT FAILS");
  std::cout << "* Try to load via Web the unknown catalog \"toto\""
            << " (pointer 'myCat')" << std::endl;
  err=myCat->importDescriptionWeb("toto");
  i=myCat->importWeb("toto");
  std::cout << "* Values returned = " << err << " then " << i << std::endl;
  myCat->getNumRows(&numRows);
  std::cout << "* Number of rows in 'myCat' = " << numRows << std::endl;

  std::cout << "\n* Try to load via Web catalog \"ROSAT 1RXS\""
            << " (pointer 'myCat')" << std::endl;
  err=myCat->importDescriptionWeb("ROSAT 1RXS", "");
  i=myCat->importWeb("ROSAT 1RXS", "cdu", -1);
  std::cout << "* Values returned = " << err << " then " << i << std::endl;
  myCat->getNumRows(&numRows);
  std::cout << "* Number of rows in 'myCat' = " << numRows << std::endl;

  std::cout << "\n* Calling: importSelected" << std::endl;
  err=myCat->importSelected();
  std::cout << "* Value returned = " << err << std::endl;

  std::cout << "\n* Calling: getQuantityNames" << std::endl;
  err=myCat->getQuantityNames(&catNames);
  vecSize=catNames.size();
  std::cout << "* Value returned = " << err << " (with vector of size "
            << vecSize << ")" << std::endl;

  std::cout << "\n* Calling: getSValue, unsetCuts, setLowerCut" << std::endl;
  err=myCat->getSValue("quant", 0, &strVal);
  myCat->unsetCuts();
  err=myCat->setLowerCut("", 1.0);

  myCat->getWebList(&catNames);
  myCat->getWebList(&webSites, false);
  std::cout << "\n* Possible sites (with their http address):"
            << std::endl;
  vecSize=catNames.size();
  for (i=0; i<vecSize; i++) {
//    std::cout << std::setw(16) << std::setfill('*') << catNames[i]
    std::cout << std::setw(16) << std::setfill(' ') << catNames[i]
              << " (" << webSites[i] << ")" << std::endl;
  }


/****************************************************************************/
  const std::string myPath=st_facilities::Env::getDataDir("catalogAccess");
  if (myPath=="")
    throw std::runtime_error("Environment variable CATALOGACCESSROOT not set.");
/*
  const std::string myPath="/home/aymsauv/GLAST/ZprogU9/unit_test/data";
*/
  strVal=myPath+"/3EG_test.out"; 

  show_STEP("\nSTEP 3) METHODS when IMPORT WORKS (on EGRET)");
  std::cout << "* Calling: importDescription on file \"3EG_test.out\""
            << " (pointer 'myCat')" << std::endl;
  err=myCat->importDescription(strVal);
  vecSize=myCat->getQuantityNames(&catNames);
  std::cout << "* Value returned = " << err << std::endl;
  std::cout << "* Calling: getQuantityNames, get " << vecSize
            << " quantities:" << std::endl;
  for (i=0; i<vecSize; i++) std::cout << catNames[i] << std::endl;
  myCat->getNumRows(&numRows);
  std::cout << "* Number of rows = " << numRows << std::endl;

  std::cout << "\n* Calling: getQuantityDescription, results: "<< std::endl;
  vecSize=myCat->getQuantityDescription(&allQ);
  for (i=0; i<vecSize; i++) show_quant(allQ[i]);

  std::cout << "\n* Calling: getSValue, unsetCuts, setLowerCut" << std::endl;
  err=myCat->getSValue("quant", 0, &strVal);
  vecSize=myCat->unsetCuts();
  i=myCat->setLowerCut("", 1.0);
  std::cout << "* Value returned = " << err <<", "<< vecSize
            << " and " << i << std::endl;

  std::cout << "\n* Calling: importSelected (empty function)" << std::endl;
  err=myCat->importSelected();
  std::cout << "* Value returned = " << err << std::endl;
  myCat->getNumRows(&numRows);
  std::cout << "* Number of rows = " << numRows << std::endl;

  strVal=myPath+"/3EG_test.out"; 
  std::cout << "\n* Calling again: importDescription on file \"3EG_test.out\""
            << std::endl;
  err=myCat->importDescription(strVal);
  std::cout << "* Value returned = " << err << std::endl;

  std::cout << "\n* Calling: import on file \"3EG_test.out\" (with 5 rows)"
            << std::endl;
  err=myCat->import(strVal);
  vecSize=myCat->getQuantityDescription(&allQ);
  myCat->getNumRows(&numRows);
  std::cout << "* Value returned = " << err << std::endl;
  std::cout << "* Number of quantities & rows = "
            << vecSize << " & " << numRows << std::endl;

  std::cout << "\n* Calling: importSelected" << std::endl;
  err=myCat->importSelected();
  std::cout << "* Value returned = " << err << std::endl;

  double rVal;
  std::cout << "\n* Calling: getSValue, three times on row 9" << std::endl;
  err=myCat->getSValue("quant", 9,  &strVal);
  vecSize=myCat->getSValue("3EG",9, &strVal);
  i=myCat->getSValue("DEJ2000", 9,  &strVal);
  std::cout << "* Values returned = " << err <<", "<< vecSize
            << " and " << i << std::endl;
 
  std::cout << "\n* Calling: getSValue, three times on row 0" << std::endl;
  err=myCat->getSValue("quant", 0,  &strVal);
  vecSize=myCat->getSValue("3EG",0, &strVal);
  show_string("3EG", strVal);
  i=myCat->getSValue("DEJ2000", 0,  &strVal);
  std::cout << "* Values returned = " << err <<", "<< vecSize
            << " and " << i << std::endl;

  std::cout << "\n* Calling: getNValue, three times on row 1" << std::endl;
  err=myCat->getNValue("3EG", 1, &rVal);
  if (err > 0) show_double("3EG", rVal);
  vecSize=myCat->getNValue("DEJ2000", 1, &rVal);
  if (vecSize > 0) show_double("DEJ2000", rVal);
  i=myCat->getNValue("z", 1, &rVal);
  if (i > 0) show_double("z", rVal);
  std::cout << "* Values returned = " << err <<", "<< vecSize
            << " and " << i << std::endl;

  std::cout << "\n* Calling: getStatErrorName on \"3EG\"" << std::endl;
  err=myCat->getStatErrorName("3EG", &strVal);
  if (err > 0) show_string("3EG stat error name", strVal);
  std::cout << "* Calling: getSysErrorName on \"DEJ2000\"" << std::endl;
  err=myCat->getSysErrorName("DEJ2000", &strVal);
  if (err > 0) show_string("DEJ2000 sys. error name", strVal);

  std::cout << "\n* Calling: getStatError on \"3EG\" row 0" << std::endl;
  err=myCat->getStatError("3EG", 0, &rVal);
  if (err >= 0) show_double("3EG stat error", rVal);
  std::cout << "* Calling: getSysError on \"DEJ2000\" row 0" << std::endl;
  err=myCat->getSysError("DEJ2000", 0, &rVal);
  if (err >= 0) show_double("DEJ2000 sys. error", rVal);

  std::cout << "\n* Calling: getSvalues on \"DEJ2000\"" << std::endl;
  vecSize=myCat->getSValues("DEJ2000", &catNames);
  std::cout << "* Calling: getSvalues on \"n_theta95\"" << std::endl;
  vecSize=myCat->getSValues("n_theta95", &catNames);
  std::cout << "* Value returned = " << vecSize << ", equal to size of vector"
            << " containing: ";
  for (i=0; i<vecSize; i++) std::cout <<"\""<< catNames[i] <<"\"  ";
  std::cout << std::endl;

  std::cout << "\n* Limits on \"3EG\", \"DEJ2000\", \"z\":"
            << std::endl;
  err=myCat->minVal("3EG", &rVal);
  err=myCat->maxVal("3EG", &rVal);
  err=myCat->minVal("DEJ2000", &rVal);
  if (err > 0) show_double("DEJ2000 minimum", rVal);
  err=myCat->maxVal("DEJ2000", &rVal);
  if (err > 0) show_double("DEJ2000 maximum", rVal);
  err=myCat->minVal("z", &rVal);
  if (err > 0) show_double("z minimum", rVal);
  err=myCat->maxVal("z", &rVal);
  if (err > 0) show_double("z maximum", rVal);


/****************************************************************************/
  show_STEP("\nSTEP 4) COPY CONSTRUCTOR and QUANTITIES");
//try {
  std::cout << "* Default Quantity constructor, changing m_name to: toto"
            << std::endl;
  catalogAccess::Quantity *myQ = new catalogAccess::Quantity();
  myQ->m_name="toto";
  show_quant(*myQ);
  delete myQ;

  std::cout << "\n* Quantity constructor, changing m_name (time),"
            << " m_ucd, m_type (NUM), m_unit, m_index (0)" << std::endl;
  myQ = new catalogAccess::Quantity("time", "", "IJD",
                           catalogAccess::Quantity::NUM, "sec", 0);
  show_quant(*myQ);

  std::cout << "\n* Quantity COPY constructor, changing m_ucd to: IJDelapsed"
            << std::endl;
  catalogAccess::Quantity aQ = *myQ;
  aQ.m_ucd="IJDelapsed";
  show_quant(aQ);
  delete myQ;

  std::cout << "\n* COPY 'myCat' to 'aCat' then DELETE 'myCat" << std::endl;
  catalogAccess::Catalog aCat = *myCat;
  delete myCat;
  aCat.getNumRows(&numRows);
  std::cout << "* Number of rows in 'aCat' = " << numRows << std::endl;

  catNames.assign(3, "");
  aCat.getCatalogTitles(&catNames);
  std::cout << "\n* 'aCat' info (size " <<catNames.size() << "):"
            << "\ncode=\"" << catNames[0] << "\""
            << "\nURL =\"" <<  catNames[1] << "\""
            << "\ncatalog=\"" <<  catNames[2] << "\"" << std::endl;

  std::cout << "\n* Getting all quantities from 'aCat':" << std::endl;
  vecSize=aCat.getQuantityDescription(&allQ);
  for (i=0; i<vecSize; i++) show_quant(allQ[i]);
//} catch (...) { catalogAccess::printErr("copy constructor", ""); }


/****************************************************************************/
  show_STEP("\nSTEP 5) SELECTING REGION after IMPORT (in copy 'aCat')");
  aCat.getNumSelRows(&numRows);
  std::cout << "* Number of selected rows = " << numRows << std::endl;

  std::cout << "\n* Calling: getSelSValue, getSelNValue "
            << "on unknown quantity:" << std::endl;
  err=aCat.getSelSValue("quant", 0, &strVal);
  err=aCat.getSelNValue("quant", 0, &rVal);

  std::cout << "\n* Calling: unsetCuts(), then setSelEllipse four times"
            << " with forbidden parameters:" << std::endl;
  aCat.unsetCuts();
  err=aCat.setSelEllipse(-1, 90.1, 10, 92, 180.1);
  err=aCat.setSelEllipse( 0, 90.1, 10, 92, 180.1);
  err=aCat.setSelEllipse( 0, 32.1, 10, 92, 180.1);
  err=aCat.setSelEllipse( 0, 32.1, 10, 92, 179.9);

  std::cout << "\n* Calling: setSelEllipse, with first main argument"
            << " as rotation:" << std::endl;
  err=aCat.setSelEllipse(1E-5, 73.125, 8.3, 9.2, axisDeg);
  aCat.getNumSelRows(&numRows);
  std::cout << "* Number of SELECTED rows = " << numRows << std::endl;

  std::cout << "\n* String values or limits on \"zu\", \"z\", \"RAJ2000\", "
            << "\"n_theta95\":" << std::endl;
  err=aCat.minSelVal("zu", &rVal);
  err=aCat.minSelVal("z", &rVal);
  if (err > 0) show_double("z minimum (selected)", rVal);
  err=aCat.minSelVal("RAJ2000", &rVal);
  if (err > 0) show_double("RAJ2000 minimum (selected)", rVal);
  err=aCat.maxSelVal("RAJ2000", &rVal);
  if (err > 0) show_double("RAJ2000 maximum (selected)", rVal);
  vecSize=aCat.getSelSValues("n_theta95", &catNames);
  std::cout << "* String vector (size=" << vecSize << ") contains: ";
  for (i=0; i<vecSize; i++) std::cout <<"\""<< catNames[i] <<"\"  ";
  std::cout << std::endl;

  std::cout << "\n* Calling: setSelEllipse, with first main argument"
            << " as circle axis:" << std::endl;
//  err=aCat.setSelEllipse(156.95, -62.0, axisDeg, axisDeg);
  err=aCat.setSelEllipse(0, -90., axisDeg, axisDeg);
  aCat.getNumSelRows(&numRows);
  std::cout << "* Number of SELECTED rows = " << numRows << std::endl;

  std::cout << "\n* String values or limits on \"zu\", \"RAJ2000\", "
            << "\"n_theta95\":" << std::endl;
  err=aCat.minSelVal("zu", &rVal);
  if (err > 0) show_double("zu minimum (selected)", rVal);
  err=aCat.minSelVal("RAJ2000", &rVal);
  err=aCat.maxSelVal("RAJ2000", &rVal);
  vecSize=aCat.getSelSValues("n_theta95", &catNames);
  std::cout << "* String vector (size=" << vecSize << ")" << std::endl;


/****************************************************************************/
  show_STEP("\nSTEP 6) READING FILE for IMPORT (in copy 'aCat')");
  std::cout << "* Calling: importDescription(\"\"):" << std::endl;
  err=aCat.importDescription("");
  std::cout << "* Same call after deleteContent():" << std::endl;
  aCat.deleteContent();
  err=aCat.importDescription("");

  std::cout << "\n* Calling: importDescription(\"totoX\") then "
            << " importDescription(\"../..\")" << std::endl;
  err=aCat.importDescription("totoX");
  i=aCat.importDescription("../..");
  std::cout << "* Values returned = " << err <<", "<< i << std::endl;

  std::cout << "\n* Calling: importDescription on file \"1rxs_50.fits\""
            << std::endl;
  strVal=myPath+"/1rxs_50.fits";  //"../../1rxs_50.fits";
  err=aCat.importDescription(strVal);
  std::cout << "* Value returned = " << err << std::endl;

  std::cout << "\n* Calling: importDescription on \"1rxs_50.fits[1]\""
            << std::endl;
  strVal=myPath+"/1rxs_50.fits[1]";
  err=aCat.importDescription(strVal);
  std::cout << "* Value returned = " << err << std::endl;


  std::cout << "\n* Calling: import on file \"" << argString
             << "\""<< std::endl;
  strVal=myPath+argString;
  err=aCat.import(strVal);
  std::cout << "* Value returned = " << err << std::endl;
  err=aCat.getQuantityNames(&catNames);
  aCat.getNumRows(&numRows);
  std::cout << "* Number of quantities & rows = "
            << err << " & " << numRows << std::endl;

  catNames.assign(6, "");
  aCat.getCatalogTitles(&catNames);
  std::cout << "* 'aCat' info (size " <<catNames.size() << "):"
            << "\ncode=\"" << catNames[0] << "\""
            << "\nURL=\"" <<  catNames[1] << "\""
            << "\ncatalog=\"" <<  catNames[2] << "\"" 
            << " (" <<  catNames[3] << ")" 
            << "\ntable  =\"" <<  catNames[4] << "\"" 
            << " (" <<  catNames[5] << ")" 
            << std::endl;
  
  std::cout << "\n* Calling: getQuantityDescription, results:" << std::endl;
  vecSize=aCat.getQuantityDescription(&allQ);
  for (i=0; i<vecSize; i++) show_quant(allQ[i]);
/*  aCat.getQuantityNames(&catNames);
  aCat.getQuantityUnits(&webSites);
  for (i=0; i<vecSize; i++) 
    std::cout << catNames[i] <<": \""<< webSites[i] <<"\""<< std::endl;
*/


/****************************************************************************/
  show_STEP("\nSTEP 7) SELECTING (in copy 'aCat')");
  aCat.getNumRows(&numRows);
  std::cout << "* Number of rows = " << numRows << std::endl;

  std::cout << "\n* Calling: setSelEllipse "
            << "(selecting object in North hemisphere)" << std::endl;
  err=aCat.setSelEllipse(0, 90., 90, 90);
  aCat.getNumSelRows(&numRows);
  std::cout << "* Number of SELECTED rows = " << numRows << std::endl;

  std::cout << "\n* Calling: set cut (on L_Extent)" << std::endl;
  aCat.setLowerCut("L_Extent", 1.1);
  aCat.getNumSelRows(&numRows);
  std::cout << "* Number of SELECTED rows = " << numRows << std::endl;
  aCat.setLowerCut("L_Extent", 1);
  aCat.getNumSelRows(&numRows);
  std::cout << "* Number of SELECTED rows = " << numRows << std::endl;
  aCat.setUpperCut("L_Extent", 30);
  aCat.getNumSelRows(&numRows);
  std::cout << "* Number of SELECTED rows = " << numRows << std::endl;
  aCat.setUpperCut("L_Extent", NO_SEL_CUT);
  aCat.getNumSelRows(&numRows);
  std::cout << "* Number of SELECTED rows = " << numRows << std::endl;

  std::cout << "\n* Calling: set cut (on MASOL)" << std::endl;
  aCat.setLowerCut("MASOL", 31);
  aCat.getNumSelRows(&numRows);
  std::cout << "* Number of SELECTED rows = " << numRows << std::endl;
  aCat.setUpperCut("MASOL", 100);
  aCat.getNumSelRows(&numRows);
  std::cout << "* Number of SELECTED rows = " << numRows << std::endl;
  std::cout << "* Calling: unsetCuts (on MASOL)" << std::endl;
  aCat.unsetCuts("MASOL");
  aCat.getNumSelRows(&numRows);
  std::cout << "* Number of SELECTED rows = " << numRows << std::endl;
  aCat.setUpperCut("MASOL", 100);
  aCat.getNumSelRows(&numRows);
  std::cout << "* Number of SELECTED rows = " << numRows << std::endl;

  std::cout << "\n* Calling: eraseNonSelected()" << std::endl;
  err=aCat.eraseNonSelected();
  aCat.getNumSelRows(&numRows);
  std::cout << "* Number of SELECTED rows = " << numRows << std::endl;

  std::cout << "\n* Calling: setUpperCut (on MASOL)" << std::endl;
  aCat.setUpperCut("MASOL", 100);
  aCat.getNumSelRows(&numRows);
  std::cout << "* Number of SELECTED rows = " << numRows << std::endl;

  strVal=myPath+argString+".txt";
  std::cout << "\n* Calling: save(" << strVal << ")" << std::endl;
  err=aCat.save(strVal, false);

  std::cout << "\n* Calling: eraseSelected()" << std::endl;
  err=aCat.eraseSelected();
  aCat.getNumSelRows(&numRows);
  std::cout << "* Number of SELECTED rows = " << numRows << std::endl;
  aCat.getNumRows(&numRows);
  std::cout << "* Number of rows = " << numRows << std::endl;


/****************************************************************************/
  std::cout << "\n!END PROGRAM!\n(as pointers are already deleted, only free"
            << " the Quantity and Catalog copies)." << std::endl;
}

void help() {
 std::cerr << "usage: <program name> <axis size> [<options> <fileName>]\n"
           << "options: \n"
           << "  -help  to show this help\n"
           << std::endl;
}
void show_STEP(const std::string text) {
  int len=text.length();
  std::cout << "\n" << std::setiosflags(std::ios::left)
            << std::setw(len) << std::setfill('=') << "\n" << text 
            << std::setw(len) << std::setfill('=') << "\n" << std::endl;
}

void show_quant(const catalogAccess::Quantity &nQ) {
  std::cout << nQ.m_name <<": ucd=\""<< nQ.m_ucd <<"\", type="<< nQ.m_type
           <<", unit=\""<< nQ.m_unit <<"\", format=\""<< nQ.m_format
          <<"\",\n     index="<< nQ.m_index <<", boolGeneric="<< nQ.m_isGeneric
         << ", boolLoad="<< nQ.m_toBeLoaded <<", boolNaN="<< nQ.m_rejectNaN
        <<"\n     selectList sizes=("<< nQ.m_listValN.size() <<" num, "
       << nQ.m_listValS.size() <<" str), cuts=" << nQ.m_lowerCut <<" to "
      << nQ.m_upperCut << "\ncomment=\""<< nQ.m_comment << "\"" << std::endl;
}
void show_string(const std::string name, const std::string val) {
  std::cout << "Quantity " << name << ": \"" << val <<"\""<< std::endl; 
}
void show_double(const std::string name, const double val) {
  std::cout << "Quantity " << name << " = " //<< std::setprecision(4)
//            << val << std::endl;
            << std::setiosflags(outDouble) << std::setprecision(3)
            << std::setw(11) << std::setfill(' ')
            << val << std::resetiosflags(outDouble) << std::endl;
}
