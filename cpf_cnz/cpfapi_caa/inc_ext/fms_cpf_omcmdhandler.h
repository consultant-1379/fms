#ifndef FMS_CPF_OMCMDHANDLER_H
#define FMS_CPF_OMCMDHANDLER_H

#include <iostream>
#include <acs_apgcc_omhandler.h>

class ACS_TRA_trace;

#include <string>

class FMS_CPF_omCmdHandler
{
    int numAttr;
    int curNumAttr;
    const char* parentName;
    const char* className;
    std::string immMsg;
    int immErrCode;
    ACS_CC_ImmParameter paramToChange;
    vector<ACS_CC_ValuesDefinitionType> AttrList;
    ACS_CC_ValuesDefinitionType *attributes;
    char dn[50];
    const char* ObjectDN;
    OmHandler omHandler;
    void addNewAttrBase(char *aName, ACS_CC_AttrValueType attrType);

public:
    FMS_CPF_omCmdHandler();
    FMS_CPF_omCmdHandler(int nAttr, const char *pName, const char *cName);

    // Constructor used to modify an attribute of an IMM object
    FMS_CPF_omCmdHandler(const char *DN, char *attrName, char *attrVal,ACS_CC_AttrValueType attrType);
    void addNewAttr(char *aName, ACS_CC_AttrValueType attrType, const char *attrVal);
    void addNewAttr(char *aName, ACS_CC_AttrValueType attrType, int *valueI32);
    void addNewAttr(char *aName, ACS_CC_AttrValueType attrType, unsigned int *valueUnsInt32);
    int searchDN(char *rootName, std::string txtToFind, std::string &strResult);
    int loadClassInst(const char* className, std::vector<std::string>&classDnList);
    int loadChildInst(const char* className, ACS_APGCC_ScopeT subLevel, std::vector<std::string> *classDnList);
    int getAttrVal(std::string objName, std::string, ACS_CC_ImmParameter *paramToFind);
    int getAttrListVal(std::string p_objectName, std::vector<ACS_APGCC_ImmAttribute *> p_attributeList);
    int deleteObject(const char* p_ObjName);
    int deleteObject(const char* p_ObjName, ACS_APGCC_ScopeT p_scope);
    int writeObj(void);

    // Modify the Object attributes in the IMM structure
    int modifyObj(void);

    ~FMS_CPF_omCmdHandler();
    std::string getLastImmError(void);
    int getLastImmErrorCode(void);
    bool isInitialized();
    int finalize();
	ACS_TRA_trace* fmsCpfOmHandlerTrace;
private:
	bool m_initialized;

	/** @brief setImmErrorResult() method
	 *
	 *	This method sets the Error status after an IMM operation
	*/
    void setImmOpResult();

	/** @brief initImmOpResult() method
	 *
	 *	This method initialize the IMM Operation status
	*/
    void initImmOpResult();


};

#endif
