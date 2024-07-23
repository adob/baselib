#pragma once
#include "lib/base.h"
namespace lib::os {
    
    extern deferror ErrPERM;
    extern deferror ErrNOENT;
    extern deferror ErrSRCH;
    extern deferror ErrINTR;
    extern deferror ErrIO;
    extern deferror ErrNXIO;
    extern deferror Err2BIG;
    extern deferror ErrNOEXEC;
    extern deferror ErrBADF;
    extern deferror ErrCHILD;
    extern deferror ErrAGAIN;
    extern deferror ErrNOMEM;
    extern deferror ErrACCES;
    extern deferror ErrFAULT;
    extern deferror ErrNOTBLK;
    extern deferror ErrBUSY;
    extern deferror ErrEXIST;
    extern deferror ErrXDEV;
    extern deferror ErrNODEV;
    extern deferror ErrNOTDIR;
    extern deferror ErrISDIR;
    extern deferror ErrINVAL;
    extern deferror ErrNFILE;
    extern deferror ErrMFILE;
    extern deferror ErrNOTTY;
    extern deferror ErrTXTBSY;
    extern deferror ErrFBIG;
    extern deferror ErrNOSPC;
    extern deferror ErrSPIPE;
    extern deferror ErrROFS;
    extern deferror ErrMLINK;
    extern deferror ErrPIPE;
    extern deferror ErrDOM;
    extern deferror ErrRANGE;

    extern deferror ErrDEADLK;
    extern deferror ErrNAMETOOLONG;
    extern deferror ErrNOLCK;

    extern deferror ErrNOSYS;

    extern deferror ErrNOTEMPTY;
    extern deferror ErrLOOP;
    extern deferror ErrWOULDBLOCK;
    extern deferror ErrNOMSG;
    extern deferror ErrIDRM;
    extern deferror ErrCHRNG;
    extern deferror ErrL2NSYNC;
    extern deferror ErrL3HLT;
    extern deferror ErrL3RST;
    extern deferror ErrLNRNG;
    extern deferror ErrUNATCH;
    extern deferror ErrNOCSI;
    extern deferror ErrL2HLT;
    extern deferror ErrBADE;
    extern deferror ErrBADR;
    extern deferror ErrXFULL;
    extern deferror ErrNOANO;
    extern deferror ErrBADRQC;
    extern deferror ErrBADSLT;

    extern deferror ErrDEADLOCK;

    extern deferror ErrBFONT;
    extern deferror ErrNOSTR;
    extern deferror ErrNODATA;
    extern deferror ErrTIME;
    extern deferror ErrNOSR;
    extern deferror ErrNONET;
    extern deferror ErrNOPKG;
    extern deferror ErrREMOTE;
    extern deferror ErrNOLINK;
    extern deferror ErrADV;
    extern deferror ErrSRMNT;
    extern deferror ErrCOMM;
    extern deferror ErrPROTO;
    extern deferror ErrMULTIHOP;
    extern deferror ErrDOTDOT;
    extern deferror ErrBADMSG;
    extern deferror ErrOVERFLOW;
    extern deferror ErrNOTUNIQ;
    extern deferror ErrBADFD;
    extern deferror ErrREMCHG;
    extern deferror ErrLIBACC;
    extern deferror ErrLIBBAD;
    extern deferror ErrLIBSCN;
    extern deferror ErrLIBMAX;
    extern deferror ErrLIBEXEC;
    extern deferror ErrILSEQ;
    extern deferror ErrRESTART;
    extern deferror ErrSTRPIPE;
    extern deferror ErrUSERS;
    extern deferror ErrNOTSOCK;
    extern deferror ErrDESTADDRREQ;
    extern deferror ErrMSGSIZE;
    extern deferror ErrPROTOTYPE;
    extern deferror ErrNOPROTOOPT;
    extern deferror ErrPROTONOSUPPORT;
    extern deferror ErrSOCKTNOSUPPORT;
    extern deferror ErrOPNOTSUPP;
    extern deferror ErrPFNOSUPPORT;
    extern deferror ErrAFNOSUPPORT;
    extern deferror ErrADDRINUSE;
    extern deferror ErrADDRNOTAVAIL;
    extern deferror ErrNETDOWN;
    extern deferror ErrNETUNREACH;
    extern deferror ErrNETRESET;
    extern deferror ErrCONNABORTED;
    extern deferror ErrCONNRESET;
    extern deferror ErrNOBUFS;
    extern deferror ErrISCONN;
    extern deferror ErrNOTCONN;
    extern deferror ErrSHUTDOWN;
    extern deferror ErrTOOMANYREFS;
    extern deferror ErrTIMEDOUT;
    extern deferror ErrCONNREFUSED;
    extern deferror ErrHOSTDOWN;
    extern deferror ErrHOSTUNREACH;
    extern deferror ErrALREADY;
    extern deferror ErrINPROGRESS;
    extern deferror ErrSTALE;
    extern deferror ErrUCLEAN;
    extern deferror ErrNOTNAM;
    extern deferror ErrNAVAIL;
    extern deferror ErrISNAM;
    extern deferror ErrREMOTEIO;
    extern deferror ErrDQUOT;

    extern deferror ErrNOMEDIUM;
    extern deferror ErrMEDIUMTYPE;
    extern deferror ErrCANCELED;
    extern deferror ErrNOKEY;
    extern deferror ErrKEYEXPIRED;
    extern deferror ErrKEYREVOKED;
    extern deferror ErrKEYREJECTED;

    extern deferror ErrOWNERDEAD;
    extern deferror ErrNOTRECOVERABLE;

    extern deferror ErrRFKILL;

    extern deferror ErrHWPOISON;
    
    extern deferror ErrUnknown;

    deferror &from_errno(int e);
}
