/**
 * @file   catalog.h
 * @brief  Declaration for the Catalog class.
 * Four symbolic constants are defined (used for C array size).
 *
 * @author A. Sauvageon
 *
 * $Header $
 */

#ifndef catalogAccess_cat_h
#define catalogAccess_cat_h

#include "quantity.h"
// can compile without first three
//#include <cctype>      //for toupper, tolower
//#include <algorithm>   //for transform
//#include <cstdio>      //for sprintf
#include <fstream>    //for ifstream, ofstream
#include <dirent.h>   //for DIR type 
#include <iomanip>    //for setprecision, _Ios_Fmtflags, ...
#include <stdexcept>  //for std::runtime_error

#define MAX_CAT 7               // number of known catalogs
#define MAX_GEN 6               // number of generic quantities
#define MAX_URL 9               // number of known VizieR web address
#define MAX_LINE 1024           // (maximum number of char)+1 with getline
namespace catalogAccess {

/**
 * @class   Catalog
 *
 * @brief  Provide methods to define a catalog and access its data.
 * Only inline methods (default constructor, the destructor and 7
 * private checking methods) are implemented here.
 *
 * @author A. Sauvageon
 *
 * $Header $
 */

class Catalog {

public:

  Catalog();                    // Default constructor
  ~Catalog();                   // Destructor needed to free memory
  Catalog(const Catalog & );    // Copy constructor needed


  // Methods giving general information
  //-----------------------------------

  void getCatList(std::vector<std::string> *names, const bool isCode=true);
      // return a list of all supported catalog names
  void getWebList(std::vector<std::string> *names, const bool isCode=true);
      // return a list of all supported web site names


  // Methods for importing, saving, loading
  //---------------------------------------

  int importDescription(const std::string &fileName);
  int importDescriptionWeb(const std::string catName,
                           const std::string urlCode="cds",
                           const std::string &fileName="");
      // read only the catalog description (i.e. fill the
      // m_quantities vector and m_code, etc.)
      // the method returns the number of quantities,
      // -1 if importDescription already done, -3 if catName is unknown
      // other negative number for loading error


  int import(const std::string &fileName);
  int importWeb(const std::string catName, const std::string urlCode="cds",
                const long maxRow=44000, const std::string &fileName="");
      // import method for loading an entire catalog without selection
      // * recognizes catalog name and identifies the files to load
      // * translates the generic quantities
      // * loads non-generic quantities
      // the method returns the number of loaded rows,
      // -1 if successful import already done, -3 if catName is unknown,
      // other negative number for loading error

  int importSelected();
      // if a catalog description was already loaded, this method does
      // the same as import(). However, it applies selection criteria
      // such that quantities which are not passing the criteria are not 
      // loaded;
      // the method returns the number of loaded rows,
      // -1 if successful import already done, -2 if importDescription not done
      // other negative number for loading error      

  void deleteContent();
      // erase m_strings, m_numericals but keep catalog definition

  int save(const std::string fileName, bool no_replace);
      // save the catalog information presently in memory to a FITS file
      // the method returns 1 if successful, negative number otherwise

  int saveSelected(const std::string fileName, bool no_replace);
      // like save(), however, storing only the selected rows
      // the method returns 1 if successful, negative number otherwise

  int load(const std::string fileName);
      // same as import(), only the data is loaded from a FITS file
      // compatible with the save() method


  // Methods for accessing data
  //---------------------------

  // accessing the Catalog definition

  void getCatalogTitles(std::vector<std::string> *titles);
      // get the 6 definition strings if vector size is enough
      // otherwise get only m_code (size 0 or 1) or m_URL, etc.
/*  void getQuantityIter(std::vector<Quantity>::const_iterator *iter);*/
      // get an iterator on the quantities
  int getQuantityDescription(std::vector<Quantity> *myQuantities);
      // get a copy of the quantity vector
  int getQuantityNames(std::vector<std::string> *names);  // get only the names
  int getQuantityUnits(std::vector<std::string> *units);  // get only the units
  int getQuantityUCDs (std::vector<std::string> *ucds);   // get only the UCDs
  int getQuantityTypes(std::vector<Quantity::QuantityType> *types);
      // get only the types

  int getStatErrorName(const std::string name, std::string *statErrName);
      // get the member "statErrName" of given quantity "name",
      // identifying the statistical error quantity
  int getSysErrorName(const std::string name, std::string *sysErrName);
      // get the member "sysErrName" of given quantity "name",
      // identifying the systematic error quantity
/*  int getVecQNames(const std::string name,
                   std::vector<std::string> *vecNames);*/
      // get the list of the names of the quantities which
      // form the elements of the given VECTOR "name"


  // accessing all the Catalog contents in memory
  //       (ignoring in-memory selection criteria)

  void getNumRows(long *nrows);  // get the number of rows in the catalog
  int getSValue(const std::string name, const long row, std::string *stringVal);
      // get the value of the given string quantity "name"
      // in the given catalog row
  int getNValue(const std::string name, const long row, double *realVal);
      // get the value of the given numerical quantity "name"
      // in the given catalog row
/*  int getVecValues(const std::string name, const  int row,
                   std::vector<double> *vecVal);*/
      // get the values of the given vector quantity "name"
      // in the given catalog row
  int getStatError(const std::string name, const long row, double *realValStat);
      // get the value of the statistical error associated with quantity "name"
      // in the given catalog row;
      // value is negative if unavailable
  int getSysError(const std::string name, const long row, double *realValSys);
      // get the value of the sytematic error associated with quantity "name"
      // in the given catalog row;
      // value is negative if unavailable
/*  int getVecStatErrors(const std::string name, const long row,
                       std::vector<double> *vecValStat);*/
      // get the statistical errors associated with vector quantity "name"
      // in the given catalog row
/*  int getVecSysErrors(const std::string name, const long row,
                      std::vector<double> *vecValSys);*/
      // get the systematic errors associated with vector quantity "name"
      // in the given catalog row
  int getObjName(const long row, std::string *stringVal);
      // access to generic quantity for object name
  int ra_deg(const long row, double *realVal);
      // access to generic quantity for RA  (degrees)
  int dec_deg(const long row, double *realVal);
      // access to generic quantity for DEC (degrees)
  int posError_deg(const long row, double *realVal);
      // access to generic quantity for position uncertainty (degrees)
  int l_deg(const long row, double *realVal);
      // access to generic quantity for l (degrees)
  int b_deg(const long row, double *realVal);
      // access to generic quantity for b (degrees)

  // possible values or range of a given quantity "name"

  int getSValues(const std::string name, std::vector<std::string> *values);
      // for numerical quantity: empty
      // for string quantity: the list of different values assumed
  int minVal(const std::string name, double *realVal);
      // for string quantity: undefined; default == NO_SEL_CUT
      // for numerical quantity: the minimum value in the catalog
  int maxVal(const std::string name, double *realVal);
      // for string quantity: undefined; default == NO_SEL_CUT
      // for numerical quantity: the maximum value in the catalog


  // accessing the Catalog contents with in-memory selection criteria applied
  // the row index relates to the selected rows, i.e. is continuous!

  void getNumSelRows(long *nrows);
      // get the number of selected rows in the catalog
  int getSelSValue(const std::string name, const long srow,
                   std::string *stringVal);
  int getSelNValue(const std::string name, const long srow, double *realVal);
/*  int getSelVecValues(const std::string name, const long srow,
                      std::vector<double> *vecVal);*/
  int getSelStatError(const std::string name, const long srow,
                      double *realValStat);
  int getSelSysError(const std::string name, const long srow,
                     double *realValSys);
/*  int getSelVecStatErrors(const std::string name, const long srow,
                          std::vector<double> *vecValStat);
  int getSelVecSysErrors(const std::string name, const long srow,
                       std::vector<double> *vecValSys);*/
  int getSelObjName(const long srow, std::string *stringVal);
  int selRA_deg(const long srow, double *realVal);
  int selDEC_deg(const long srow, double *realVal);
  int selPosError_deg(const long srow, double *realVal);
  int selL_deg(const long srow, double *realVal);
  int selB_deg(const long srow, double *realVal);
  // more quantities here? (the same as above of course)


  // possible values or range of a given quantity "name" in the selected rows

  int getSelSValues(const std::string name, std::vector<std::string> *values);
  int minSelVal(const std::string name, double *realVal);
  int maxSelVal(const std::string name, double *realVal);


  // Methods for selecting data
  //---------------------------

  // All setting or unsetting of cuts immediately takes effect,
  // i.e. the value of the vector m_rowIsSelected is recalculated.
  // The next time any of the above getSelectValue methods is
  // used, it is taking into account the changed cuts.

  int unsetCuts();              // unset all cuts on all quantities except
                                // the selection ellipse;
                                // this also deletes the selection string

  // comment: all 'unset' methods have only an effect if the cuts were applied 
  // after loading the data AND the eraseNonSelected method was not called

  int unsetCuts(const std::string name);
      // unset all selection criteria relating to quantity "name"
  int setLowerCut(const std::string name, double cutVal);
      // set and apply a cut on quantity "name" (all values >= cutVal pass)
      // double is not const because it can be locally modified to NO_SEL_CUT
  int setUpperCut(const std::string name, double cutVal);
      // set and apply a cut on quantity "name" (all values <= cutVal pass)
      // double is not const because it can be locally modified to NO_SEL_CUT
/*  int setLowerVecCuts(const std::string name,
                      const std::vector<double> &cutValues);*/
      // set and apply a cuts on quantities in VECTOR type quantity "name"
      // such that all values >= cutValues[i] pass
/*  int setUpperVecCuts(const std::string name,
                      const std::vector<double> &cutValues);*/
      // set and apply a cut on quantities in VECTOR type quantity "name"
      // such that all values <= cutValues[i] pass
  int excludeS(const std::string name,
               const std::vector<std::string> &stringList, bool exact=false);
      // exclude all rows which have string value in the given list
  int useOnlyS(const std::string name,
               const std::vector<std::string> &stringList, bool exact=false);
      // only include rows which have string value in the given list

  int excludeN(const std::string name, const std::vector<double> &listVal);
      // exclude all rows which have value around one numerical in list
  int useOnlyN(const std::string name, const std::vector<double> &listVal);
      // only include all rows which value around one numerical in list
  int setRejectNaN(const std::string name, const bool rejectNaN);
      // set quantity member m_rejectNaN and apply
      // (if different from previous and quantity is selected)  
  int setMatchPercent(const std::string name, double percent);
  int setMatchEpsilon(const std::string name, const unsigned long step);
      // set quantity member m_precision and apply
      // (if quantity has a non empty selection list)   

  int setSelEllipse(const double centRA_deg, const double centDEC_deg,
                    const double majAxis_deg, const double minAxis_deg,
                    const double rot_deg=0.);
     // set and apply an elliptical selection region
     // (box cuts of constant size CANNOT be achieved)
  int unsetSelEllipse();        // remove the effects of the ellipse selection
  int eraseNonSelected();
     // erase all non-selected rows from memory
  int eraseSelected();
    // erase all selected rows from memory

  // for convenience: methods for cutting on the generic quantities

  void unSetCutsObjName();
      // unset all selection criteria relating to the object name
  void excludeObjName(const std::string stringVal);
      // exclude all rows which have object name == stringVal
  void useOnlyObjName(const std::string stringVal);
      // only include all rows which have object name == stringVal
  void unsetCutsRA();
      // unset all selection criteria relating to RA
  void setMinRA(double cutVal);
      // set and apply a lower cut on RA
  void setMaxRA(double cutVal);
      // set and apply an upper cut on RA
  void unsetCutsDEC();
      // unset all selection criteria relating to DEC
  void setMinDEC(double cutVal);
      // set and apply a lower cut on DEC
  void setMaxDEC(double cutVal);
      // set and apply an upper cut on DEC
  void unsetCutsL();
      // unset all selection criteria relating to L
  void setMinL(double cutVal);
      // set and apply a lower cut on L
  void setMaxL(double cutVal);
     // set and apply an upper cut on L
  void unsetCutsB();
      // unset all selection criteria relating to B
  void setMinB(double cutVal);
      // set and apply a lower cut on B
  void setMaxB(double cutVal);
      // set and apply an upper cut on B

  // general cut described by string to be parsed

/*  int setCutString(const std::string stringVal);*/
      // set the selection string m_selection
      // Syntax:
      // a) quantities are described by giving their name
      //    in square brackets, e.g. [f6cm]
      // b) otherwise FORTRAN syntax is used, e.g.
      //   "[f6cm].geq.[f12cm].and.([hr1]-[hr2]).lt.2.3"
      //
      // returns 1 if expression is correct, negative number otherwise

/*  void getCutString(std::string *stringVal);*/
      // get a copy of m_selection
    
/*  int selStringTrue(const long row, bool *isSelected);*/
      // returns true if the condition described by m_selection is true
      // for the given row, otherwise false


  // Methods for sorting
  //--------------------

  // will use sorting methods present in the standard template library

  void sortAscend(const std::string quantityName);
      // reassign the row numbers by sorting by the given
      // quantity in ascending order

  void sortDecend(const std::string quantityName);
      // reassign the row numbers by sorting by the given
      // quantity in decending order


/**********************************************************************/
private:

  std::string m_code;
  std::string m_URL;
  std::string m_catName;
  std::string m_catRef;
  std::string m_tableName;
  std::string m_tableRef;

  std::vector<Quantity> m_quantities; // the definition of the catalog

  std::vector<std::vector<std::string> > m_strings;
      // stores all string contents of the catalog;
      // e.g. m_strings[3] gives you the vector containing
      // the values of quantity 3 from the catalog;
      // m_strings[3][25] gives you the value of quantity 3 for catalog entry 25

  // comment: the number string quantities in the catalog == m_strings.size()

  std::vector<std::vector<double> > m_numericals;
      // stores all numerical contents of the catalog

  // comment: the number of numerical quantities == m_numericals.size()

  long m_numRows;
      // number of catalog rows loaded into memory
      // ( == m_strings[0].size() if m_strings.size() != 0 which should always
      //   be true due to the existence of generic quantities )
      // ( also == m_numericals[0].size if m_numericals.size != 0 which should
      //   always be true due to the existence of generic quantities )

  std::vector<std::vector<unsigned long> > m_rowIsSelected; 
      // false by default, i.e. all bits to 0;
      // each vector column has m_numRows elements; for a given row:
      // first bit to 1 if all selection criteria are met
      // each following bits for one quantity criteria

  std::string m_selection;      // to contain a general cut which is parsed
                                // by the method setCutString()
  bool m_criteriaORed;
      // if true: OR instead of AND between bits of m_rowIsSelected

  bool m_selRegion;
      // true if an elliptical region is to be selected;
      // default == false
  double m_selEllipseCentRA_deg;
      // the RA of the center of the ellipse (degrees)
  double m_selEllipseCentDEC_deg;
      // the DEC of the center of the ellipse (degrees)
  double m_selEllipseMinAxis_deg;
      // the size of the minor axis (degrees)
  double m_selEllipseMajAxis_deg;
      // the size of the major axis (degrees)
  double m_selEllipseRot_deg;
      // the rotation angle, i.e. the angle between major axis and the
      // celestial equator (degrees); default == 0

  // following four data members needed for efficient selection
  long m_numSelRows;            // for quick test: 0 = nothing selected
  int m_indexRA;                // index for RA in m_numericals
  int m_indexDEC;               // index for DEC in m_numericals
  std::vector<double> m_selEllipse;
      // the sinus and cosinus of the 2 spherical angles + ellipse size

  bool checkRegion(const long row, const int nRA, const int nDEC);
      // to check if given row is inside the elliptical region,
      // nRA and nDEC are the position inside m_quantities.
  bool checkSTR(const std::string s, const int index, const int code);
      // to check if given value pass list criteria for given quantity index
  bool checkNUM(const double r, const int index, const bool reject,
                const double precis);
      // to check if given value pass 3 criteria for given quantity index
  bool rowSelect(const long row, const std::vector<bool> &quantSel);
      // computes the global row selection from bits in m_rowIsSelected
  int doSelS(const std::string name, const std::string origin,
             const std::vector<std::string> &stringList, bool exact);
      // select rows depending on the given string list
  int doSelN(const std::string name, const std::string origin,
             const std::vector<double> &listVal);
      // select rows depending on the given numerical list

  void deleteDescription();
      // erase the changes made by "importDescription"
      // must call first "deleteContent" if "import" or "importSelected" done

  void setGeneric(const int whichCat);
      // called just after "import..." to set m_isGeneric, m_indexRA, m_indexDEC
  void create_tables(const int nbQuantAscii);
      // creates a new column in m_strings, m_numericals
  void add_row();
      // creates a new row in m_strings, m_numericals
  void translate_cell(std::string mot, const int index);
      // loads one quantity at last row (m_numRows);

  int analyze_fits(const std::string &fileName, const bool getAll,
                   const std::string origin);
  int analyze_text(const std::string &fileName, const bool getAll,
                   const std::string origin);
      // both methods read file for import or importDescription (getAll=false)
      // returns IS_OK for completion, otherwise BAD_FILETYPE or tip error

  void showRAMsize(long numRows=44000);
      // output the estimation of RAM needed per data row
  int load(const std::string &fileName, const bool getAll);
      // common code between import and importDescription
  int loadWeb(const std::string catName, const std::string urlCode,
              const std::string &fileName, const long maxRow);
      // common code between importWeb and importDescriptionWeb
 
  // inline private methods

  int checkImport(const std::string origin, const bool isDone);
      // return -1 or -2 if problem, 0 or positive number otherwise
      // if isDone is true, cannot return 0=IS_VOID
      // if isDone is true and import is done, return m_quantities.size
      // if isDone is false and import NOT done, return 0
  int checkCatName(const std::string origin, const std::string catName);
      // return -3 if catName do not exist, its index otherwise
  int checkSize_row(const std::string origin, const long row);
      // return strictly positive number (m_numRows) if row exist
  int checkQuant_name(const std::string origin, const std::string name);
      // return negative number if problem, quantity index otherwise
  int checkSel_row(const std::string origin, const long srow);
      // return strictly positive number (m_numSelRows) if selected row exist

  bool existCriteria(std::vector<bool> *quantSel);
      // returns true if a region or at least one quantity is selected
  unsigned long bitPosition(const int index, int *k);
      // returns the number to test quantity[index] bit in m_rowIsSelected[k] 

  // constant members
  static const char *s_CatalogURL[MAX_URL];
  static const char *s_CatalogList[2*MAX_CAT];
  static const char *s_CatalogGeneric[MAX_CAT][MAX_GEN];

}; // end top class definition


/**********************************************************************/
/*  DEFINING inline FUNCTION MEMBERS                                  */
/**********************************************************************/

// Default constructor
inline Catalog::Catalog() {


  m_code="";
  m_URL ="";
  m_catName   ="";
  m_catRef    ="";
  m_tableName ="";
  m_tableRef  ="";
  m_numRows   =0;
  m_selection ="";
  m_criteriaORed=false;
  m_selRegion =false;
  // following four data members needed for efficient selection
  m_numSelRows=0;
  m_indexRA = -1;
  m_indexDEC= -1;
  try { m_selEllipse.assign(7, 0.0); }
  catch (std::exception &err) {
    std::string errText;
    errText=std::string("EXCEPTION on creating m_selEllipse: ")+err.what();
    printErr("Catalog constructor", errText);
    throw;
  }
  #ifdef DEBUG_CAT
  std::cout << "!! DEBUG Catalog constructor (sizes=" <<  m_quantities.size()
            <<","<< m_strings.size() <<","<< m_numericals.size() <<","
            << m_rowIsSelected.size() <<", m_selEllipse "<< m_selEllipse.size()
            << ")" << std::endl;
  #endif
}

/**********************************************************************/
// Destructor needed to free memory
inline Catalog::~Catalog() {

  #ifdef DEBUG_CAT
  std::cout << "!! DEBUG Catalog destructor on: "
            << m_tableName << std::endl;
  #endif
  deleteContent();
  deleteDescription();
}

/**********************************************************************/
// To check if import or importDescription already done
inline int Catalog::checkImport(const std::string origin, const bool isDone) {

  int quantSize=m_quantities.size();
  if (quantSize > 0) {
    if (isDone) return quantSize;
    if (m_numRows <= 0) {
      printLog(2, "deleting previous Catalog description");
      deleteDescription();
      return IS_VOID;
    }
    printWarn(origin, "call 'deleteContent' before importing again");
    return IMPORT_BIS;
  }
  else {
    if (! isDone) return IS_VOID;
    printWarn(origin, "must first use one 'import' method");
    return IMPORT_NEED;
  }
}

/**********************************************************************/
// To check if catName is valid 
inline int Catalog::checkCatName(const std::string origin,
                                 const std::string catName) {

  for (int i=0; i<MAX_CAT; i++) {
    if (Catalog::s_CatalogList[2*i] == catName) return i;
  }
  std::string errText;
  errText="given Catalog name ("+catName+") do not exist";
  printWarn(origin, errText);
  return BAD_CATNAME;
}

/**********************************************************************/
// To check if catalog was succesfully loaded and row exist
inline int Catalog::checkSize_row(const std::string origin, const long row) {

  if (m_numRows <= 0) {
    printWarn(origin, "catalog is empty");
    return IS_VOID;
  }
  if ((row < 0) || (row >= m_numRows)) {
    std::ostringstream sortie;
    sortie << "row must be within [ 0, " << m_numRows-1 << "]";
    printWarn(origin, sortie.str());
    return BAD_ROW;
  }
  return m_numRows;
}

/**********************************************************************/
// To check if quantity name is valid
inline int Catalog::checkQuant_name(const std::string origin,
                                    const std::string name) {

  int quantSize=m_quantities.size();
  for (int i=0; i<quantSize; i++) {
    if (m_quantities[i].m_name == name) return i;
  }
  std::string errText;
  errText="given Quantity name ("+name+") do not exist";
  printWarn(origin, errText);
  return BAD_QUANT_NAME;
}

/**********************************************************************/
// To check if catalog was succesfully loaded and selected row exist
inline int Catalog::checkSel_row(const std::string origin, const long srow) {

  if (m_numRows <= 0) {
    printWarn(origin, "catalog is empty");
    return IS_VOID;
  }
    if (m_numSelRows == 0) {
    printWarn(origin, "no row is selected");
    return IS_VOID;
  }
  if ((srow < 0) || (srow >= m_numSelRows)) {
    std::ostringstream sortie;
    sortie << "row must be within [ 0, " << m_numSelRows-1 << "]";
    printWarn(origin, sortie.str());
    return BAD_ROW;
  }
  return m_numSelRows;
}

/**********************************************************************/
// returns true if a region or at least one quantity is selected
inline bool Catalog::existCriteria(std::vector<bool> *quantSel) {

  bool all=false;
  std::vector<Quantity>::const_iterator itQ;
  quantSel->clear();
  try {
    if (!m_selRegion) quantSel->push_back(false);
    else { all=true;  quantSel->push_back(true); }
    for (itQ=m_quantities.begin(); itQ != m_quantities.end(); itQ++) {

      if (itQ->m_type == Quantity::STRING) {
        if (itQ->m_listValS.size() > 0) {
          all=true;
          quantSel->push_back(true);
        }
        else quantSel->push_back(false);
      }

      else if (itQ->m_type == Quantity::NUM) {
        if ( (itQ->m_lowerCut < NO_SEL_CUT)||(itQ->m_upperCut < NO_SEL_CUT)
            || (itQ->m_listValN.size() > 0) ) {
          all=true;
          quantSel->push_back(true);
        }
        else quantSel->push_back(false);
      }

      else quantSel->push_back(false);
      // VECTOR quantity are selected by the quantities in m_vectorQs

    }// loop on quantities
  }
  catch (std::exception &err) {
    std::string errText=std::string("EXCEPTION on boolean vector: ")+err.what();
    printErr("private existCriteria", errText);
    throw;
  }
  return all;
}

/**********************************************************************/
// private method, suppose given quantity index exist
inline unsigned long Catalog::bitPosition(const int index, int *k) {

  unsigned long test=1ul;
  int pos=index+1; // position of given criteria in isSelected vector
  *k=(pos+1)/(sizeof(long)*8);
  // index of m_rowIsSelected with given criteria
  int i=pos+1-(*k)*sizeof(long)*8;
  // bit pos.of given criteria in m_rowIsSelected[k]
  int j=0;
  while (j < i) {test*=2ul; j++;}

  return test;
}

} // namespace catalogAccess
#endif // catalogAccess_cat_h
