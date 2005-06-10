/**
 * @file   catalog_sel.cxx
 * @brief  Selection routines for Catalog class.
 * Provide methods to MAKE the selection (READING selection is in catalog.cxx).
 * Selection can be defined with no row, it will be used with
 * importSelected() method.
 *
 * @author A. Sauvageon
 *
 * $Header $
 */

#include "catalog.h"

namespace catalogAccess {

/**********************************************************************/
/*  METHOD for SELECTING QUANTITY (BEFORE IMPORT)                     */
/**********************************************************************/
// importSelected() will actually load selected quantities
int Catalog::selectQuantity(const std::string name, const bool toBeLoaded) {

  int quantSize=checkImport("selectQuantity", true);
  if (quantSize < IS_VOID) return quantSize;
  int num=checkQuant_name("selectQuantity", name);
  if (num < 0) return num;

  int maxSize=m_loadQuantity.size();
  if (quantSize != maxSize) {
    printWarn("selectQuantity",
             "cannot get all initial quantities once importSelected() done");
    return BAD_SEL_QUANT;
  }
  m_loadQuantity.at(num)=toBeLoaded;
  return IS_OK;
}
/**********************************************************************/
int Catalog::selectAllQuantities(const bool toBeLoaded) {

  int quantSize=checkImport("selectAllQuantities", true);
  if (quantSize < IS_VOID) return quantSize;

  // beware m_loadQuantity has always the initial size
  // whereas m_quantities size can decrease
  int maxSize=m_loadQuantity.size();
  if (quantSize != maxSize) {
    printWarn("selectAllQuantities",
             "cannot get all initial quantities once importSelected() done");
    return BAD_SEL_QUANT;
  }
  if (toBeLoaded) // vectors with same size
       m_loadQuantity.assign(maxSize, true);
  else m_loadQuantity.assign(maxSize, false);
  return IS_OK;
}


/**********************************************************************/
/*  METHODS for SELECTING DATA (BEFORE or AFTER IMPORT)               */
/**********************************************************************/
// check if given row is inside the elliptical region (private method)
bool Catalog::checkRegion(const long row, const int nRA, const int nDEC) {

  /* for the moment, only circle
    if angle phi=RA and t=PI/2 - DEC then:
    OM postion is x= (sin t <=> cos DEC) * cos phi
                  y= (sin t <=> cos DEC) * sin phi
                  z= (cos t <=> sin DEC)
   and the circular region around OA is defined by
   its scalar product with OM > cos desired_angle
  */
  double myRA=m_numericals[nRA].at(row);
  double myDEC=m_numericals[nDEC].at(row);
  if (isnan(myRA))  return !(m_quantities[m_indexRA].m_rejectNaN);
  if (isnan(myDEC)) return !(m_quantities[m_indexDEC].m_rejectNaN);
  double obj_cosP=cos(myRA * Angle_Conv);
  double obj_sinP=sin(myRA * Angle_Conv);
  double obj_sinT=cos(myDEC * Angle_Conv);
  double obj_cosT=sin(myDEC * Angle_Conv);
  double myAngle=obj_sinT*obj_cosP*m_selEllipse.at(2)*m_selEllipse.at(0)
                +obj_sinT*obj_sinP*m_selEllipse.at(2)*m_selEllipse.at(1)
                +obj_cosT*m_selEllipse.at(3);
/*std::cout << "cos(angle)*1E6 = " << myAngle*1E6 << " ? "
            <<  m_selEllipse.at(4)*1E6 << std::endl;*/
  if (myAngle >= m_selEllipse.at(4)) return true;
  return false;
}

/**********************************************************************/
// check if value pass criteria for given quantity index (cut AND list)
// private method called by useOnlyN, excludeN
bool Catalog::checkNUM(const double r, const int index, const bool miss,
                       const bool reject, const double precis) {

  // since we test the NaN only once at the beginning
  // this methods MUST NOT be called if no criteria is applied
  if (isnan(r)) return !reject;

  double myLimit;
  myLimit=m_quantities[index].m_lowerCut;
  if (myLimit < NO_SEL_CUT) {
    if (r < myLimit) return false;
  }
  myLimit=m_quantities[index].m_upperCut;
  if (myLimit < NO_SEL_CUT) {
    if (r > myLimit) return false;
  }

  int vecSize=m_quantities[index].m_listValN.size();
  if (vecSize > 0) {
    for (int i=0; i<vecSize; i++) {
      myLimit=m_quantities[index].m_listValN[i];
      if (fabs(myLimit) > NearZero) {
        if (fabs(r/myLimit-1.0) <= precis) return !miss;
      }
      else if (fabs(r) <= precis) return !miss;
      // returns TRUE for inclusion (equality found)
      // returns FALSE for exclusion (difference found)
    }
    return miss;
    // not in list: returns FALSE for inclusion, TRUE for exclusion
  }

  // empty list (interval exist): within interval
  return true;
}

/**********************************************************************/
// check if value pass criteria for given quantity index (cut OR list)
// private method called by includeN
bool Catalog::checkNUMor(const double r, const int index,
                         const bool reject, const double precis) {

  // since we test the NaN only once at the beginning
  // this methods MUST NOT be called if no criteria is applied
  if (isnan(r)) return !reject;

  bool check=true;
  double myLimit;
  int vecSize=m_quantities[index].m_listValN.size();
  if (vecSize > 0) {
    int i;
    check=false;
    for (i=0; i<vecSize; i++) {
      myLimit=m_quantities[index].m_listValN[i];
      if (fabs(myLimit) > NearZero) {
        if (fabs(r/myLimit-1.0) <= precis) return true;
      }
      else if (fabs(r) <= precis) return true;
      // returns TRUE for inclusion (equality found)
      // needn't check for upper/lower cut (OR condition)
    }
  }

  myLimit=m_quantities[index].m_lowerCut;
  if (myLimit < NO_SEL_CUT) {
    check=true; // used with list
    if (r < myLimit) return false;
  }

  myLimit=m_quantities[index].m_upperCut;
  if (myLimit < NO_SEL_CUT) {
    check=true; // used with list
    if (r > myLimit) return false;
  }

  // empty list  + within interval = TRUE
  // not in list + within interval = TRUE
  // not in list +   no  interval = FALSE
  return check;

}

/**********************************************************************/
// compute the global bit selection at existing row,
// suppose that m_rowIsSelected is correctly set (private method)
bool Catalog::rowSelect(const long row, const std::vector<bool> &quantSel) {

  int  numBit=sizeof(long)*8,
       vecSize=quantSel.size();
  bool check=true;
  if (m_criteriaORed) check=false;
  unsigned long currSel, test=2ul; // bit0 reserved for check
  #ifdef DEBUG_CAT
  std::cout << m_criteriaORed << "quantSize = " << vecSize;
  #endif
  int j=0;
  for (int i=1; i<=vecSize; ) {

    currSel=m_rowIsSelected[j].at(row);
    if (quantSel[i-1]) {
      #ifdef DEBUG_CAT
      std::cout << " (AND with " << currSel <<" & "<< test
                << ", pos[" << j <<"] )" << std::endl;
      #endif
      if (m_criteriaORed) {
        // OR  of bits, true --> useless to continue
        if ((currSel & test) == test) {check=true; break;}
      }
      else {
        // AND of bits, false--> useless to continue
        if ((currSel & test) == 0ul) {check=false; break;}
      }
    }
    if ((++i % numBit) == 0) {
      test=1ul; // first bit
      j++;      // of next vector index.
    }
    else test*=2ul;

  }
  if (check) // setting bit0 to 1
    m_rowIsSelected[0].at(row)|= 1ul;
  else       // setting bit0 to 0
    m_rowIsSelected[0].at(row)&= (Max_Test-1ul);

  return check;
}


/**********************************************************************/
// unset cut on quantity found by its existing index (private method)
void Catalog::unsetCuts(const int index) {

  m_quantities.at(index).m_listValS.clear();
  m_quantities.at(index).m_lowerCut=NO_SEL_CUT;
  m_quantities.at(index).m_upperCut=NO_SEL_CUT;
  m_quantities.at(index).m_listValN.clear();
  /* if no data: exit */
  if (m_numRows == 0) return;

  // now, must check criteria on region and other quantities
  int k;
  std::vector<bool> isSelected;
  bool oneCriteria=existCriteria(&isSelected);
  if (oneCriteria) {

    unsigned long test=bitPosition(index, &k);
    m_numSelRows=0;
    for (long i=0; i<m_numRows; i++) {
      // setting required bit to 0
      m_rowIsSelected[k].at(i)&= (Max_Test-test);
      if (rowSelect(i, isSelected) == true) m_numSelRows++;
    }// loop on rows

  }
  else {
    if (m_numSelRows > 0) {
      int quantSize=m_rowIsSelected.size(); // to avoid compiler Warning
      for (k=0; k<quantSize; k++)
        m_rowIsSelected[k].assign(m_numRows, 0);
    }
    m_numSelRows=0;
    printLog(0, "All rows unselected");
  }

}
/**********************************************************************/
// unset all selection criteria relating to quantity "name"
int Catalog::unsetCuts(const std::string name) {

  int quantSize=checkImport("unsetCuts", true);
  if (quantSize < IS_VOID) return quantSize;
  int index=checkQuant_name("unsetCuts", name);
  if (index < 0) return index;

  unsetCuts(index);
  return IS_OK;
}

/**********************************************************************/
// unset all cuts on all quantities except the selection ellipse;
// this also deletes the selection string
int Catalog::unsetCuts() {

  int quantSize=checkImport("unsetCuts", true);
  if (quantSize < IS_VOID) return quantSize;
  int j;
  /*for (j=0; j<quantSize; j++) unsetCuts(m_quantities.at(j).m_name);*/
  //quicker to just check for selection ellipse
  for (j=0; j<quantSize; j++) {
    m_quantities.at(j).m_listValS.clear();
    m_quantities.at(j).m_lowerCut=NO_SEL_CUT;
    m_quantities.at(j).m_upperCut=NO_SEL_CUT;
    m_quantities.at(j).m_listValN.clear();
  }
  m_selection="";
  /* if no data: exit */
  if (m_numRows == 0) return IS_OK;

  // now, must check criteria on region
  quantSize=m_rowIsSelected.size();
  if (m_selRegion) {

    unsigned long currSel;
    m_numSelRows=0;
    for (long i=0; i<m_numRows; i++) {
      currSel=m_rowIsSelected[0].at(i);
      if (currSel & 2ul) {
        m_rowIsSelected[0].at(i)=3ul;
        m_numSelRows++;
      }
      else m_rowIsSelected[0].at(i)=0ul;
      // setting all bits to 0 except first two
      for (j=1; j<quantSize; j++) m_rowIsSelected[j].at(i)=0ul;
    }// loop on rows

  }
  else {
    if (m_numSelRows > 0)
      for (j=0; j<quantSize; j++) m_rowIsSelected[j].assign(m_numRows, 0);
    m_numSelRows=0;
    printLog(0, "All rows unselected");
  }
  return IS_OK;
}

/**********************************************************************/
// if true, criteria between quantities are ORed instead of ANDed
int Catalog::setCriteriaORed(const bool bitOR) {

  const std::string origin="setCriteriaORed";
  int quantSize=checkImport(origin, true);
  if (quantSize < IS_VOID) return quantSize;

  // if boolean is the same: do nothing
  if (bitOR == m_criteriaORed) return IS_OK;
  m_criteriaORed=bitOR;

  /* if no data: exit */
  if (m_numRows == 0) return IS_OK;
  m_numSelRows=0;
  std::vector<bool> isSelected;
  existCriteria(&isSelected);
  for (long i=0; i<m_numRows; i++) {
    if (rowSelect(i, isSelected) == true) m_numSelRows++;
  }
  return IS_OK;
}
      

/**********************************************************************/
// set and apply an elliptical selection region
int Catalog::setSelEllipse(const double centRA_deg, const double centDEC_deg,
                           const double majAxis_deg, const double minAxis_deg,
                           const double rot_deg) {

  const std::string origin="setSelEllipse";
  std::string text;
  std::ostringstream sortie;

  // first check that selection is possible
  int numPb=checkImport(origin, true);
  if (numPb < IS_VOID) return numPb;
  if ((m_indexRA < 0) || (m_indexDEC < 0)) {
    text="missing generic position quantities (RA and DEC)"; 
    printWarn(origin, text);
    return NO_RA_DEC;
  }
  numPb=0;
  //const std::_Ios_Fmtflags outDouble=std::ios::right | std::ios::scientific;
  if ((centRA_deg < 0.) || (centRA_deg >= 360.)) numPb=BAD_RA;
  else if ((centDEC_deg < -90.) || (centDEC_deg > 90.)) numPb=BAD_DEC;
  else if ((rot_deg < 0.) || (rot_deg >= 180.)) numPb=BAD_ROT;
  if (numPb < 0) {
    text="bad ellipse position (impossible RA, DEC or rotation)"; 
    printWarn(origin, text);
    return numPb;
  }
  if ((majAxis_deg < Min_Axis) || (majAxis_deg > 90.)) numPb=BAD_AXIS;
  else if ((minAxis_deg < Min_Axis) || (minAxis_deg > 90.)) numPb=BAD_AXIS;
  // size limited to one hemisphere
  if (numPb < 0) {
    sortie << "bad ellipse size, radius from " << std::setprecision(2)
           << Min_Axis*1000 << "E-3 to 90 (in RA or DEC)";
    printWarn(origin, sortie.str());
    return numPb;
  }
  if (rot_deg > 0.)
    printWarn(origin, "whatever orientation, using 0"); 
  if (fabs(majAxis_deg/minAxis_deg - 1.) > 10*Min_Prec)
    printWarn(origin, "axis sizes differ, taking only major axis");
  m_selRegion=true;
  m_selEllipseCentRA_deg=centRA_deg;
  m_selEllipseCentDEC_deg=centDEC_deg;
  m_selEllipseMajAxis_deg=majAxis_deg;
  m_selEllipseMinAxis_deg=majAxis_deg; //minAxis_deg;
  m_selEllipseRot_deg=0.; //rot_deg;
  // angles are phi=RA and theta=PI/2-DEC
  m_selEllipse.at(0)=cos(m_selEllipseCentRA_deg * Angle_Conv);
  m_selEllipse.at(1)=sin(m_selEllipseCentRA_deg * Angle_Conv);
  m_selEllipse.at(2)=cos(m_selEllipseCentDEC_deg* Angle_Conv);
  m_selEllipse.at(3)=sin(m_selEllipseCentDEC_deg* Angle_Conv);
  m_selEllipse.at(4)=cos(majAxis_deg * Angle_Conv);

  sortie << "selection ellipse center RA=" << std::setprecision(4)
         << m_selEllipseCentRA_deg << " , DEC="
         << m_selEllipseCentDEC_deg << " with radius "
         << m_selEllipseMajAxis_deg << " * "
         << m_selEllipseMinAxis_deg << " (degrees) orientated at "
         << m_selEllipseRot_deg << " (with respect to North pole)";
  printLog(1, sortie.str());
  /* if no data: exit */
  if (m_numRows == 0) return IS_OK;

  // now, apply the selection ellipse
  std::vector<bool> isSelected;
  existCriteria(&isSelected); // returns true (m_selRegion is true)
  bool check;
  int nRA =m_quantities[m_indexRA].m_index,
      nDEC=m_quantities[m_indexDEC].m_index;
  unsigned long currSel;
  m_numSelRows=0;
  for (long i=0; i<m_numRows; i++) {

    check=checkRegion(i, nRA, nDEC);
    currSel=m_rowIsSelected[0].at(i);
    if (check) // setting 2nd bit to 1
      m_rowIsSelected[0].at(i)=currSel | 2ul;
    else       // setting 2nd bit to 0
      m_rowIsSelected[0].at(i)=currSel & (Max_Test-2ul);
    if (rowSelect(i, isSelected) == true) m_numSelRows++;

  }// loop on all rows
  return IS_OK;
}
/**********************************************************************/
// remove the effects of the ellipse selection
int Catalog::unsetSelEllipse() {

  int quantSize=checkImport("unsetSelEllipse", true);
  if (quantSize < IS_VOID) return quantSize;

  // if region was already unset: do nothing
  if (!m_selRegion) return IS_OK;
  m_selRegion=false;
  /* if no data: exit */
  if (m_numRows == 0) return IS_OK;

  // now, must check criteria on other quantities
  std::vector<bool> isSelected;
  bool oneCriteria=existCriteria(&isSelected);
  if (oneCriteria) {
    m_numSelRows=0;
    for (long i=0; i<m_numRows; i++) {

      // setting 2nd bit to 0
      m_rowIsSelected[0].at(i)&= (Max_Test-2ul);
      if (rowSelect(i, isSelected) == true) m_numSelRows++;
 
    }// loop on rows
  }
  else {
    if (m_numSelRows > 0) {
      quantSize=m_rowIsSelected.size();
      for (int j=0; j<quantSize; j++) m_rowIsSelected[j].assign(m_numRows, 0);
    }
    m_numSelRows=0;
    printLog(0, "All rows unselected");
  }
  return IS_OK;
}


/**********************************************************************/
// set and apply a cut on given quantity such that all values >= cutVal pass
// double is not const because it can be locally modified to NO_SEL_CUT
int Catalog::setLowerCut(const std::string name, double cutVal) {

  const std::string origin="setLowerCut";
  int quantSize=checkImport(origin, true);
  if (quantSize < IS_VOID) return quantSize;

  int index=checkQuant_name(origin, name);
  if (index < 0) return index;
  if (m_quantities.at(index).m_type != Quantity::NUM) {  
    std::string errText;
    errText="given Quantity name ("+name+") is not of NUM type";
    printWarn(origin, errText);
    return BAD_QUANT_TYPE;
  }

  // if cut is the same: do nothing
  if (cutVal >= NO_SEL_CUT) cutVal=NO_SEL_CUT;
  if (cutVal == m_quantities.at(index).m_lowerCut) return IS_OK;
  m_quantities.at(index).m_lowerCut=cutVal;

  /* if no data: exit */
  if (m_numRows == 0) return IS_OK;

  std::vector<bool> isSelected;
  bool oneCriteria=existCriteria(&isSelected);
  bool check=true;
  if (cutVal >= NO_SEL_CUT) {
    printLog(1, "Disabling lower cut (on "+name+")");
    if (!oneCriteria) {
      if (m_numSelRows > 0) {
        quantSize=m_rowIsSelected.size();
        for (index=0; index<quantSize; index++)
          m_rowIsSelected[index].assign(m_numRows, 0);
      }
      m_numSelRows=0;
      check=false;
      printLog(0, "All rows unselected");
    }
  }
  else {
    std::ostringstream sortie;
    sortie << "Enabling lower cut (" <<cutVal<<" on "<<name<< ")";
    printLog(1, sortie.str());  
  }
  
  if (check) {
    bool reject=m_quantities[index].m_rejectNaN,
         cutOR=m_quantities[index].m_cutORed,
         miss=m_quantities[index].m_excludeList;
    double precis=m_quantities[index].m_precision;
    int  k,
         pos=m_quantities[index].m_index;
    long i;
    unsigned long test=bitPosition(index, &k);
    m_numSelRows=0;
    // true if given quantity is selected (list or lower/upper cut)
    oneCriteria=isSelected.at(index+1);
    if (oneCriteria) {

      // due to NaN test, call checkNUM only if selection exists
      for (i=0; i<m_numRows; i++) {
        if (!cutOR) // usual case
          check=checkNUM(m_numericals[pos].at(i), index, miss, reject, precis);
        else
          check=checkNUMor(m_numericals[pos].at(i), index, reject, precis);
        // setting required bit
        if (check)
          m_rowIsSelected[k].at(i)|= test;
        else
          m_rowIsSelected[k].at(i)&= (Max_Test-test);
        if (rowSelect(i, isSelected) == true) m_numSelRows++;
      }// loop on rows

    }
    else {

      for (i=0; i<m_numRows; i++) {
        // required bit set to FALSE
        m_rowIsSelected[k].at(i)&= (Max_Test-test);
        if (rowSelect(i, isSelected) == true) m_numSelRows++;
      }// loop on rows

    }
  }
  return IS_OK;
}
/**********************************************************************/
// set and apply a cut on given quantity such that all values <= cutVal pass
// double is not const because it can be locally modified to NO_SEL_CUT
int Catalog::setUpperCut(const std::string name, double cutVal) {

  const std::string origin="setUpperCut";
  int quantSize=checkImport(origin, true);
  if (quantSize < IS_VOID) return quantSize;

  int index=checkQuant_name(origin, name);
  if (index < 0) return index;
  if (m_quantities.at(index).m_type != Quantity::NUM) {  
    std::string errText;
    errText="given Quantity name ("+name+") is not of NUM type";
    printWarn(origin, errText);
    return BAD_QUANT_TYPE;
  }
  // if cut is the same: do nothing
  if (cutVal >= NO_SEL_CUT) cutVal=NO_SEL_CUT;
  if (cutVal == m_quantities.at(index).m_upperCut) return IS_OK;
  m_quantities.at(index).m_upperCut=cutVal;

  /* if no data: exit */
  if (m_numRows == 0) return IS_OK;

  std::vector<bool> isSelected;
  bool oneCriteria=existCriteria(&isSelected);
  bool check=true;
  if (cutVal >= NO_SEL_CUT) {
    printLog(1, "Disabling upper cut (on "+name+")");
    if (!oneCriteria) {
      if (m_numSelRows > 0) {
        quantSize=m_rowIsSelected.size();
        for (index=0; index<quantSize; index++)
          m_rowIsSelected[index].assign(m_numRows, 0);
      }
      m_numSelRows=0;
      check=false;
      printLog(0, "All rows unselected");
    }
  }
  else {
    std::ostringstream sortie;
    sortie << "Enabling upper cut (" <<cutVal<<" on "<<name<< ")";
    printLog(1, sortie.str());  
  }
  
  if (check) {
    bool reject=m_quantities[index].m_rejectNaN,
         cutOR=m_quantities[index].m_cutORed,
         miss=m_quantities[index].m_excludeList;
    double precis=m_quantities[index].m_precision;
    int  k,
         pos=m_quantities[index].m_index;
    long i;
    unsigned long test=bitPosition(index, &k);
    m_numSelRows=0;
    // true if given quantity is selected (list or lower/upper cut)
    oneCriteria=isSelected.at(index+1);
    if (oneCriteria) {

      // due to NaN test, call checkNUM only if selection exists
      for (i=0; i<m_numRows; i++) {
        if (!cutOR) // usual case
          check=checkNUM(m_numericals[pos].at(i), index, miss, reject, precis);
        else
          check=checkNUMor(m_numericals[pos].at(i), index, reject, precis);
        // setting required bit
        if (check)
          m_rowIsSelected[k].at(i)|= test;
        else
          m_rowIsSelected[k].at(i)&= (Max_Test-test);
        if (rowSelect(i, isSelected) == true) m_numSelRows++;
      }// loop on rows

    }
    else {

      for (i=0; i<m_numRows; i++) {
        // required bit set to FALSE
        m_rowIsSelected[k].at(i)&= (Max_Test-test);
        if (rowSelect(i, isSelected) == true) m_numSelRows++;
      }// loop on rows

    }
  }
  return IS_OK;
}

/**********************************************************************/
// set and apply a cut on quantities in VECTOR type quantity "name"
// such that all values >= cutValues[i] pass
 
/**********************************************************************/
// set and apply a cut on quantities in VECTOR type quantity "name"
// such that all values <= cutValues[i] pass
 

/**********************************************************************/
// select rows depending on the given numerical list for existing index
int Catalog::doSelN(const std::string name, const int index, const int code,
                    const std::vector<double> &listVal) {

  std::ostringstream sortie;
  m_quantities[index].m_cutORed=false;
  bool miss;
  // take only first 7 char to match possible generic functions
  if (code & 1) {
    m_quantities[index].m_excludeList=false;
    miss=false;
    sortie << "Include rows";
  }
  else {
    m_quantities[index].m_excludeList=true;
    miss=true;
    sortie << "Exclude rows";
  }
  int listSize=listVal.size();
  /* if empty list as before: exit */
  if (listSize == 0) {
    // if NO list, m_cutORed has no effect (false method quicker)
    if (m_quantities[index].m_listValN.size() == 0) return IS_OK;
  }
  else if (code > 1)
    m_quantities[index].m_cutORed=true;

  int j;
  m_quantities[index].m_listValN.clear();
  for (j=0; j<listSize; j++)
     m_quantities[index].m_listValN.push_back(listVal.at(j));

  /* if no data: exit */
  if (m_numRows == 0) return IS_OK;

  std::vector<bool> isSelected;
  bool oneCriteria=existCriteria(&isSelected);
  bool check=true;
  if (listSize == 0) {
    printLog(1, "Disabling list selection (on "+name+")");
    if (!oneCriteria) {
      if (m_numSelRows > 0) {
        listSize=m_rowIsSelected.size(); // to avoid compiler Warning
        for (j=0; j<listSize; j++)
          m_rowIsSelected[j].assign(m_numRows, 0);
      }
      m_numSelRows=0;
      check=false;
      printLog(0, "All rows unselected");
    }
  }
  else {
    sortie << " with \"" << name <<"\" around value in list ("
           << listSize << " elements, ";
    if (m_quantities[index].m_cutORed)
      sortie << "ORed with cut)";
    else
      sortie << "ANDed with cut)";
    printLog(1, sortie.str());
  }
  
  if (check) {
    bool reject=m_quantities[index].m_rejectNaN,
         cutOR=m_quantities[index].m_cutORed;
    double precis=m_quantities[index].m_precision;
    int  k,
         pos=m_quantities[index].m_index;
    long i;
    unsigned long test=bitPosition(index, &k);
    m_numSelRows=0;
    // true if given quantity is selected (list or lower/upper cut)
    oneCriteria=isSelected.at(index+1);
    if (oneCriteria) {

      // due to NaN test, call checkNUM only if selection exists
      for (i=0; i<m_numRows; i++) {
        if (!cutOR) // usual case
          check=checkNUM(m_numericals[pos].at(i), index, miss, reject, precis);
        else
          check=checkNUMor(m_numericals[pos].at(i), index, reject, precis);
        // setting required bit
        if (check)
          m_rowIsSelected[k].at(i)|= test;
        else
          m_rowIsSelected[k].at(i)&= (Max_Test-test);
        if (rowSelect(i, isSelected) == true) m_numSelRows++;
      }// loop on rows

    }
    else {

      for (i=0; i<m_numRows; i++) {
        // required bit set to FALSE
        m_rowIsSelected[k].at(i)&= (Max_Test-test);
        if (rowSelect(i, isSelected) == true) m_numSelRows++;
      }// loop on rows

    }
  }
  return IS_OK;  
}
/**********************************************************************/
// include rows which value is NOT in the given list AND value within cut
int Catalog::excludeN(const std::string name,
                      const std::vector<double> &listVal) {

  int done=checkImport("excludeN", true);
  if (done < IS_VOID) return done;
  int index=checkQuant_name("excludeN", name);
  if (index < 0) return index;

  if (m_quantities.at(index).m_type != Quantity::NUM) {  
    std::string errText;
    errText="given Quantity name ("+name+") is not of NUM type";
    printWarn("excludeN", errText);
    return BAD_QUANT_TYPE;
  }
  done=doSelN(name, index, 0, listVal);
  return done;
}
/**********************************************************************/
// only include rows which have numerical value in the given list AND cut
int Catalog::useOnlyN(const std::string name,
                      const std::vector<double> &listVal) {

  int done=checkImport("useOnlyN", true);
  if (done < IS_VOID) return done;
  int index=checkQuant_name("useOnlyN", name);
  if (index < 0) return index;

  if (m_quantities.at(index).m_type != Quantity::NUM) {  
    std::string errText;
    errText="given Quantity name ("+name+") is not of NUM type";
    printWarn("useOnlyN", errText);
    return BAD_QUANT_TYPE;
  }
  done=doSelN(name, index, 1, listVal);
  return done;
}
/**********************************************************************/
// only include rows which have numerical value in the given list OR cut
int Catalog::includeN(const std::string name,
                      const std::vector<double> &listVal) {

  int done=checkImport("includeN", true);
  if (done < IS_VOID) return done;
  int index=checkQuant_name("includeN", name);
  if (index < 0) return index;

  if (m_quantities.at(index).m_type != Quantity::NUM) {  
    std::string errText;
    errText="given Quantity name ("+name+") is not of NUM type";
    printWarn("includeN", errText);
    return BAD_QUANT_TYPE;
  }
  done=doSelN(name, index, 3, listVal); // 3 to set bit 0 to 1.
  return done;
}

/**********************************************************************/
// set quantity member and apply (if different and quantity is selected)
int Catalog::setRejectNaN(const std::string name, const bool rejectNaN) {

  const std::string origin="setRejectNaN";
  int quantSize=checkImport(origin, true);
  if (quantSize < IS_VOID) return quantSize;
  int index=checkQuant_name(origin, name);
  if (index < 0) return index;
  if (m_quantities.at(index).m_type != Quantity::NUM) {  
    std::string errText;
    errText="given Quantity name ("+name+") is not of NUM type";
    printWarn(origin, errText);
    return BAD_QUANT_TYPE;
  }
  // if same boolean: do nothing
  if (rejectNaN == m_quantities.at(index).m_rejectNaN) return IS_OK;
  m_quantities.at(index).m_rejectNaN=rejectNaN;

  /* if no data: exit */
  if (m_numRows == 0) return IS_OK;

  std::vector<bool> isSelected;
  existCriteria(&isSelected);
  if (isSelected.at(index+1)) {
    // apply the selection with new behaviour on NaN

    std::string text="NaN values are now ";
    if (rejectNaN) text=text+" rejected (on ";
    else text=text+" accepted (on ";
    text=text+name+ ")";
    printLog(1, text);
    bool check,
         cutOR=m_quantities[index].m_cutORed,
         miss=m_quantities[index].m_excludeList;
    double precis=m_quantities[index].m_precision;
    int  k,
         pos=m_quantities[index].m_index;
    unsigned long test=bitPosition(index, &k);
    m_numSelRows=0;
    for (long i=0; i<m_numRows; i++) {

      if (!cutOR) // usual case
        check=checkNUM(m_numericals[pos].at(i), index, miss, rejectNaN, precis);
      else
        check=checkNUMor(m_numericals[pos].at(i), index, rejectNaN, precis);
      if (check)
        m_rowIsSelected[k].at(i)|= test;
      else
        m_rowIsSelected[k].at(i)&= (Max_Test-test);
      if (rowSelect(i, isSelected) == true) m_numSelRows++;

    }// loop on rows

  }
  return IS_OK;  
}
/**********************************************************************/
// set quantity member and apply (if quantity has a non empty selection list)
int Catalog::setMatchPercent(const std::string name, double percent) {

  const std::string origin="setMatchPercent";
  int quantSize=checkImport(origin, true);
  if (quantSize < IS_VOID) return quantSize;
  int index=checkQuant_name(origin, name);
  if (index < 0) return index;
  if (m_quantities.at(index).m_type != Quantity::NUM) {  
    std::string errText;
    errText="given Quantity name ("+name+") is not of NUM type";
    printWarn(origin, errText);
    return BAD_QUANT_TYPE;
  }
  std::ostringstream sortie;
  if (percent <= 100.*NearZero) {
    sortie << "Percentage must be > " << 100.*NearZero;
    printWarn(origin, sortie.str());
    return BAD_SEL_LIM;
  }
  // if limit is the same: do nothing
  percent/=100.;
  if (percent == m_quantities[index].m_precision) return IS_OK;
  m_quantities.at(index).m_precision=percent;

  /* if no data: exit */
  if (m_numRows == 0) return IS_OK;
  /* if empty list: exit */
  if (m_quantities[index].m_listValN.size() == 0) return IS_OK;

  std::vector<bool> isSelected;
  existCriteria(&isSelected);
  // quantity is selected since list is not empty
  sortie << "Relative precision for values in list (for "<<name<< ")"
         << " is: " << percent << " (absolute precision around 0)";
  printLog(1, sortie.str());
  bool check,
       cutOR=m_quantities[index].m_cutORed,
       reject=m_quantities[index].m_rejectNaN,
       miss=m_quantities[index].m_excludeList;
  int  k,
       pos=m_quantities[index].m_index;
  unsigned long test=bitPosition(index, &k);
  m_numSelRows=0;
  for (long i=0; i<m_numRows; i++) {

    if (!cutOR) // usual case
      check=checkNUM(m_numericals[pos].at(i), index, miss, reject, percent);
    else
      check=checkNUMor(m_numericals[pos].at(i), index, reject, percent);
    if (check)
      m_rowIsSelected[k].at(i)|= test;
    else
      m_rowIsSelected[k].at(i)&= (Max_Test-test);
    if (rowSelect(i, isSelected) == true) m_numSelRows++;

  }// loop on rows

  return IS_OK;  
}
/**********************************************************************/
// set quantity member and apply (if quantity has a non empty selection list)
int Catalog::setMatchEpsilon(const std::string name, const unsigned long step)
{
  const std::string origin="setMatchEpsilon";
  int quantSize=checkImport(origin, true);
  if (quantSize < IS_VOID) return quantSize;
  int index=checkQuant_name(origin, name);
  if (index < 0) return index;
  if (m_quantities.at(index).m_type != Quantity::NUM) {  
    std::string errText;
    errText="given Quantity name ("+name+") is not of NUM type";
    printWarn(origin, errText);
    return BAD_QUANT_TYPE;
  }
  std::ostringstream sortie;
  if (step == 0ul) {
    sortie << "Number of epsilon must be > 0";
    printWarn(origin, sortie.str());
    return BAD_SEL_LIM;
  }
  // if limit is the same: do nothing
  double precis=step*std::numeric_limits<double>::epsilon();
  if (precis == m_quantities[index].m_precision) return IS_OK;
  m_quantities.at(index).m_precision=precis;

  /* if no data: exit */
  if (m_numRows == 0) return IS_OK;
  /* if empty list: exit */
  if (m_quantities[index].m_listValN.size() == 0) return IS_OK;

  std::vector<bool> isSelected;
  existCriteria(&isSelected);
  // quantity is selected since list is not empty
  sortie << "Relative precision for values in list (for "<<name<< ")"
         << " is: " << precis << " (absolute precision around 0)";
  printLog(1, sortie.str());
  bool check,
       cutOR=m_quantities[index].m_cutORed,
       reject=m_quantities[index].m_rejectNaN,
       miss=m_quantities[index].m_excludeList;
  int  k,
       pos=m_quantities[index].m_index;
  unsigned long test=bitPosition(index, &k);
  m_numSelRows=0;
  for (long i=0; i<m_numRows; i++) {

    if (!cutOR) // usual case
      check=checkNUM(m_numericals[pos].at(i), index, miss, reject, precis);
    else
      check=checkNUMor(m_numericals[pos].at(i), index, reject, precis);
    if (check)
      m_rowIsSelected[k].at(i)|= test;
    else
      m_rowIsSelected[k].at(i)&= (Max_Test-test);
    if (rowSelect(i, isSelected) == true) m_numSelRows++;

  }// loop on rows

  return IS_OK;  
}

/**********************************************************************/
// check if given value is within interval cut
int Catalog::checkValWithinCut(const std::string name, const double value,
                               bool *cutLow, bool *cutUp,int *checkResult) {

  *cutLow=false;
  *cutUp=false;
  *checkResult=0;
  const std::string origin="checkValWithinCut";
  int quantSize=checkImport(origin, true);
  if (quantSize < IS_VOID) return quantSize;
  int index=checkQuant_name(origin, name);
  if (index < 0) return index;
  if (m_quantities.at(index).m_type != Quantity::NUM) {  
    std::string errText;
    errText="given Quantity name ("+name+") is not of NUM type";
    printWarn(origin, errText);
    return BAD_QUANT_TYPE;
  }  

  double myLimit, r=value;
  myLimit=m_quantities[index].m_lowerCut;
  if (fabs(value) > NearZero)
    r+=value*m_quantities[index].m_precision;
  else
    r+=m_quantities[index].m_precision;
  if (myLimit < NO_SEL_CUT) {
    *cutLow=true;
    if (r < myLimit) *checkResult=-2;
    else if (value < myLimit) *checkResult=-1;
  }
  r=value;
  myLimit=m_quantities[index].m_upperCut;
  if (fabs(value) > NearZero)
    r-=value*m_quantities[index].m_precision;
  else
    r-=m_quantities[index].m_precision;
  if (myLimit < NO_SEL_CUT) {
    *cutUp=true;
    if (r > myLimit) *checkResult=2;
    else if (value > myLimit) *checkResult=1;
  }

  return IS_OK;  
}


/**********************************************************************/
// select rows depending on the given string list for existing index
int Catalog::doSelS(const std::string name, const int index, const int code,
                    const std::vector<std::string> &list, const bool exact) {

  std::ostringstream sortie;
  bool miss;
  if (code & 1) {
    m_quantities[index].m_excludeList=false; // set but unused for string
    miss=false;
    sortie << "Include rows";
  }
  else {
    m_quantities[index].m_excludeList=true; // set but unused for string
    miss=true;
    sortie << "Exclude rows";
  }
  int listSize=list.size();
  if ((!listSize) && (!m_quantities[index].m_listValS.size())) return IS_OK;
  m_quantities[index].m_listValS.clear();
  int j;
  int (*pfunc)(int)=tolower; // function used by transform
  std::string mot;
  if (exact) {
    for (j=0; j<listSize; j++)
      m_quantities[index].m_listValS.push_back(list.at(j));
  }
  else {
    for (j=0; j<listSize; j++) {
      mot=list.at(j);
      transform(mot.begin(), mot.end(), mot.begin(), pfunc);
      m_quantities[index].m_listValS.push_back(mot);
    }
  }
  #ifdef DEBUG_CAT
  for (j=0; j<listSize; j++)
    std::cout << m_quantities[index].m_listValS[j] << "|";
  std::cout <<"caseless="<< exact << std::endl;
  #endif

  /* if no data: exit */
  if (m_numRows == 0) return IS_OK;

  std::vector<bool> isSelected;
  bool oneCriteria=existCriteria(&isSelected);
  bool check=true;
  if (listSize == 0) {
    printLog(1, "Disabling list selection (on "+name+")");
    if (!oneCriteria) {
      if (m_numSelRows > 0) {
        listSize=m_rowIsSelected.size(); // to avoid compiler Warning
        for (j=0; j<listSize; j++)
          m_rowIsSelected[j].assign(m_numRows, 0);
      }
      m_numSelRows=0;
      check=false;
      printLog(0, "All rows unselected");
    }
  }
  else {
    sortie << " with \"" << name <<"\" string in list ("
           << listSize << " elements, ";
    if (exact) sortie << "exact match)"; else sortie << "caseless match)"; 
    printLog(1, sortie.str());
  }
  
  if (check) {
    int k, pos=m_quantities[index].m_index;
    unsigned long test=bitPosition(index, &k);
    m_numSelRows=0;
    for (long i=0; i<m_numRows; i++) {

      mot=m_strings[pos].at(i);
      if (!exact) transform(mot.begin(), mot.end(), mot.begin(), pfunc);
      check=miss;
      for (j=0; j<listSize; j++) {
         if (mot == m_quantities[index].m_listValS[j]) {check=!miss; break;}
      }
      // setting required bit
      if (check)
        m_rowIsSelected[k].at(i)|= test;
      else
        m_rowIsSelected[k].at(i)&= (Max_Test-test);
      if (rowSelect(i, isSelected) == true) m_numSelRows++;
 
    }// loop on rows
  }

  return IS_OK;
}
/**********************************************************************/
// exclude all rows which have string value in the given list
int Catalog::excludeS(const std::string name,
                      const std::vector<std::string> &list, const bool exact) {

  int done=checkImport("excludeS", true);
  if (done < IS_VOID) return done;
  int index=checkQuant_name("excludeS", name);
  if (index < 0) return index;

  if (m_quantities.at(index).m_type != Quantity::STRING) {  
    std::string errText;
    errText="given Quantity name ("+name+") is not of STRING type";
    printWarn("excludeS", errText);
    return BAD_QUANT_TYPE;
  }
  done=doSelS(name, index, 0, list, exact);
  return done;
}
/**********************************************************************/
// only include rows which have string value in the given list
int Catalog::useOnlyS(const std::string name,
                      const std::vector<std::string> &list, const bool exact) {

  int done=checkImport("useOnlyS", true);
  if (done < IS_VOID) return done;
  int index=checkQuant_name("useOnlyS", name);
  if (index < 0) return index;

  if (m_quantities.at(index).m_type != Quantity::STRING) {  
    std::string errText;
    errText="given Quantity name ("+name+") is not of STRING type";
    printWarn("useOnlyS", errText);
    return BAD_QUANT_TYPE;
  }
  done=doSelS(name, index, 1, list, exact);
  return done;
}


/**********************************************************************/
// erase all non-selected rows from memory
int Catalog::eraseNonSelected() {

  const std::string origin="eraseNonSelected";
  if (m_numRows <= 0) {
    printWarn(origin, "catalog is empty");
    return IS_VOID; 
  }
  // test below avoid deleting NOTHING
  if (m_numRows == m_numSelRows) {
    printWarn(origin, "all rows selected, nothing done");
    return IS_OK; 
  }
  // log if ALL is deleted
  std::string text;
  if (m_numSelRows == 0) {
    text="no row selected, calling deleteContent()";
    printLog(2, text);
    deleteContent();
    return IS_OK;
  }
  try {
    long i, tot=0;
    int j;
    int sizeS=m_strings.size();
    int sizeN=m_numericals.size();
    int vecSize=m_rowIsSelected.size();
    // to speed-up, will not change (avoid reading size in loop)

    std::vector<std::vector<double> > myNum;
    myNum.resize(sizeN);
    for (j=0; j<sizeN; j++) myNum[j].assign(m_numSelRows, 0.0);
    std::vector<std::vector<std::string> > myStr;
    myStr.resize(sizeS);
    for (j=0; j<sizeS; j++) myStr[j].assign(m_numSelRows, "");
    std::vector<std::vector<unsigned long> > myBits;
    myBits.resize(vecSize);
    for (j=0; j<vecSize; j++) myBits[j].assign(m_numSelRows, 0ul);
    for (i=0; i<m_numRows; i++) {
      if (m_rowIsSelected[0].at(i) & 1) {
        for (j=0; j<sizeN; j++) myNum[j].at(tot)=m_numericals[j].at(i);
        for (j=0; j<sizeS; j++) myStr[j].at(tot)=m_strings[j].at(i);
        for (j=0; j<vecSize; j++) myBits[j].at(tot)=m_rowIsSelected[j].at(i);
        if (++tot == m_numSelRows) break; // to speed up
      }
    }
    for (j=0; j<sizeN; j++) {
      m_numericals[j].insert(m_numericals[j].begin(),
                             myNum[j].begin(), myNum[j].end());
      myNum[j].clear();
    }
    myNum.clear();
    for (j=0; j<sizeS; j++) {
      m_strings[j].insert(m_strings[j].begin(),
                          myStr[j].begin(), myStr[j].end());
      myStr[j].clear();
    }
    myStr.clear();
    for (j=0; j<vecSize; j++)  {
      m_rowIsSelected[j].insert(m_rowIsSelected[j].begin(),
                                myBits[j].begin(), myBits[j].end());
      myBits[j].clear();
    }
    myBits.clear();
/* if (!keepCriteria) {
    for (j=0; j<vecSize; j++) m_rowIsSelected[j].assign(m_numSelRows, 0);
    vecSize=m_quantities.size();
    for (i=0; i<vecSize; i++) {
      m_quantities.at(i).m_listValS.clear();
      m_quantities.at(i).m_lowerCut=NO_SEL_CUT;
      m_quantities.at(i).m_upperCut=NO_SEL_CUT;
      m_quantities.at(i).m_listValN.clear();
    }
    m_selRegion=false;
*/
/*  THIS METHOD IS VERY VERY LONG 
    std::vector< std::vector<std::string>::iterator > stringIter;
    std::vector< std::vector<double>::iterator > doubleIter;
    for (j=0; j<sizeS; j++) stringIter.push_back(m_strings[j].begin());
    for (j=0; j<sizeN; j++) doubleIter.push_back(m_numericals[j].begin());
    if (keepCriteria) {

      std::vector< std::vector<unsigned long>::iterator > myIter;
      for (j=0; j<vecSize; j++) myIter.push_back(m_rowIsSelected[j].begin());
      for (i=0; i<m_numRows; i++) {
        if (!(*myIter[0] & 1)) {
          for (j=0; j<sizeS; j++) m_strings[j].erase(stringIter[j]);
          for (j=0; j<sizeN; j++) m_numericals[j].erase(doubleIter[j]);
          for (j=0; j<vecSize; j++) m_rowIsSelected[j].erase(myIter[j]);
        }
        else {
          for (j=0; j<sizeS; j++) stringIter.at(j)++;
          for (j=0; j<sizeN; j++) doubleIter.at(j)++;
          for (j=0; j<vecSize; j++) myIter.at(j)++;
        }
      }
      stringIter.clear();
      doubleIter.clear();
      myIter.clear();
*/
  }
  catch (const std::exception &prob) {
    text="EXCEPTION erasing m_strings, m_numericals or m_rowIsSelected: ";
    text=text+prob.what();
    printErr(origin, text);
    throw;
  }
  std::ostringstream sortie;
  sortie << m_numRows-m_numSelRows << " row(s) deleted";
  printLog(0, sortie.str());
  m_numRows=m_numSelRows;
  //if (!keepCriteria) m_numSelRows=0;
  return IS_OK;
}
/**********************************************************************/
// erase all selected rows from memory
int Catalog::eraseSelected() {

  const std::string origin="eraseSelected";
  if (m_numRows <= 0) {
    printWarn(origin, "catalog is empty");
    return IS_VOID; 
  }
  // test below avoid deleting NOTHING
  if (m_numSelRows == 0) {
    printWarn(origin, "no row selected, nothing done");
    return IS_OK; 
  }
  // log if ALL is deleted
  std::string text;
  if (m_numRows == m_numSelRows) {
    text="all rows selected, calling deleteContent()";
    printLog(2, text);
    deleteContent();
    return IS_OK;
  }
  try {
    long i, tot=0,
         numRows=m_numRows-m_numSelRows;
    int j;
    int sizeS=m_strings.size();
    int sizeN=m_numericals.size();
    int vecSize=m_rowIsSelected.size();
    // to speed-up, will not change (avoid reading size in loop)

    std::vector<std::vector<double> > myNum;
    myNum.resize(sizeN);
    for (j=0; j<sizeN; j++) myNum[j].assign(numRows, 0.0);
    std::vector<std::vector<std::string> > myStr;
    myStr.resize(sizeS);
    for (j=0; j<sizeS; j++) myStr[j].assign(numRows, "");
    std::vector<std::vector<unsigned long> > myBits;
    myBits.resize(vecSize);
    for (j=0; j<vecSize; j++) myBits[j].assign(numRows, 0ul);
    for (i=0; i<m_numRows; i++) {
      if (!(m_rowIsSelected[0].at(i) & 1)) {
        for (j=0; j<sizeN; j++) myNum[j].at(tot)=m_numericals[j].at(i);
        for (j=0; j<sizeS; j++) myStr[j].at(tot)=m_strings[j].at(i);
        for (j=0; j<vecSize; j++) myBits[j].at(tot)=m_rowIsSelected[j].at(i);
        if (++tot == numRows) break; // to speed up
      }
    }
    for (j=0; j<sizeN; j++) {
      m_numericals[j].insert(m_numericals[j].begin(),
                             myNum[j].begin(), myNum[j].end());
      myNum[j].clear();
    }
    myNum.clear();
    for (j=0; j<sizeS; j++) {
      m_strings[j].insert(m_strings[j].begin(),
                          myStr[j].begin(), myStr[j].end());
      myStr[j].clear();
    }
    myStr.clear();
    for (j=0; j<vecSize; j++)  {
      m_rowIsSelected[j].insert(m_rowIsSelected[j].begin(),
                                myBits[j].begin(), myBits[j].end());
      myBits[j].clear();
    }
    myBits.clear();
/* if (!keepCriteria) {
    for (j=0; j<vecSize; j++) m_rowIsSelected[j].assign(numRows, 0);
    vecSize=m_quantities.size();
    for (i=0; i<vecSize; i++) {
      m_quantities.at(i).m_listValS.clear();
      m_quantities.at(i).m_lowerCut=NO_SEL_CUT;
      m_quantities.at(i).m_upperCut=NO_SEL_CUT;
      m_quantities.at(i).m_listValN.clear();
    }
    m_selRegion=false;
*/
  }
  catch (const std::exception &prob) {
    text="EXCEPTION erasing m_strings, m_numericals or m_rowIsSelected: ";
    text=text+prob.what();
    printErr(origin, text);
    throw;
  }
  std::ostringstream sortie;
  sortie << m_numSelRows << " row(s) deleted";
  printLog(0, sortie.str());
  m_numRows=m_numRows-m_numSelRows;
  m_numSelRows=0;
  return IS_OK;
}

} // namespace catalogAccess
