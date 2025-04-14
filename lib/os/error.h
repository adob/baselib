#pragma once
#include <errno.h>
#include "lib/base.h"
#include "lib/fs/fs.h"
#include "lib/io/io_stream.h"

namespace lib::os {

    struct Errno : ErrorBase<Errno> {
        int code;

        Errno(int code) : code(code) {}

        virtual void fmt(io::Writer &out, error err) const override;
        virtual bool is(TypeID) const override;
        
    } ;

    struct SyscallError : ErrorBase<SyscallError> {
        str syscall;
        int code;

        SyscallError(str syscall, int code) : syscall(syscall), code(code) {}
        virtual void fmt(io::Writer &out, error err) const override;
    } ;

    struct Error : ErrorBase<Error> {
        str function_name;
        int code;  // errno code

        Error(str function_name, int code) : function_name(function_name), code(code) {}
        virtual void fmt(io::Writer &out, error err) const override;
    } ;

    using fs::PathError;

    using fs::ErrInvalid;
    using fs::ErrPermission;
    using fs::ErrExist;
    using fs::ErrClosed;
    
    // extern Error ErrPERM;
    // extern Error ErrNOENT;
    // extern Error ErrSRCH;
    // extern Error ErrINTR;
    // extern Error ErrIO;
    // extern Error ErrNXIO;
    // extern Error Err2BIG;
    // extern Error ErrNOEXEC;
    // extern Error ErrBADF;
    // extern Error ErrCHILD;
    // extern Error ErrAGAIN;
    // extern Error ErrNOMEM;
    // extern Error ErrACCES;
    // extern Error ErrFAULT;
    // extern Error ErrNOTBLK;
    // extern Error ErrBUSY;
    // extern Error ErrEXIST;
    // extern Error ErrXDEV;
    // extern Error ErrNODEV;
    // extern Error ErrNOTDIR;
    // extern Error ErrISDIR;
    // extern Error ErrINVAL;
    // extern Error ErrNFILE;
    // extern Error ErrMFILE;
    // extern Error ErrNOTTY;
    // extern Error ErrTXTBSY;
    // extern Error ErrFBIG;
    // extern Error ErrNOSPC;
    // extern Error ErrSPIPE;
    // extern Error ErrROFS;
    // extern Error ErrMLINK;
    // extern Error ErrPIPE;
    // extern Error ErrDOM;
    // extern Error ErrRANGE;

    // extern Error ErrDEADLK;
    // extern Error ErrNAMETOOLONG;
    // extern Error ErrNOLCK;

    // extern Error ErrNOSYS;

    // extern Error ErrNOTEMPTY;
    // extern Error ErrLOOP;
    // extern Error ErrWOULDBLOCK;
    // extern Error ErrNOMSG;
    // extern Error ErrIDRM;
    // extern Error ErrCHRNG;
    // extern Error ErrL2NSYNC;
    // extern Error ErrL3HLT;
    // extern Error ErrL3RST;
    // extern Error ErrLNRNG;
    // extern Error ErrUNATCH;
    // extern Error ErrNOCSI;
    // extern Error ErrL2HLT;
    // extern Error ErrBADE;
    // extern Error ErrBADR;
    // extern Error ErrXFULL;
    // extern Error ErrNOANO;
    // extern Error ErrBADRQC;
    // extern Error ErrBADSLT;

    // extern Error ErrDEADLOCK;

    // extern Error ErrBFONT;
    // extern Error ErrNOSTR;
    // extern Error ErrNODATA;
    // extern Error ErrTIME;
    // extern Error ErrNOSR;
    // extern Error ErrNONET;
    // extern Error ErrNOPKG;
    // extern Error ErrREMOTE;
    // extern Error ErrNOLINK;
    // extern Error ErrADV;
    // extern Error ErrSRMNT;
    // extern Error ErrCOMM;
    // extern Error ErrPROTO;
    // extern Error ErrMULTIHOP;
    // extern Error ErrDOTDOT;
    // extern Error ErrBADMSG;
    // extern Error ErrOVERFLOW;
    // extern Error ErrNOTUNIQ;
    // extern Error ErrBADFD;
    // extern Error ErrREMCHG;
    // extern Error ErrLIBACC;
    // extern Error ErrLIBBAD;
    // extern Error ErrLIBSCN;
    // extern Error ErrLIBMAX;
    // extern Error ErrLIBEXEC;
    // extern Error ErrILSEQ;
    // extern Error ErrRESTART;
    // extern Error ErrSTRPIPE;
    // extern Error ErrUSERS;
    // extern Error ErrNOTSOCK;
    // extern Error ErrDESTADDRREQ;
    // extern Error ErrMSGSIZE;
    // extern Error ErrPROTOTYPE;
    // extern Error ErrNOPROTOOPT;
    // extern Error ErrPROTONOSUPPORT;
    // extern Error ErrSOCKTNOSUPPORT;
    // extern Error ErrOPNOTSUPP;
    // extern Error ErrPFNOSUPPORT;
    // extern Error ErrAFNOSUPPORT;
    // extern Error ErrADDRINUSE;
    // extern Error ErrADDRNOTAVAIL;
    // extern Error ErrNETDOWN;
    // extern Error ErrNETUNREACH;
    // extern Error ErrNETRESET;
    // extern Error ErrCONNABORTED;
    // extern Error ErrCONNRESET;
    // extern Error ErrNOBUFS;
    // extern Error ErrISCONN;
    // extern Error ErrNOTCONN;
    // extern Error ErrSHUTDOWN;
    // extern Error ErrTOOMANYREFS;
    // extern Error ErrTIMEDOUT;
    // extern Error ErrCONNREFUSED;
    // extern Error ErrHOSTDOWN;
    // extern Error ErrHOSTUNREACH;
    // extern Error ErrALREADY;
    // extern Error ErrINPROGRESS;
    // extern Error ErrSTALE;
    // extern Error ErrUCLEAN;
    // extern Error ErrNOTNAM;
    // extern Error ErrNAVAIL;
    // extern Error ErrISNAM;
    // extern Error ErrREMOTEIO;
    // extern Error ErrDQUOT;

    // extern Error ErrNOMEDIUM;
    // extern Error ErrMEDIUMTYPE;
    // extern Error ErrCANCELED;
    // extern Error ErrNOKEY;
    // extern Error ErrKEYEXPIRED;
    // extern Error ErrKEYREVOKED;
    // extern Error ErrKEYREJECTED;

    // extern Error ErrOWNERDEAD;
    // extern Error ErrNOTRECOVERABLE;

    // extern Error ErrRFKILL;

    // extern Error ErrHWPOISON;
    
    // extern Error ErrUnknown;

    // Error &from_errno(int e);
}
